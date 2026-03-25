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
#include "gnomemanager.h"
#include "game.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/io.h"
#include "../game/creature.h"
#include "../game/gnomefactory.h"
#include "../game/gnometrader.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/militarymanager.h"
#include "../game/world.h"
#include "../gfx/spritefactory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QStandardPaths>

GnomeManager::GnomeManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
	loadProfessions();
}

GnomeManager::~GnomeManager()
{
	for ( const auto& gnome : m_gnomes )
	{
		delete gnome;
	}
	for ( const auto& gnome : m_specialGnomes )
	{
		delete gnome;
	}
	for ( const auto& gnome : m_deadGnomes )
	{
		delete gnome;
	}
	for ( const auto& gnome : m_automatons )
	{
		delete gnome;
	}
}

bool GnomeManager::contains( unsigned int gnomeID )
{
	for( const auto& gnome : m_gnomes )
	{
		if( gnome->id() == gnomeID )
		{
			return true;
		}
	}
	return false;
}

void GnomeManager::addGnome( Position pos )
{
	GnomeFactory gf( g );
	m_gnomes.push_back( gf.createGnome( pos ) );
	m_gnomesByID.insert( m_gnomes.last()->id(), m_gnomes.last() );
}

unsigned int GnomeManager::addTrader( Position pos, unsigned int workshopID, QString type )
{
	GnomeFactory gf( g );
	GnomeTrader* gnome = gf.createGnomeTrader( pos );
	gnome->setName( "Trader " + gnome->name() );
	gnome->setMarketStall( workshopID );

	auto rows = DB::selectRows( "Traders_Items", type );

	if ( rows.size() )
	{
		QVariantMap vmTrader;
		vmTrader.insert( "ID", type );
		QVariantList items;
		for ( auto row : rows )
		{
			items.append( row );
		}
		vmTrader.insert( "Items", items );
		gnome->setTraderDefinition( vmTrader );
	}

	m_specialGnomes.push_back( gnome );
	m_gnomesByID.insert( gnome->id(), m_specialGnomes.last() );
	return gnome->id();
}

void GnomeManager::addAutomaton( Automaton* a )
{
	m_automatons.append( a );
	m_gnomesByID.insert( a->id(), m_automatons.last() );

	//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), g->m_inv->spriteID( a->automatonItem() ) ) );
	//a->updateSprite();
}

void GnomeManager::addAutomaton( QVariantMap values )
{
	Automaton* a = new Automaton( values, g );
	m_automatons.append( a );
	m_gnomesByID.insert( a->id(), m_automatons.last() );

	//a->setSpriteID( Global::sf().setAutomatonSprite( a->id(), g->m_inv->spriteID( a->automatonItem() ) ) );
	//a->updateSprite();
}

void GnomeManager::addGnome( QVariantMap values )
{
	GnomeFactory gf( g );
	Gnome* gn( gf.createGnome( values ) );
	m_gnomes.push_back( gn );
	m_gnomesByID.insert( gn->id(), m_gnomes.last() );
}

void GnomeManager::addTrader( QVariantMap values )
{
	GnomeFactory gf( g );
	GnomeTrader* gt( gf.createGnomeTrader( values ) );
	m_specialGnomes.push_back( gt );
	m_gnomesByID.insert( gt->id(), m_specialGnomes.last() );
}

void GnomeManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	QElapsedTimer timer;
	timer.start();

	//create possible automaton jobs;
	createJobs();

	// Process social interactions between nearby gnomes
	processSocialInteractions( tickNumber );

	if ( m_startIndex >= m_gnomes.size() )
	{
		m_startIndex = 0;
	}
	QList<unsigned int> deadGnomes;
	QList<unsigned int> deadOrGoneSpecial;
	for ( int i = m_startIndex; i < m_gnomes.size(); ++i )
	{
		Gnome* gn = m_gnomes[i];
#ifdef CHECKTIME
		QElapsedTimer timer2;
		timer2.start();

		CreatureTickResult tr = g->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );

		auto elapsed = timer2.elapsed();
		if ( elapsed > 100 )
		{
			qDebug() << g->name() << "just needed" << elapsed << "ms for tick";
			Global::cfg->set( "Pause", true );
			return;
		}
#else
		CreatureTickResult tr = gn->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
#endif
		m_startIndex = i + 1;
		switch ( tr )
		{
			case CreatureTickResult::DEAD:
				deadGnomes.append( gn->id() );
				break;
			case CreatureTickResult::OK:
				break;
			case CreatureTickResult::JOBCHANGED:
				emit signalGnomeActivity( gn->id(), gn->getActivity() );
				break;
			case CreatureTickResult::TODESTROY:
				break;
			case CreatureTickResult::NOFLOOR:
				break;
			case CreatureTickResult::LEFTMAP:
				break;
		}

		if ( timer.elapsed() > 5 )
		{
			break;
		}
	}

	for ( auto& gnome : m_specialGnomes )
	{
		CreatureTickResult tr = gnome->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
		if ( tr == CreatureTickResult::DEAD || tr == CreatureTickResult::LEFTMAP )
		{
			deadOrGoneSpecial.append( gnome->id() );
		}
	}

	for ( auto& automaton : m_automatons )
	{
		CreatureTickResult tr = automaton->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
		switch ( tr )
		{
			case CreatureTickResult::NOFUEL:
				break;
			case CreatureTickResult::NOCORE:
				break;
		}
	}

	if ( deadGnomes.size() )
	{
		for ( auto gid : deadGnomes )
		{
			for ( int i = 0; i < m_gnomes.size(); ++i )
			{
				if ( gid == m_gnomes[i]->id() )
				{
					Gnome* dg = m_gnomes[i];
					m_deadGnomes.append( dg );
					m_gnomesByID.insert( dg->id(), m_deadGnomes.last() );
					m_gnomes.removeAt( i );
					g->mil()->removeGnome( gid );
					emit signalGnomeDeath( dg->id() );
					break;
				}
			}
		}
	}
	if ( deadOrGoneSpecial.size() )
	{
		for ( auto gid : deadOrGoneSpecial )
		{
			for ( int i = 0; i < m_specialGnomes.size(); ++i )
			{
				if ( gid == m_specialGnomes[i]->id() )
				{
					Gnome* dg = m_specialGnomes[i];
					if ( dg->isDead() )
					{
						m_deadGnomes.append( dg );
						m_gnomesByID.insert( dg->id(), m_deadGnomes.last() );
					}
					else
					{
						m_gnomesByID.remove( dg->id() );
						g->m_world->addToUpdateList( dg->getPos() );
						delete dg;
					}
					m_specialGnomes.removeAt( i );
					break;
				}
			}
		}
	}

	if ( m_deadGnomes.size() )
	{
		for ( int i = 0; i < m_deadGnomes.size(); ++i )
		{
			Gnome* dg = m_deadGnomes[i];
			if ( dg->expires() < GameState::tick )
			{
				m_gnomesByID.remove( dg->id() );
				g->m_world->addToUpdateList( dg->getPos() );
				m_deadGnomes.removeAt( i );
				delete dg;
				break;
			}
		}
	}
}

void GnomeManager::forceMoveGnomes( Position from, Position to )
{
	for ( auto& gn : m_gnomes )
	{
		// check gnome position
		if ( gn->getPos().toInt() == from.toInt() )
		{
			//qDebug() << "force move gnome from " << from.toString() << " to " << to.toString();
			// move gnome
			gn->forceMove( to );
			// abort job if he has one
			gn->setJobAborted( "GnomeManager" );
		}
	}
}

QList<Gnome*> GnomeManager::gnomesAtPosition( Position pos )
{
	QList<Gnome*> out;
	unsigned int tileID = pos.toInt();
	auto& posMap = g->w()->creaturePositions();
	if ( posMap.contains( tileID ) )
	{
		for ( auto id : posMap[tileID] )
		{
			if ( m_gnomesByID.contains( id ) )
			{
				Gnome* gn = m_gnomesByID[id];
				if ( !gn->goneOffMap() )
				{
					out.push_back( gn );
				}
			}
		}
	}
	// Dead gnomes aren't in the position map, scan the small dead list
	for ( int i = 0; i < m_deadGnomes.size(); ++i )
	{
		if ( m_deadGnomes[i]->getPos() == pos )
		{
			out.push_back( m_deadGnomes[i] );
		}
	}
	return out;
}

QList<Gnome*> GnomeManager::deadGnomesAtPosition( Position pos )
{
	QList<Gnome*> out;
	for ( int i = 0; i < m_deadGnomes.size(); ++i )
	{
		if ( m_deadGnomes[i]->getPos() == pos )
		{
			out.push_back( m_deadGnomes[i] );
		}
	}
	return out;
}

Gnome* GnomeManager::gnome( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID];
	}
	return nullptr;
}

GnomeTrader* GnomeManager::trader( unsigned int traderID )
{
	if ( m_gnomesByID.contains( traderID ) )
	{
		for ( int i = 0; i < m_specialGnomes.size(); ++i )
		{
			if ( m_specialGnomes[i]->id() == traderID )
			{
				return dynamic_cast<GnomeTrader*>( m_specialGnomes[i] );
			}
		}
	}
	return nullptr;
}

Automaton* GnomeManager::automaton( unsigned int automatonID )
{
	if ( m_gnomesByID.contains( automatonID ) )
	{
		return dynamic_cast<Automaton*>( m_gnomesByID[automatonID] );
	}
	return nullptr;
}

QList<Gnome*> GnomeManager::gnomesSorted()
{
	QList<Gnome*> out = gnomes();
	std::sort( out.begin(), out.end(), CreatureCompare() );
	return out;
}

void GnomeManager::saveProfessions()
{
	QVariantList pl;
	for ( auto key : m_profs.keys() )
	{
		QVariantMap pm;
		pm.insert( "Name", key );
		pm.insert( "Skills", m_profs[key] );
		pl.append( pm );
	}

	QJsonDocument sd = QJsonDocument::fromVariant( pl );
	IO::saveFile( IO::getDataFolder() + "/settings/profs.json", sd );
}

void GnomeManager::loadProfessions()
{
	QJsonDocument sd;
	if ( !IO::loadFile( IO::getDataFolder() + "/settings/profs.json", sd ) )
	{
		// if it doesn't exist get from /content/JSON
		if ( IO::loadFile( Global::cfg->get( "dataPath" ).toString() + "/JSON/profs.json", sd ) )
		{
			IO::saveFile( IO::getDataFolder() + "/settings/profs.json", sd );
		}
		else
		{
			qDebug() << "Unable to find profession config!";
			return;
		}
	}
	auto profList = sd.toVariant().toList();

	m_profs.clear();

	for ( auto vprof : profList )
	{
		QString name       = vprof.toMap().value( "Name" ).toString();
		QStringList skills = vprof.toMap().value( "Skills" ).toStringList();
		m_profs.insert( name, skills );
	}

	if ( !m_profs.contains( "Gnomad" ) )
	{
		QStringList skills = DB::ids( "Skills" );
		m_profs.insert( "Gnomad", skills );
		saveProfessions();
	}
}

QStringList GnomeManager::professions()
{
	return m_profs.keys();
}

QStringList GnomeManager::professionSkills( QString profession )
{
	if ( m_profs.contains( profession ) )
	{
		return m_profs.value( profession );
	}
	return QStringList();
}

QString GnomeManager::addProfession()
{
	QString name = "NewProfession";
	
	if( !m_profs.contains( name ) )
	{
		m_profs.insert( name, QStringList() );
		saveProfessions();
		return name;
	}
	int suffixNumber = 1;

	while( m_profs.contains( name + QString::number( suffixNumber ) ) )
	{
		++suffixNumber;
	}
	name += QString::number( suffixNumber );
	m_profs.insert( name, QStringList() );
	saveProfessions();
	return name;
}

void GnomeManager::addProfession( QString name, QStringList skills )
{
	if ( !m_profs.contains( name ) )
	{
		m_profs.insert( name, skills );
		saveProfessions();
	}
}

void GnomeManager::removeProfession( QString name )
{
	if ( name == "Gnomad" )
		return;

	m_profs.remove( name );
	saveProfessions();
}

void GnomeManager::modifyProfession( QString name, QString newName, QStringList skills )
{
	if ( name == "Gnomad" )
		return;

	m_profs.remove( name );

	if ( m_profs.contains( newName ) )
	{
		m_profs.insert( name, skills );
	}
	else
	{
		m_profs.insert( newName, skills );
	}

	saveProfessions();
}

bool GnomeManager::gnomeCanReach( unsigned int gnomeID, Position pos )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return g->m_world->regionMap().checkConnectedRegions( m_gnomesByID[gnomeID]->getPos(), pos );
	}
	return false;
}

void GnomeManager::createJobs()
{
	for ( auto a : m_automatons )
	{
		if ( a->maintenanceJobID() == 0 )
		{
			// has core
			if ( a->coreItem() )
			{
				// remove core
				if ( a->uninstallFlag() )
				{
					getUninstallJob( a );
				}
				else if ( a->getRefuelFlag() && a->getFuelLevel() <= 0 )
				{
					getRefuelJob( a );
				}
			}
			else
			{
				// no core but core type is set, install core
				if ( !a->coreType().isEmpty() )
				{
					getInstallJob( a );
				}
			}
		}
	}
}

void GnomeManager::getRefuelJob( Automaton* a )
{
	auto jobID = g->jm()->addJob( "Refuel", a->getPos(), 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setAutomaton( a->id() );
		job->setRequiredSkill( "Machining" );
		job->addPossibleWorkPosition( a->getPos() );
		job->addRequiredItem( 1, "RawCoal", "any", {} );
		a->setMaintenanceJob( job );
	}
}

void GnomeManager::getInstallJob( Automaton* a )
{
	auto jobID = g->jm()->addJob( "Install", a->getPos(), 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setAutomaton( a->id() );
		job->setRequiredSkill( "Machining" );
		job->addPossibleWorkPosition( a->getPos() );
		job->addRequiredItem( 1, a->coreType(), "any", {} );
		a->setMaintenanceJob( job );
	}
}

void GnomeManager::getUninstallJob( Automaton* a )
{
	auto jobID = g->jm()->addJob( "Uninstall", a->getPos(), 0, true );
	auto job = g->jm()->getJob( jobID );
	if( job )
	{
		job->setAutomaton( a->id() );
		job->setRequiredSkill( "Machining" );
		job->addPossibleWorkPosition( a->getPos() );
		a->setMaintenanceJob( job );
	}
}

void GnomeManager::setInMission( unsigned int gnomeID, unsigned int missionID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		m_gnomesByID[gnomeID]->setMission( missionID );
		m_gnomesByID[gnomeID]->setOnMission( true );
	}
}

QString GnomeManager::name( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->name();
	}
	return "*no name*";
}

unsigned int GnomeManager::roleID( unsigned int gnomeID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->roleID();
	}
	return 0;
}
	
void GnomeManager::setRoleID( unsigned int gnomeID, unsigned int roleID )
{
	if ( m_gnomesByID.contains( gnomeID ) )
	{
		return m_gnomesByID[gnomeID]->setRole( roleID );
	}
}

int GnomeManager::numGnomes()
{
	return m_gnomes.size();
}

// =============================================================================
// Social System (Milestone 2.0b, redesigned)
// =============================================================================

int GnomeManager::opinion( unsigned int gnomeA, unsigned int gnomeB ) const
{
	if ( m_opinions.contains( gnomeA ) && m_opinions[gnomeA].contains( gnomeB ) )
	{
		return m_opinions[gnomeA][gnomeB];
	}
	return 0;
}

void GnomeManager::modifyOpinion( unsigned int gnomeA, unsigned int gnomeB, int delta )
{
	int current = opinion( gnomeA, gnomeB );
	m_opinions[gnomeA][gnomeB] = qBound( -100, current + delta, 100 );
}

QString GnomeManager::relationshipLabel( unsigned int gnomeA, unsigned int gnomeB ) const
{
	int op = opinion( gnomeA, gnomeB );
	if ( op < -30 ) return "Rival";
	if ( op < -10 ) return "Disliked";
	if ( op < 10 ) return "Neutral";
	if ( op < 30 ) return "Friendly";
	return "Close Friend";
}

QHash<unsigned int, int> GnomeManager::opinionsOf( unsigned int gnomeID ) const
{
	if ( m_opinions.contains( gnomeID ) )
		return m_opinions[gnomeID];
	return QHash<unsigned int, int>();
}

QList<GnomeManager::SocialMemory> GnomeManager::memoriesOf( unsigned int gnomeID ) const
{
	if ( m_socialMemories.contains( gnomeID ) )
		return m_socialMemories[gnomeID];
	return QList<SocialMemory>();
}

void GnomeManager::addSocialMemory( unsigned int gnomeID, unsigned int otherID, const QString& event, quint64 tick, int change )
{
	SocialMemory mem;
	mem.otherID = otherID;
	mem.event = event;
	mem.tick = tick;
	mem.opinionChange = change;

	auto& list = m_socialMemories[gnomeID];
	list.prepend( mem );

	// Keep only last 10 memories per gnome
	while ( list.size() > 10 )
		list.removeLast();
}

bool GnomeManager::hasRecentGrievance( unsigned int gnomeA, unsigned int gnomeB, quint64 currentTick ) const
{
	if ( !m_socialMemories.contains( gnomeA ) ) return false;
	quint64 threeDays = 14400 * 3; // 3 in-game days
	for ( const auto& mem : m_socialMemories[gnomeA] )
	{
		if ( mem.otherID == gnomeB && mem.opinionChange < -1 && ( currentTick - mem.tick ) < threeDays )
			return true;
	}
	return false;
}

int GnomeManager::traitCompatibility( Gnome* a, Gnome* b ) const
{
	// Weighted compatibility based on specific trait interactions
	// Returns -50 to +50
	int score = 0;

	// Optimism vs Pessimism: opposites clash
	int optA = a->trait( "Optimism" ), optB = b->trait( "Optimism" );
	if ( ( optA > 20 && optB < -20 ) || ( optA < -20 && optB > 20 ) )
		score -= 8; // strong friction
	else if ( abs( optA - optB ) < 15 )
		score += 3; // similar outlook

	// Sociability: similar levels get along
	int socA = a->trait( "Sociability" ), socB = b->trait( "Sociability" );
	if ( abs( socA - socB ) < 15 )
		score += 2;
	else if ( abs( socA - socB ) > 40 )
		score -= 3; // introvert + extrovert friction

	// Empathy: both high = strong affinity
	int empA = a->trait( "Empathy" ), empB = b->trait( "Empathy" );
	if ( empA > 15 && empB > 15 )
		score += 5;
	else if ( empA < -15 && empB < -15 )
		score += 2; // cold people tolerate each other

	// Temper: two hot-heads are volatile
	int tmpA = a->trait( "Temper" ), tmpB = b->trait( "Temper" );
	if ( tmpA > 20 && tmpB > 20 )
		score -= 4; // both volatile = sparks fly

	// Industriousness: similar work ethic
	int indA = a->trait( "Industriousness" ), indB = b->trait( "Industriousness" );
	if ( abs( indA - indB ) < 15 )
		score += 2;
	else if ( ( indA > 20 && indB < -20 ) || ( indA < -20 && indB > 20 ) )
		score -= 3; // hard worker resents lazy one

	// Curiosity: explorers bond
	int curA = a->trait( "Curiosity" ), curB = b->trait( "Curiosity" );
	if ( curA > 15 && curB > 15 )
		score += 3;

	// Creativity: artists appreciate each other
	int creA = a->trait( "Creativity" ), creB = b->trait( "Creativity" );
	if ( creA > 15 && creB > 15 )
		score += 3;

	return qBound( -50, score, 50 );
}

int GnomeManager::backstoryCompatibility( Gnome* a, Gnome* b ) const
{
	// Check if backstories share skill modifiers (similar backgrounds = affinity)
	int score = 0;

	auto getSkillGroups = []( Gnome* gn ) -> QStringList
	{
		QStringList groups;
		auto addFromBackstory = [&groups]( const QString& bsID )
		{
			if ( bsID.isEmpty() ) return;
			auto row = DB::selectRow( "Backstories", bsID );
			QString mods = row.value( "SkillModifiers" ).toString();
			for ( const auto& entry : mods.split( '|' ) )
			{
				auto parts = entry.split( ':' );
				if ( parts.size() == 2 )
				{
					// Look up the skill's group
					auto skillRow = DB::selectRow( "Skills", parts[0] );
					QString group = skillRow.value( "SkillGroup" ).toString();
					if ( !group.isEmpty() && !groups.contains( group ) )
						groups.append( group );
				}
			}
		};
		addFromBackstory( gn->childhoodBackstory() );
		addFromBackstory( gn->adulthoodBackstory() );
		return groups;
	};

	QStringList groupsA = getSkillGroups( a );
	QStringList groupsB = getSkillGroups( b );

	// Count shared skill groups
	for ( const auto& g : groupsA )
	{
		if ( groupsB.contains( g ) )
			score += 4; // shared background area = affinity
	}

	return qBound( -10, score, 15 );
}

void GnomeManager::processSocialInteractions( quint64 tickNumber )
{
	// Process once per in-game hour (600 ticks) — gives ~1-2 interactions/pair/day
	if ( tickNumber % 600 != 0 ) return;
	if ( m_gnomes.size() < 2 ) return;

	// Step 1: Opinion decay — 1 point toward 0 per in-game day (every 24 checks)
	if ( tickNumber % 14400 == 0 )
	{
		for ( auto it = m_opinions.begin(); it != m_opinions.end(); ++it )
		{
			for ( auto jt = it.value().begin(); jt != it.value().end(); ++jt )
			{
				int& op = jt.value();
				if ( op > 0 ) op--;
				else if ( op < 0 ) op++;
			}
		}

		// Clean up old social memories (older than 3 days)
		quint64 threeDays = 14400 * 3;
		for ( auto it = m_socialMemories.begin(); it != m_socialMemories.end(); ++it )
		{
			auto& list = it.value();
			for ( int i = list.size() - 1; i >= 0; --i )
			{
				if ( tickNumber - list[i].tick > threeDays )
					list.removeAt( i );
			}
		}
	}

	// Step 2: Process interactions between nearby gnomes
	for ( int i = 0; i < m_gnomes.size(); ++i )
	{
		Gnome* a = m_gnomes[i];
		if ( a->isDead() ) continue;

		for ( int j = i + 1; j < m_gnomes.size(); ++j )
		{
			Gnome* b = m_gnomes[j];
			if ( b->isDead() ) continue;

			Position posA = a->getPos();
			Position posB = b->getPos();
			int dist = abs( posA.x - posB.x ) + abs( posA.y - posB.y ) + abs( posA.z - posB.z );
			if ( dist > 5 ) continue;

			// 5% chance per eligible pair per hourly check = ~1.2 interactions/day
			if ( rand() % 100 >= 5 ) continue;

			// Calculate interaction tone from multiple factors
			int compat = traitCompatibility( a, b );
			int bsCompat = backstoryCompatibility( a, b );

			// Mood modifier: happy gnomes are more agreeable, unhappy more prickly
			int moodModA = ( a->mood() - 50 ) / 10; // -5 to +5
			int moodModB = ( b->mood() - 50 ) / 10;
			int moodBonus = ( moodModA + moodModB ) / 2;

			int tone = compat + bsCompat + moodBonus;

			// Grievance/apology check
			bool aHasGrievance = hasRecentGrievance( a->id(), b->id(), tickNumber );
			bool bHasGrievance = hasRecentGrievance( b->id(), a->id(), tickNumber );

			// Gnomes with high Empathy may apologize for past wrongs
			if ( aHasGrievance && a->trait( "Empathy" ) > 15 && rand() % 100 < 20 )
			{
				modifyOpinion( a->id(), b->id(), 2 );
				modifyOpinion( b->id(), a->id(), 3 );
				addSocialMemory( a->id(), b->id(), "Apologized", tickNumber, 2 );
				addSocialMemory( b->id(), a->id(), "Received apology", tickNumber, 3 );
				continue;
			}
			// Gnomes with high Temper may escalate past conflicts
			if ( bHasGrievance && b->trait( "Temper" ) > 20 && rand() % 100 < 15 )
			{
				modifyOpinion( a->id(), b->id(), -2 );
				modifyOpinion( b->id(), a->id(), -3 );
				addSocialMemory( a->id(), b->id(), "Was confronted", tickNumber, -2 );
				addSocialMemory( b->id(), a->id(), "Confronted", tickNumber, -3 );
				continue;
			}

			int roll = rand() % 100;

			if ( tone > 10 )
			{
				// Positive tone
				if ( roll < 60 )
				{
					modifyOpinion( a->id(), b->id(), 1 );
					modifyOpinion( b->id(), a->id(), 1 );
					addSocialMemory( a->id(), b->id(), "Had a good chat", tickNumber, 1 );
					addSocialMemory( b->id(), a->id(), "Had a good chat", tickNumber, 1 );
				}
				else if ( roll < 80 )
				{
					modifyOpinion( a->id(), b->id(), 2 );
					modifyOpinion( b->id(), a->id(), 2 );
					addSocialMemory( a->id(), b->id(), "Shared a laugh", tickNumber, 2 );
					addSocialMemory( b->id(), a->id(), "Shared a laugh", tickNumber, 2 );
				}
				// else: 20% no meaningful interaction
			}
			else if ( tone < -10 )
			{
				// Negative tone
				if ( roll < 50 )
				{
					modifyOpinion( a->id(), b->id(), -1 );
					modifyOpinion( b->id(), a->id(), -1 );
					addSocialMemory( a->id(), b->id(), "Awkward exchange", tickNumber, -1 );
					addSocialMemory( b->id(), a->id(), "Awkward exchange", tickNumber, -1 );
				}
				else if ( roll < 75 )
				{
					modifyOpinion( a->id(), b->id(), -2 );
					modifyOpinion( b->id(), a->id(), -2 );
					addSocialMemory( a->id(), b->id(), "Argued", tickNumber, -2 );
					addSocialMemory( b->id(), a->id(), "Argued", tickNumber, -2 );
				}
				else if ( roll < 90 )
				{
					// Insult: asymmetric
					modifyOpinion( b->id(), a->id(), -3 );
					addSocialMemory( a->id(), b->id(), "Said something rude", tickNumber, 0 );
					addSocialMemory( b->id(), a->id(), "Was insulted", tickNumber, -3 );
				}
				// else: 10% cold shoulder, no change
			}
			// else: neutral tone = no opinion change (the key fix)
		}
	}
}