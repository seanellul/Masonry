#include "testcommandserver.h"

#include "../base/global.h"
#include "../gui/eventconnector.h"
#include "../gui/imguibridge.h"
#include "../gui/mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

#include <iostream>
#include <string>

// ============================================================================
// StdinReader — runs on a worker thread, reads lines from stdin
// ============================================================================

StdinReader::StdinReader( QObject* parent )
	: QObject( parent )
{
}

void StdinReader::run()
{
	std::string line;
	while ( std::getline( std::cin, line ) )
	{
		if ( line.empty() )
			continue;

		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson( QByteArray::fromStdString( line ), &error );
		if ( error.error != QJsonParseError::NoError )
		{
			// Send error response directly
			QJsonObject errResp;
			errResp["status"] = "error";
			errResp["message"] = QString( "JSON parse error: %1" ).arg( error.errorString() );
			QByteArray json = QJsonDocument( errResp ).toJson( QJsonDocument::Compact );
			fprintf( stdout, "%s\n", json.constData() );
			fflush( stdout );
			continue;
		}

		if ( doc.isObject() )
		{
			emit commandReceived( doc.object() );
		}
	}

	emit finished();
}

// ============================================================================
// TestCommandServer — dispatches commands on the main thread
// ============================================================================

TestCommandServer::TestCommandServer( ImGuiBridge* bridge, MainWindow* window, QObject* parent )
	: QObject( parent )
	, m_bridge( bridge )
	, m_window( window )
{
}

TestCommandServer::~TestCommandServer()
{
	m_readerThread.quit();
	m_readerThread.wait();
}

void TestCommandServer::start()
{
	auto* ec = Global::eventConnector;
	connect( ec, &EventConnector::signalHeartbeat, this, &TestCommandServer::onHeartbeat, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalLoadGameDone, this, &TestCommandServer::onLoadGameDone, Qt::QueuedConnection );

	StdinReader* reader = new StdinReader();
	reader->moveToThread( &m_readerThread );

	connect( &m_readerThread, &QThread::started, reader, &StdinReader::run );
	connect( reader, &StdinReader::commandReceived, this, &TestCommandServer::onCommand, Qt::QueuedConnection );
	connect( reader, &StdinReader::finished, reader, &QObject::deleteLater );
	connect( reader, &StdinReader::finished, &m_readerThread, &QThread::quit );

	m_readerThread.start();

	// Signal that the server is ready
	QJsonObject ready;
	ready["status"] = "ready";
	ready["message"] = "Test command server started. Send JSON commands on stdin.";
	sendResponse( ready );
}

void TestCommandServer::onCommand( const QJsonObject& cmd )
{
	QString command = cmd["cmd"].toString();
	QJsonObject response;

	if ( command == "quit" )
	{
		response["status"] = "ok";
		sendResponse( response );
		QTimer::singleShot( 100, qApp, &QApplication::quit );
		return;
	}
	else if ( command == "get_state" )
	{
		response = handleGetState();
	}
	else if ( command == "screenshot" )
	{
		response = handleScreenshot( cmd );
	}
	else if ( command == "advance_ticks" )
	{
		response = handleAdvanceTicks( cmd );
		// This is async — response will be sent from onHeartbeat
		if ( response.contains( "async" ) )
			return;
	}
	else if ( command == "load_save" )
	{
		response = handleLoadSave( cmd );
		if ( response.contains( "async" ) )
			return;
	}
	else if ( command == "set_paused" )
	{
		response = handleSetPaused( cmd );
	}
	else if ( command == "set_game_speed" )
	{
		response = handleSetGameSpeed( cmd );
	}
	else if ( command == "build" )
	{
		response = handleBuild( cmd );
	}
	else if ( command == "spawn_creature" )
	{
		response = handleSpawnCreature( cmd );
	}
	else if ( command == "get_metrics" )
	{
		response = handleGetMetrics();
	}
	else if ( command == "save_game" )
	{
		m_bridge->cmdSaveGame();
		response["status"] = "ok";
	}
	else
	{
		response["status"] = "error";
		response["message"] = QString( "Unknown command: %1" ).arg( command );
	}

	sendResponse( response );
}

void TestCommandServer::sendResponse( const QJsonObject& response )
{
	QJsonDocument doc( response );
	QByteArray json = doc.toJson( QJsonDocument::Compact );
	fprintf( stdout, "%s\n", json.constData() );
	fflush( stdout );
}

QJsonObject TestCommandServer::handleGetState()
{
	QJsonObject response;
	response["status"] = "ok";

	QJsonObject data;

	// Time
	QJsonObject time;
	time["minute"] = m_bridge->minute;
	time["hour"] = m_bridge->hour;
	time["day"] = m_bridge->day;
	time["year"] = m_bridge->year;
	time["season"] = m_bridge->season;
	time["sun_status"] = m_bridge->sunStatus;
	data["time"] = time;

	// Kingdom
	QJsonObject kingdom;
	kingdom["name"] = m_bridge->kingdomName;
	kingdom["info1"] = m_bridge->kingdomInfo1;
	kingdom["info2"] = m_bridge->kingdomInfo2;
	kingdom["info3"] = m_bridge->kingdomInfo3;
	data["kingdom"] = kingdom;

	// Game state
	data["paused"] = m_bridge->paused;
	data["game_speed"] = static_cast<int>( m_bridge->gameSpeed );
	data["view_level"] = m_bridge->viewLevel;

	// App state
	data["app_state"] = static_cast<int>( m_bridge->appState );

	response["data"] = data;
	return response;
}

QJsonObject TestCommandServer::handleScreenshot( const QJsonObject& cmd )
{
	QString path = cmd["path"].toString( "/tmp/ingnomia_screenshot.png" );
	m_window->takeScreenshot( path );

	QJsonObject response;
	response["status"] = "ok";
	response["screenshot"] = path;
	return response;
}

QJsonObject TestCommandServer::handleAdvanceTicks( const QJsonObject& cmd )
{
	int count = cmd["count"].toInt( 100 );
	if ( count <= 0 )
	{
		QJsonObject response;
		response["status"] = "error";
		response["message"] = "count must be > 0";
		return response;
	}

	m_targetTicks = count;
	m_currentTicks = 0;
	m_waitingForTicks = true;

	// Unpause
	m_bridge->cmdSetPaused( false );

	// Return async marker — actual response sent from onHeartbeat
	QJsonObject response;
	response["async"] = true;
	return response;
}

QJsonObject TestCommandServer::handleLoadSave( const QJsonObject& cmd )
{
	QString path = cmd["path"].toString();
	if ( path.isEmpty() )
	{
		QJsonObject response;
		response["status"] = "error";
		response["message"] = "path is required";
		return response;
	}

	m_waitingForLoad = true;

	QMetaObject::invokeMethod( Global::eventConnector, "onLoadGame", Qt::QueuedConnection,
		Q_ARG( QString, path ) );

	// Return async marker — actual response sent from onLoadGameDone
	QJsonObject response;
	response["async"] = true;
	return response;
}

QJsonObject TestCommandServer::handleSetPaused( const QJsonObject& cmd )
{
	bool value = cmd["value"].toBool();
	m_bridge->cmdSetPaused( value );

	QJsonObject response;
	response["status"] = "ok";
	response["paused"] = value;
	return response;
}

QJsonObject TestCommandServer::handleSetGameSpeed( const QJsonObject& cmd )
{
	QString speed = cmd["speed"].toString( "Normal" );
	GameSpeed gs = GameSpeed::Normal;
	if ( speed == "Fast" ) gs = GameSpeed::Fast;

	m_bridge->cmdSetGameSpeed( gs );

	QJsonObject response;
	response["status"] = "ok";
	response["speed"] = speed;
	return response;
}

QJsonObject TestCommandServer::handleBuild( const QJsonObject& cmd )
{
	QString type = cmd["type"].toString();
	QString param = cmd["param"].toString();
	QString item = cmd["item"].toString();
	QStringList mats;
	for ( auto val : cmd["materials"].toArray() )
	{
		mats.append( val.toString() );
	}

	BuildItemType buildType = BuildItemType::Workshop;
	if ( type == "Wall" ) buildType = BuildItemType::Terrain;
	else if ( type == "Floor" ) buildType = BuildItemType::Terrain;
	else if ( type == "Fence" ) buildType = BuildItemType::Terrain;

	m_bridge->cmdBuild( buildType, param, item, mats );

	QJsonObject response;
	response["status"] = "ok";
	return response;
}

QJsonObject TestCommandServer::handleSpawnCreature( const QJsonObject& cmd )
{
	QString type = cmd["type"].toString( "Gnome" );
	m_bridge->cmdSpawnCreature( type );

	QJsonObject response;
	response["status"] = "ok";
	response["type"] = type;
	return response;
}

QJsonObject TestCommandServer::handleGetMetrics()
{
	QJsonObject response;
	response["status"] = "ok";

	QJsonObject data;
	data["paused"] = m_bridge->paused;

	QJsonObject time;
	time["minute"] = m_bridge->minute;
	time["hour"] = m_bridge->hour;
	time["day"] = m_bridge->day;
	time["year"] = m_bridge->year;
	time["season"] = m_bridge->season;
	data["time"] = time;

	QJsonObject kingdom;
	kingdom["name"] = m_bridge->kingdomName;
	kingdom["info1"] = m_bridge->kingdomInfo1;
	kingdom["info2"] = m_bridge->kingdomInfo2;
	kingdom["info3"] = m_bridge->kingdomInfo3;
	data["kingdom"] = kingdom;

	response["data"] = data;
	return response;
}

void TestCommandServer::onHeartbeat( int value )
{
	Q_UNUSED( value );

	if ( !m_waitingForTicks )
		return;

	m_currentTicks++;

	if ( m_currentTicks >= m_targetTicks )
	{
		m_waitingForTicks = false;

		// Pause
		m_bridge->cmdSetPaused( true );

		QJsonObject response;
		response["status"] = "ok";
		response["ticks_completed"] = m_currentTicks;
		sendResponse( response );
	}
}

void TestCommandServer::onLoadGameDone( bool success )
{
	if ( !m_waitingForLoad )
		return;

	m_waitingForLoad = false;

	QJsonObject response;
	if ( success )
	{
		response["status"] = "ok";
		response["save_loaded"] = true;
	}
	else
	{
		response["status"] = "error";
		response["message"] = "Failed to load save game";
		response["save_loaded"] = false;
	}
	sendResponse( response );
}
