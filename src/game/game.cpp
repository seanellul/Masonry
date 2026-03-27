/*	
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "game.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../base/pathfinder.h"
#include "../base/util.h"
#include "../game/animal.h"

#include "../game/inventory.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/farmingmanager.h"
#include "../game/fluidmanager.h"
#include "../game/gnomemanager.h"
#include "../game/mechanismmanager.h"
#include "../game/militarymanager.h"
#include "../game/newgamesettings.h"
#include "../game/roommanager.h"
#include "../game/soundmanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../gfx/spritefactory.h"

#include "../game/gamemanager.h"
#include "../game/gnome.h"
#include "../game/spatialgrid.h"

#include "../game/itemhistory.h"
#include "../game/object.h"
#include "../game/plant.h"
#include "../game/techtree.h"
#include "../game/world.h"
#include "../game/worldgenerator.h"

#include "../gui/eventconnector.h"
#include "../gui/strings.h"
#include "../gui/aggregatorcreatureinfo.h"

#include <future>

#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>

#include <time.h>

Game::Game( QObject* parent ) :
	QObject( parent )
{
	qDebug() << "init game...";
	
	m_upsTimer.start();

	m_sf.reset( new SpriteFactory() );
	
	#pragma region initStuff
	DB::select( "Value_", "Time", "MillisecondsSlow" );

	m_millisecondsSlow = DB::select( "Value_", "Time", "MillisecondsSlow" ).toInt();
	m_millisecondsFast = DB::select( "Value_", "Time", "MillisecondsFast" ).toInt();

	GameState::tick = 1;
	GameState::hour = 9;
	GameState::minute = 0;
	GameState::day    = 1;
	GameState::season = 0; // zero based, 0-numSeasons, typically 4 can be modded
	GameState::year   = 1;

	QString dt = QString( "Day %1, %2:%3" ).arg( GameState::day, 2, 10, QChar( ' ' ) ).arg( GameState::hour, 2, 10, QChar( '0' ) ).arg( GameState::minute, 2, 10, QChar( '0' ) );
	GameState::currentDayTime = dt;
	GameState::seasonString = "Spring";
	GameState::currentYearAndSeason = "Year " + QString::number( GameState::year ) + ", " + S::s( "$SeasonName_" + GameState::seasonString );

	calcDaylight();
	GameState::daylight = true;

	for( auto t : DB::ids( "Tech" ) )
	{
		GameState::techs.insert( t, 1 );
	}
#pragma endregion
	m_inv   			= new Inventory( this );
	
	m_spm				= new StockpileManager( this );
	m_farmingManager	= new FarmingManager( this );
	m_workshopManager	= new WorkshopManager( this );
	m_roomManager		= new RoomManager( this );

	m_jobManager		= new JobManager( this );
	
	m_creatureManager	= new CreatureManager( this );
	m_gnomeManager		= new GnomeManager( this );
	m_militaryManager	= new MilitaryManager( this );

	m_mechanismManager	= new MechanismManager( this );
	m_fluidManager		= new FluidManager( this );
	
	m_neighborManager	= new NeighborManager( this );
	m_eventManager		= new EventManager( this );
	
	m_soundManager	= new SoundManager( this );

	qDebug() << "init game done";
}


Game::~Game()
{
	delete m_spatialGrid;
}

void Game::generateWorld( NewGameSettings* ngs )
{
	m_inv->loadFilter();

	WorldGenerator wg( ngs, this );
	connect( &wg, &WorldGenerator::signalStatus, dynamic_cast<GameManager*>( parent() ), &GameManager::onGeneratorMessage );
	m_world.reset( wg.generateTopology() );	
	wg.addLife();

	m_pf.reset( new PathFinder( m_world.get(), this ) );

	delete m_spatialGrid;
	m_spatialGrid = new SpatialGrid( Global::dimX, Global::dimY, Global::dimZ );
}

void Game::setWorld( int dimX, int dimY, int dimZ )
{
	m_world.reset( new World( dimX, dimY, dimZ, this ) );
	m_pf.reset( new PathFinder( m_world.get(), this ) );

	delete m_spatialGrid;
	m_spatialGrid = new SpatialGrid( dimX, dimY, dimZ );
}

void Game::start()
{
	qDebug() << "Starting game";
	Global::logger().log( LogType::INFO, "Game started. " + GameState::currentYearAndSeason + ", " + GameState::currentDayTime, 0 );

	if ( GameState::tick == 0 && !GameState::initialSave )
	{
		Global::cfg->set( "DaysToNextAutoSave", 0 );
		autoSave();
		Global::cfg->set( "Pause", true );
		emit signalPause( true );
	}

	if ( m_timer )
	{
		stop();
	}
	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &Game::loop );
	m_timer->start( m_millisecondsSlow );
}

void Game::stop()
{
	qDebug() << "Stop game";
	if ( m_timer )
	{
		m_timer->stop();
		delete m_timer;
	}
}

void Game::loop()
{
	QElapsedTimer timer;
	timer.start();
	if (m_guiHeartbeat <= m_guiHeartbeatResponse+20)
	{
		m_upsCounter1++;
		if ( m_upsTimer.elapsed() > 1000 ) 
		{
			m_upsTimer.restart();
			//printf(" gameloop ups %d avg ms %d\n", m_upsCounter1, m_avgLoopTime/m_upsCounter1);
			m_upsCounter = m_upsCounter1;
			m_upsCounter1 = 0;
			m_avgLoopTime = 0;
		}
		int ms2 = 0;
		
		if ( !m_paused )
		{
			emit sendOverlayMessage( 6, "tick " + QString::number( GameState::tick ) );

			sendClock();

			// Tick profiling — logs every 100 ticks
			QElapsedTimer phase;
			auto sc = GameState::seasonChanged;
			auto dc = GameState::dayChanged;
			auto hc = GameState::hourChanged;
			auto mc = GameState::minuteChanged;
			bool profileTick = ( GameState::tick % 50 == 0 );

			// Collect pathfinding results from previous tick's dispatch
			// Use nsecsElapsed for microsecond precision (restart() only gives ms)
			auto usElapsed = [&phase]() -> qint64 {
				qint64 us = phase.nsecsElapsed() / 1000;
				phase.restart();
				return us;
			};
			phase.start();
			m_pf->collectPaths();
			qint64 tPathCollect = usElapsed();

			// Phase 2: Parallel natural world processing
			// Grass (vegetationLevel), water (fluidLevel), and plants (m_plants map) touch independent data
			auto grassFuture = std::async( std::launch::async, [this]{ m_world->processGrass(); } );
			auto waterFuture = std::async( std::launch::async, [this]{ m_world->processWater(); } );
			processPlants();
			grassFuture.get();
			waterFuture.get();
			qint64 tGrass = 0; // Combined into parallel block
			qint64 tPlants = usElapsed();

			m_creatureManager->onTick( GameState::tick, sc, dc, hc, mc );
			qint64 tCreatures = usElapsed();

			m_gnomeManager->onTick( GameState::tick, sc, dc, hc, mc );
			ms2 = phase.elapsed();
			qint64 tGnomes = usElapsed();

			m_jobManager->onTick();
			qint64 tJobs = usElapsed();

			m_spm->onTick( GameState::tick );
			qint64 tStockpiles = usElapsed();

			m_farmingManager->onTick( GameState::tick, sc, dc, hc, mc );
			qint64 tFarming = usElapsed();

			m_workshopManager->onTick( GameState::tick );
			qint64 tWorkshops = usElapsed();

			// Phase 3: Passive systems run in parallel with event/history processing
			auto passiveFuture = std::async( std::launch::async, [this, sc, dc, hc, mc]{
				m_roomManager->onTick( GameState::tick );
				m_mechanismManager->onTick( GameState::tick, sc, dc, hc, mc );
				m_fluidManager->onTick( GameState::tick, sc, dc, hc, mc );
				m_neighborManager->onTick( GameState::tick, sc, dc, hc, mc );
				m_soundManager->onTick( GameState::tick );
			});

			m_inv->itemHistory()->onTick( dc );
			m_eventManager->onTick( GameState::tick, sc, dc, hc, mc );
			qint64 tEvents = usElapsed();

			passiveFuture.get();
			qint64 tRooms = 0;       // Combined into passive block
			qint64 tMechanisms = 0;
			qint64 tFluids = 0;
			qint64 tNeighbors = 0;
			qint64 tSound = usElapsed();

			// Water already processed in parallel block above
			qint64 tWater = 0;

			// Dispatch pathfinding workers (non-blocking — results collected next tick)
			m_pf->dispatchPaths();
			qint64 tPaths = usElapsed();

			// Store timing for benchmark/perf queries
			{
				qint64 total = tPathCollect + tPlants + tCreatures + tGnomes + tJobs +
					tStockpiles + tFarming + tWorkshops + tEvents + tSound + tPaths;
				m_lastTickTiming.pathCollect_us = tPathCollect;
				m_lastTickTiming.naturalWorld_us = tPlants; // includes parallel grass+water
				m_lastTickTiming.creatures_us = tCreatures;
				m_lastTickTiming.gnomes_us = tGnomes;
				m_lastTickTiming.jobs_us = tJobs;
				m_lastTickTiming.stockpiles_us = tStockpiles;
				m_lastTickTiming.farming_us = tFarming;
				m_lastTickTiming.workshops_us = tWorkshops;
				m_lastTickTiming.passive_us = tSound; // includes parallel rooms+mechanisms+fluids+neighbors
				m_lastTickTiming.events_us = tEvents;
				m_lastTickTiming.pathDispatch_us = tPaths;
				m_lastTickTiming.total_us = total;
			}

			if ( profileTick )
			{
				qint64 total = tPathCollect + tGrass + tPlants + tCreatures + tGnomes + tJobs +
					tStockpiles + tFarming + tWorkshops + tRooms + tEvents +
					tMechanisms + tFluids + tNeighbors + tSound + tWater + tPaths;
				qDebug().noquote() << QString(
					"[TICK %1] %2ms | pathCollect:%3 grass:%4 plants:%5 creatures:%6 gnomes:%7 jobs:%8 "
					"stockpiles:%9 farming:%10 workshops:%11 rooms:%12 events:%13 "
					"mechanisms:%14 fluids:%15 neighbors:%16 sound:%17 water:%18 pathDispatch:%19" )
					.arg( GameState::tick )
					.arg( total )
					.arg( tPathCollect ).arg( tGrass ).arg( tPlants ).arg( tCreatures ).arg( tGnomes )
					.arg( tJobs ).arg( tStockpiles ).arg( tFarming ).arg( tWorkshops )
					.arg( tRooms ).arg( tEvents ).arg( tMechanisms ).arg( tFluids )
					.arg( tNeighbors ).arg( tSound ).arg( tWater ).arg( tPaths );
			}

			++GameState::tick;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// update gui
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
		QElapsedTimer guiPhase;
		guiPhase.start();

		auto updates = m_world->updatedTiles();
		if ( !updates.empty() )
		{
			signalUpdateTileInfo( std::move( updates ) );
		}
		qint64 tTileUpdates = guiPhase.restart();

		emit signalUpdateStockpile();
		qint64 tStockpileSignal = guiPhase.restart();

		Global::eventConnector->aggregatorCreatureInfo()->update();
		qint64 tCreatureInfo = guiPhase.restart();

		if ( !m_paused && GameState::tick % 50 == 1 )
		{
			qDebug().noquote() << QString( "[GUI] tileUpdates:%1 stockpileSignal:%2 creatureInfo:%3 ms" )
				.arg( tTileUpdates ).arg( tStockpileSignal ).arg( tCreatureInfo );
		}

		int ms        = timer.elapsed();
		m_maxLoopTime = qMax( ms2, m_maxLoopTime );
	
		auto numString = QString::number( ms );
		while ( numString.size() < 5 )
			numString.prepend( '0' );
	
		QString msg = "game loop time: " + numString;
		if ( Global::debugMode )
			msg += " ms (max gnome time:" + QString::number( m_maxLoopTime ) + "ms)";
		emit sendOverlayMessage( 3, msg );
	
		
		emit signalKingdomInfo( GameState::kingdomName,
			"Gnomes: " + QString::number( gm()->numGnomes() ),
			"Food: " + QString::number( inv()->foodItemCount() ) + " | Drink: " + QString::number( inv()->drinkItemCount() ),
			"Items: "  + QString::number( inv()->numItems() ) );

		m_guiHeartbeat = m_guiHeartbeat + 1;
		emit signalHeartbeat(m_guiHeartbeat);
	}

	
	m_avgLoopTime += timer.elapsed();
	
}

void Game::sendClock()
{
	GameState::minuteChanged = false;
	GameState::hourChanged   = false;
	GameState::dayChanged    = false;
	GameState::seasonChanged = false;

	if ( GameState::tick % Global::util->ticksPerMinute == 0 )
	{
		++GameState::minute;
		GameState::minuteChanged = true;
	}
	if ( GameState::minute == Global::util->minutesPerHour )
	{
		GameState::minute = 0;
		++GameState::hour;
		GameState::hourChanged = true;
	}
	if ( GameState::hour == Global::util->hoursPerDay )
	{
		GameState::hour = 0;
		++GameState::day;
		GameState::dayChanged = true;
	}

	if ( GameState::dayChanged )
	{
		int daysThisSeason = DB::select( "NumDays", "Seasons", GameState::seasonString ).toInt();
		if ( GameState::day > daysThisSeason )
		{
			GameState::day = 1;
			++GameState::season;
			GameState::seasonChanged    = true;
			QString nextSeason = DB::select( "NextSeason", "Seasons", GameState::seasonString ).toString();
			//qDebug() << "Now it's " << nextSeason;
			GameState::seasonString = nextSeason;

			Global::util->daysPerSeason = DB::select( "NumDays", "Seasons", nextSeason ).toInt();

			int numSeasonsPerYear = DB::numRows( "Seasons" );
			if ( GameState::season == numSeasonsPerYear )
			{
				GameState::season = 0;
				++GameState::year;
			}
		}

		calcDaylight();

		autoSave();
	}
	if ( GameState::seasonChanged )
	{
		auto gm = dynamic_cast<GameManager*>( parent() );
		m_sf->forceUpdate();
	}

	QString sunStatus;
	int currentTimeInt = GameState::hour * Global::util->minutesPerHour + GameState::minute;
	if ( currentTimeInt < GameState::sunrise )
	{
		//time between midnight and sunrise
		sunStatus        = "Sunrise: " + intToTime( GameState::sunrise );
		GameState::daylight = false;
	}
	else if ( currentTimeInt < GameState::sunset )
	{
		// day time
		sunStatus        = "Sunset: " + intToTime( GameState::sunset );
		GameState::daylight = true;
	}
	else
	{
		// time after sunset
		sunStatus        = "Sunrise: " + intToTime( GameState::nextSunrise );
		GameState::daylight = false;
	}
	QString dt = QString( "Day %1, %2:%3" ).arg( GameState::day, 2, 10, QChar( ' ' ) ).arg( GameState::hour, 2, 10, QChar( '0' ) ).arg( GameState::minute, 2, 10, QChar( '0' ) );
	GameState::currentDayTime = dt;
	GameState::currentYearAndSeason = "Year " + QString::number( GameState::year ) + ", " + S::s( "$SeasonName_" + GameState::seasonString );

	// Update temperature based on season + weather modifiers
	if ( GameState::hourChanged )
	{
		float baseTemp = 50.0f;
		if ( GameState::seasonString == "Spring" ) baseTemp = 50.0f;
		else if ( GameState::seasonString == "Summer" ) baseTemp = 70.0f;
		else if ( GameState::seasonString == "Autumn" ) baseTemp = 40.0f;
		else if ( GameState::seasonString == "Winter" ) baseTemp = 20.0f;

		// Day/night variation: ±5 degrees
		baseTemp += GameState::daylight ? 5.0f : -5.0f;

		// Weather modifiers
		if ( GameState::activeWeather == "HeatWave" ) baseTemp += 20.0f;
		else if ( GameState::activeWeather == "ColdSnap" ) baseTemp -= 20.0f;

		GameState::temperature = qBound( 0.0f, baseTemp, 100.0f );
	}

	// Random weather events (check daily)
	if ( GameState::dayChanged )
	{
		// Clear expired weather (lasts 3 days, tracked by simple random chance of clearing)
		if ( !GameState::activeWeather.isEmpty() && ( rand() % 3 == 0 ) )
		{
			Global::logger().log( LogType::INFO, "The " + GameState::activeWeather + " has passed.", 0 );
			GameState::activeWeather = "";
		}

		// Random chance of new weather event (5% per day)
		if ( GameState::activeWeather.isEmpty() && ( rand() % 20 == 0 ) )
		{
			int roll = rand() % 3;
			switch ( roll )
			{
				case 0:
					GameState::activeWeather = "Storm";
					Global::logger().log( LogType::WARNING, "A storm has rolled in!", 0 );
					break;
				case 1:
					GameState::activeWeather = "HeatWave";
					Global::logger().log( LogType::WARNING, "A heat wave has begun!", 0 );
					break;
				case 2:
					GameState::activeWeather = "ColdSnap";
					Global::logger().log( LogType::WARNING, "A cold snap has set in!", 0 );
					break;
			}
		}
	}

	emit signalTimeAndDate( GameState::minute, GameState::hour, GameState::day, S::s( "$SeasonName_" + GameState::seasonString ), GameState::year, sunStatus );
}

void Game::sendTime()
{
	emit signalTimeAndDate( GameState::minute, GameState::hour, GameState::day, S::s( "$SeasonName_" + GameState::seasonString ), GameState::year, "" );
}

void Game::calcDaylight()
{
	QString currentSeason = GameState::seasonString;
	QString sunrise       = DB::select( "SunRiseFirst", "Seasons", currentSeason ).toString();
	QString sunset        = DB::select( "SunSetFirst", "Seasons", currentSeason ).toString();
	QString nextSeason    = DB::select( "NextSeason", "Seasons", currentSeason ).toString();

	QString nextSunrise = DB::select( "SunRiseFirst", "Seasons", nextSeason ).toString();
	QString nextSunset  = DB::select( "SunSetFirst", "Seasons", nextSeason ).toString();

	int numDays = DB::select( "NumDays", "Seasons", currentSeason ).toInt();

	int sr1 = timeToInt( sunrise );
	int sr2 = timeToInt( nextSunrise );
	int ss1 = timeToInt( sunset );
	int ss2 = timeToInt( nextSunset );

	GameState::sunrise     = sr1 + ( ( sr2 - sr1 ) / numDays ) * ( GameState::day - 1 );
	GameState::nextSunrise = sr1 + ( ( sr2 - sr1 ) / numDays ) * ( GameState::day );
	GameState::sunset      = ss1 + ( ( ss2 - ss1 ) / numDays ) * ( GameState::day - 1 );

	//qDebug() << "sunrise: " << intToTime( m_sunrise ) << " sunset:" << intToTime( m_sunset );
}

int Game::timeToInt( QString time )
{
	QStringList tl = time.split( ":" );
	return tl[0].toInt() * Global::util->minutesPerHour + tl[1].toInt();
}

QString Game::intToTime( int time )
{
	int hour    = time / Global::util->minutesPerHour;
	int minute  = time - ( hour * Global::util->minutesPerHour );
	QString out = "";
	if ( hour < 10 )
		out += "0";
	out += QString::number( hour );
	out += ":";
	if ( minute < 10 )
		out += "0";
	out += QString::number( minute );
	return out;
}

void Game::processPlants()
{
	QList<Position> toRemove;
	for ( auto& p : m_world->plants() )
	{
		switch ( p.onTick( GameState::tick, GameState::dayChanged, GameState::seasonChanged ) )
		{
			case OnTickReturn::DESTROY:
				toRemove.push_back( p.getPos() );
				break;
			case OnTickReturn::UPDATE:
				break;
		}
	}
	for ( auto p : toRemove )
	{
		m_world->removePlant( p );
	}
}

void Game::autoSave()
{
	int daysToNext = Global::cfg->get( "DaysToNextAutoSave" ).toInt();

	if ( daysToNext == 0 )
	{
		Global::cfg->set( "Pause", true );
		emit signalStartAutoSave();
		emit signalPause( true );
		IO io( this, this );
		io.save( true );
		emit signalEndAutoSave();

		if ( Global::cfg->get( "AutoSaveContinue" ).toBool() )
		{
			emit signalPause( false );
			Global::cfg->set( "Pause", false );
		}

		Global::cfg->set( "DaysToNextAutoSave", Global::cfg->get( "AutoSaveInterval" ).toInt() - 1 );
	}
	else
	{
		--daysToNext;
		Global::cfg->set( "DaysToNextAutoSave", daysToNext );
	}
}

void Game::save()
{
	Global::cfg->set( "Pause", true );
	emit signalStartAutoSave();
	emit signalPause( true );
	IO io( this, this );
	io.save( true );
	emit signalEndAutoSave();

	if ( Global::cfg->get( "AutoSaveContinue" ).toBool() )
	{
		emit signalPause( false );
		Global::cfg->set( "Pause", false );
	}
}

	
GameSpeed Game::gameSpeed()
{
	return m_gameSpeed;
}

void Game::setGameSpeed( GameSpeed speed )
{
	m_gameSpeed = speed;
	switch( m_gameSpeed )
	{
		case GameSpeed::Normal:
			m_timer->setInterval( m_millisecondsSlow );
			break;
		case GameSpeed::Fast:
			m_timer->setInterval( m_millisecondsFast );
			break;
	}
}

bool Game::paused()
{
	return m_paused;
}

void Game::setPaused( bool value )
{
	m_paused = value;
	if (m_paused) 
	{
		m_timer->setInterval( m_millisecondsSlow*2 );
	}
	else {
		switch( m_gameSpeed )
		{
			case GameSpeed::Normal:
				m_timer->setInterval( m_millisecondsSlow );
				break;
			case GameSpeed::Fast:
				m_timer->setInterval( m_millisecondsFast );
				break;
		}
	}
	
}
void Game::setHeartbeatResponse( int value )
{
	//printf("heartbeatresponse %d", value);
	m_guiHeartbeatResponse = value;
}

Inventory*			Game::inv(){ return m_inv; }
ItemHistory*		Game::ih(){ return m_inv->itemHistory(); }
JobManager*			Game::jm(){ return m_jobManager; }
StockpileManager*	Game::spm(){ return m_spm; }
FarmingManager*		Game::fm(){ return m_farmingManager; }
WorkshopManager*	Game::wsm(){ return m_workshopManager; }
World*				Game::w(){ return m_world.get(); }
SpriteFactory*		Game::sf(){ return m_sf.get(); }
RoomManager*		Game::rm(){ return m_roomManager; }
GnomeManager*		Game::gm(){ return m_gnomeManager; }
CreatureManager*	Game::cm(){ return m_creatureManager; }
EventManager*		Game::em(){ return m_eventManager; }
MechanismManager*	Game::mcm(){ return m_mechanismManager; }
FluidManager*		Game::flm(){ return m_fluidManager; }
NeighborManager*	Game::nm(){ return m_neighborManager; }
MilitaryManager*	Game::mil(){ return m_militaryManager; }
SoundManager*		Game::sm(){ return m_soundManager; }
PathFinder*			Game::pf(){ return m_pf.get(); }
SpatialGrid*		Game::sg(){ return m_spatialGrid; }
World*				Game::world() { return m_world.get(); }
