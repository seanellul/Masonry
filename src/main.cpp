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
#include "base/config.h"
#include "base/db.h"
#include "base/io.h"
#include "base/crashhandler.h"
#include "base/global.h"

#include "game/gamemanager.h"

#include "gui/mainwindow.h"
#include "gui/strings.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#ifndef __APPLE__
#include <QDesktopWidget>
#endif
#include <QDir>
#include <QFileIconProvider>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QTimer>
#include <QWindow>
#include <QtWidgets/QApplication>

#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#include "test/testcontroller.h"
#include "test/testcommandserver.h"
#include "version.h"

QTextStream* out = 0;
bool verbose     = false;

void clearLog()
{
	QString folder   = IO::getDataFolder();
	bool ok          = true;
	QString fileName = "log.txt";
	if ( QDir( folder ).exists() )
	{
		fileName = folder + "/" + fileName;
	}

	QFile file( fileName );
	file.open( QIODevice::WriteOnly );
	file.close();
}

QPointer<QFile> openLog()
{
	QString folder   = IO::getDataFolder();
	bool ok          = true;
	QString fileName = "log.txt";
	if ( QDir( folder ).exists() )
	{
		fileName = folder + "/" + fileName;
	}

	QPointer<QFile> outFile(new QFile( fileName ));
	if(outFile->open( QIODevice::WriteOnly | QIODevice::Append ))
	{
		return outFile;
	}
	else
	{
		return nullptr;
	}
}

void logOutput( QtMsgType type, const QMessageLogContext& context, const QString& message )
{
	if ( message.startsWith( "libpng warning:" ) )
		return;

	QString filedate  = QDateTime::currentDateTime().toString( "yyyy.MM.dd hh:mm:ss:zzz" );
#ifdef _WIN32
	if ( IsDebuggerPresent() )
#else
	if (verbose)
#endif // _WIN32
	{
		QString debugdate = QDateTime::currentDateTime().toString( "hh:mm:ss:zzz" );

		switch ( type )
		{
			case QtDebugMsg:
				debugdate += " [D]";
				break;
			case QtInfoMsg:
				debugdate += " [I]";
				break;
			case QtWarningMsg:
				debugdate += " [W]";
				break;
			case QtCriticalMsg:
				debugdate += " [C]";
				break;
			case QtFatalMsg:
				debugdate += " [F]";
				break;
		}
		QString text    = debugdate + " " + message + "\n";
		std::string str = text.toStdString();

#ifdef _WIN32
		OutputDebugStringA( str.c_str() );
#else
		std::cerr << str;
#endif
	}
	static QPointer<QFile> outFile = openLog();
	static std::mutex guard;

	if ( outFile )
	{
		std::lock_guard<std::mutex> lock( guard );
		QTextStream ts( outFile );
		ts << filedate << " " << message << endl;
	}
}

int main( int argc, char* argv[] )
{
	setupCrashHandler();
	clearLog();
	qInstallMessageHandler( &logOutput );
	qInfo() << PROJECT_NAME << "version" << PROJECT_VERSION;
#ifdef GIT_REPO
	qInfo() << "Built from" << GIT_REPO << GIT_REF << "(" << GIT_SHA << ")"
			<< "build" << BUILD_ID;
#endif // GIT_REPO

	// Disable use of ANGLE, as it supports OpenGL 3.x at most
	QCoreApplication::setAttribute( Qt::AA_UseDesktopOpenGL );
	// Require use of shared base context, so OpenGL context won't get invalidated on fullscreen toggles etc.
	QCoreApplication::setAttribute( Qt::AA_ShareOpenGLContexts );
	// Enable correct render surface scaling with HDPI setups.
	QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
	// Enable fractional DPI support (e.g. 150%)
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy( Qt::HighDpiScaleFactorRoundingPolicy::PassThrough );

	QApplication a( argc, argv );
	QCoreApplication::addLibraryPath( QCoreApplication::applicationDirPath() );
	QCoreApplication::setOrganizationDomain( PROJECT_HOMEPAGE_URL );
	QCoreApplication::setOrganizationName( "Roest" );
	QCoreApplication::setApplicationName( PROJECT_NAME );
	QCoreApplication::setApplicationVersion( PROJECT_VERSION );

	Global::cfg = new Config;

	if ( !Global::cfg->valid() )
	{
		qDebug() << "Failed to init Config.";
		abort();
	}

	DB::init();
	DB::initStructs();

	if ( !S::gi().init() )
	{
		qDebug() << "Failed to init translation.";
		abort();
	}

	Global::cfg->set( "CurrentVersion", PROJECT_VERSION );

	QStringList args = a.arguments();
	TestConfig testConfig;

	for ( int i = 1; i < args.size(); ++i )
	{
		if ( args.at( i ) == "-h" || args.at( i ) == "?" )
		{
			qDebug() << "Command line options:";
			qDebug() << "-h : displays this message";
			qDebug() << "-v : toggles verbose mode, warning: this will spam your console with messages";
			qDebug() << "--test-mode       : enable test mode (suppress audio, enable test controller)";
			qDebug() << "--load-save <path> : load a specific save game on startup";
			qDebug() << "--screenshot <path> : take a screenshot and save to path";
			qDebug() << "--screenshot-delay <frames> : frames to wait before screenshot (default: 60)";
			qDebug() << "--run-ticks <N>   : advance N game ticks, then exit";
			qDebug() << "--metrics-out <path> : write metrics JSON to file (default: stdout)";
			qDebug() << "---";
		}
		if ( args.at( i ) == "-v" )
		{
			verbose = true;
		}
		if ( args.at( i ) == "-ds" )
		{
			Global::debugSound = true;
		}
		if ( args.at( i ) == "--test-mode" )
		{
			testConfig.enabled = true;
			Global::testMode = true;
			Global::debugSound = true; // suppress audio in test mode
		}
		if ( args.at( i ) == "--load-save" && i + 1 < args.size() )
		{
			testConfig.loadSavePath = args.at( ++i );
			testConfig.enabled = true;
			Global::testMode = true;
		}
		if ( args.at( i ) == "--screenshot" && i + 1 < args.size() )
		{
			testConfig.screenshotPath = args.at( ++i );
			testConfig.enabled = true;
			Global::testMode = true;
		}
		if ( args.at( i ) == "--screenshot-delay" && i + 1 < args.size() )
		{
			testConfig.screenshotDelay = args.at( ++i ).toInt();
		}
		if ( args.at( i ) == "--run-ticks" && i + 1 < args.size() )
		{
			testConfig.runTicks = args.at( ++i ).toInt();
			testConfig.enabled = true;
			Global::testMode = true;
		}
		if ( args.at( i ) == "--metrics-out" && i + 1 < args.size() )
		{
			testConfig.metricsOutPath = args.at( ++i );
		}
	}

	int width  = qMax( 1200, Global::cfg->get( "WindowWidth" ).toInt() );
	int height = qMax( 675, Global::cfg->get( "WindowHeight" ).toInt() );

	auto defaultFormat = QSurfaceFormat::defaultFormat();
	defaultFormat.setRenderableType( QSurfaceFormat::OpenGL );
	defaultFormat.setSwapBehavior( QSurfaceFormat::TripleBuffer );
	defaultFormat.setColorSpace( QSurfaceFormat::sRGBColorSpace );
	defaultFormat.setDepthBufferSize( 16 );
	// 0 = unthrottled, 1 = vysnc full FPS, 2 = vsync half FPS
	defaultFormat.setSwapInterval( 0 );
	defaultFormat.setVersion( 4, 1 );
	defaultFormat.setRenderableType( QSurfaceFormat::OpenGL );
	defaultFormat.setProfile( QSurfaceFormat::CoreProfile );
#ifndef __APPLE__
	defaultFormat.setOption( QSurfaceFormat::DebugContext );
#endif
	QSurfaceFormat::setDefaultFormat( defaultFormat );

	GameManager* gm = new GameManager;
	QThread gameThread;
	gameThread.start();
	gm->moveToThread( &gameThread );


	//MainWindow w;
	MainWindow w;
	
	w.setIcon( QIcon( QCoreApplication::applicationDirPath() + "/content/icon.png" ) );
	w.resize( width, height );
	w.setPosition( Global::cfg->get( "WindowPosX" ).toInt(), Global::cfg->get( "WindowPosY" ).toInt() );
	w.show();
	if( Global::cfg->get( "fullscreen" ).toBool() )
	{
		w.onFullScreen( true );
	}

	// Create test controller if test flags were provided
	TestController* testController = nullptr;
	TestCommandServer* testCommandServer = nullptr;
	if ( testConfig.enabled )
	{
		qDebug() << "[Test] Test mode enabled";
		if ( Global::testMode && testConfig.screenshotPath.isEmpty() )
		{
			w.showMinimized();
		}
		else if ( Global::testMode )
		{
			// Keep window visible for screenshots — macOS won't run
			// paintGL on minimized windows, resulting in blank captures.
			// showNormal() re-exposes the surface if show() already ran.
			w.showNormal();
			w.raise();
			w.requestActivate();
		}

		// If we have specific test actions (load-save, screenshot, run-ticks), use TestController
		if ( !testConfig.loadSavePath.isEmpty() || !testConfig.screenshotPath.isEmpty() || testConfig.runTicks > 0 )
		{
			testController = new TestController( testConfig, &w );
			// Delay start until event loop is running and GameManager is ready
			QTimer::singleShot( 500, testController, &TestController::start );
		}

		// Always start the command server in test mode for interactive control
		// It starts after GL init so ImGuiBridge is available
		QTimer::singleShot( 1000, [&w, &testCommandServer]()
		{
			auto* bridge = w.imguiBridge();
			if ( bridge )
			{
				testCommandServer = new TestCommandServer( bridge, &w );
				testCommandServer->start();
			}
			else
			{
				qWarning() << "[Test] ImGuiBridge not available, command server not started";
			}
		} );
	}

	auto ret = a.exec();

	delete testCommandServer;
	delete testController;

	gameThread.terminate();
	gameThread.wait();

	return ret;
}

#ifdef _WIN32
INT WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow )
{
	return main( 0, nullptr );
}

extern "C"
{
	// Request use of dedicated GPUs for NVidia/AMD/iGPU mixed setups
	__declspec( dllexport ) DWORD NvOptimusEnablement                  = 1;
	__declspec( dllexport ) DWORD AmdPowerXpressRequestHighPerformance = 1;
}
#endif // _WIN32

