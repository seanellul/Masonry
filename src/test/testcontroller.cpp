#include "testcontroller.h"

#include "../base/global.h"
#include "../gui/eventconnector.h"
#include "../gui/mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

TestController::TestController( const TestConfig& config, MainWindow* window, QObject* parent )
	: QObject( parent )
	, m_config( config )
	, m_window( window )
{
}

void TestController::start()
{
	m_elapsed.start();

	auto* ec = Global::eventConnector;

	// Connect to signals we need
	connect( ec, &EventConnector::signalLoadGameDone, this, &TestController::onLoadGameDone, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalHeartbeat, this, &TestController::onHeartbeat, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalTimeAndDate, this, &TestController::onTimeAndDate, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalKingdomInfo, this, &TestController::onKingdomInfo, Qt::QueuedConnection );

	if ( !m_config.loadSavePath.isEmpty() )
	{
		qDebug() << "[TestController] Loading save:" << m_config.loadSavePath;
		// Trigger save load via EventConnector (same path ImGuiBridge uses)
		QMetaObject::invokeMethod( ec, "onLoadGame", Qt::QueuedConnection,
			Q_ARG( QString, m_config.loadSavePath ) );
	}
	else
	{
		qDebug() << "[TestController] No save path specified, waiting...";
	}
}

void TestController::onLoadGameDone( bool success )
{
	m_saveLoaded = success;
	if ( !success )
	{
		qWarning() << "[TestController] Failed to load save game";
		writeMetricsAndExit();
		return;
	}

	qDebug() << "[TestController] Save loaded successfully";

	if ( m_config.runTicks > 0 )
	{
		qDebug() << "[TestController] Unpausing game, will run" << m_config.runTicks << "ticks";
		// Unpause the game via EventConnector
		QMetaObject::invokeMethod( Global::eventConnector, "onSetPause", Qt::QueuedConnection,
			Q_ARG( bool, false ) );
	}
	else if ( !m_config.screenshotPath.isEmpty() )
	{
		// No ticks to run, just wait for frames and screenshot
		startFrameCountdown();
	}
	else
	{
		// Nothing to do, just write metrics and exit
		writeMetricsAndExit();
	}
}

void TestController::startFrameCountdown()
{
	m_framesRendered = 0;
	QTimer* frameTimer = new QTimer( this );
	connect( frameTimer, &QTimer::timeout, this, [this, frameTimer]()
	{
		m_framesRendered++;
		if ( m_framesRendered >= m_config.screenshotDelay )
		{
			frameTimer->stop();
			frameTimer->deleteLater();
			takeScreenshotAndMaybeExit();
		}
	} );
	frameTimer->start( 16 ); // ~60fps timing
}

void TestController::onHeartbeat( int value )
{
	Q_UNUSED( value );

	if ( !m_saveLoaded || m_config.runTicks <= 0 )
		return;

	m_ticksElapsed++;

	if ( m_ticksElapsed >= m_config.runTicks )
	{
		// Prevent further tick processing
		m_config.runTicks = 0;

		qDebug() << "[TestController] Completed" << m_ticksElapsed << "ticks";

		// Pause the game
		QMetaObject::invokeMethod( Global::eventConnector, "onSetPause", Qt::QueuedConnection,
			Q_ARG( bool, true ) );

		if ( !m_config.screenshotPath.isEmpty() && !m_screenshotTaken )
		{
			startFrameCountdown();
		}
		else
		{
			writeMetricsAndExit();
		}
	}
}

void TestController::onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus )
{
	m_minute = minute;
	m_hour = hour;
	m_day = day;
	m_season = season;
	m_year = year;
	m_sunStatus = sunStatus;
}

void TestController::onKingdomInfo( QString name, QString info1, QString info2, QString info3 )
{
	m_kingdomName = name;
	m_kingdomInfo1 = info1;
	m_kingdomInfo2 = info2;
	m_kingdomInfo3 = info3;
}

void TestController::takeScreenshotAndMaybeExit()
{
	if ( m_screenshotTaken )
		return;

	m_screenshotTaken = true;
	qDebug() << "[TestController] Taking screenshot:" << m_config.screenshotPath;
	m_window->takeScreenshot( m_config.screenshotPath );

	writeMetricsAndExit();
}

void TestController::writeMetricsAndExit()
{
	writeMetrics();

	qDebug() << "[TestController] Test complete, exiting";
	QTimer::singleShot( 100, qApp, &QApplication::quit );
}

void TestController::writeMetrics()
{
	qint64 totalElapsed = m_elapsed.elapsed();

	QJsonObject metrics;
	metrics["save_loaded"] = m_saveLoaded;
	metrics["ticks_run"] = m_ticksElapsed;

	QJsonObject gameTime;
	gameTime["year"] = m_year;
	gameTime["season"] = m_season;
	gameTime["day"] = m_day;
	gameTime["hour"] = m_hour;
	gameTime["minute"] = m_minute;
	gameTime["sun_status"] = m_sunStatus;
	metrics["game_time"] = gameTime;

	QJsonObject kingdom;
	kingdom["name"] = m_kingdomName;
	kingdom["info1"] = m_kingdomInfo1;
	kingdom["info2"] = m_kingdomInfo2;
	kingdom["info3"] = m_kingdomInfo3;
	metrics["kingdom"] = kingdom;

	QJsonObject performance;
	performance["total_elapsed_ms"] = totalElapsed;
	if ( m_ticksElapsed > 0 )
	{
		performance["avg_tick_time_ms"] = static_cast<double>( totalElapsed ) / m_ticksElapsed;
	}
	metrics["performance"] = performance;

	if ( !m_config.screenshotPath.isEmpty() && m_screenshotTaken )
	{
		metrics["screenshot"] = m_config.screenshotPath;
	}

	QJsonDocument doc( metrics );
	QByteArray json = doc.toJson( QJsonDocument::Indented );

	if ( m_config.metricsOutPath.isEmpty() )
	{
		// Write to stdout
		fprintf( stdout, "%s\n", json.constData() );
		fflush( stdout );
	}
	else
	{
		QFile file( m_config.metricsOutPath );
		if ( file.open( QIODevice::WriteOnly ) )
		{
			file.write( json );
			file.close();
			qDebug() << "[TestController] Metrics written to:" << m_config.metricsOutPath;
		}
		else
		{
			qWarning() << "[TestController] Failed to write metrics to:" << m_config.metricsOutPath;
			// Fallback to stdout
			fprintf( stdout, "%s\n", json.constData() );
			fflush( stdout );
		}
	}
}
