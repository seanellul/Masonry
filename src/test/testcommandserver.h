#pragma once

#include <QObject>
#include <QThread>
#include <QJsonObject>

class ImGuiBridge;
class MainWindow;

// Reads JSON commands from stdin on a worker thread
class StdinReader : public QObject
{
	Q_OBJECT

public:
	StdinReader( QObject* parent = nullptr );

public slots:
	void run();

signals:
	void commandReceived( const QJsonObject& cmd );
	void finished();
};

// Dispatches JSON commands to game systems and writes JSON responses to stdout
class TestCommandServer : public QObject
{
	Q_OBJECT

public:
	TestCommandServer( ImGuiBridge* bridge, MainWindow* window, QObject* parent = nullptr );
	~TestCommandServer();

	void start();

private slots:
	void onCommand( const QJsonObject& cmd );

private:
	void sendResponse( const QJsonObject& response );
	QJsonObject handleGetState();
	QJsonObject handleScreenshot( const QJsonObject& cmd );
	QJsonObject handleAdvanceTicks( const QJsonObject& cmd );
	QJsonObject handleLoadSave( const QJsonObject& cmd );
	QJsonObject handleSetPaused( const QJsonObject& cmd );
	QJsonObject handleSetGameSpeed( const QJsonObject& cmd );
	QJsonObject handleBuild( const QJsonObject& cmd );
	QJsonObject handleSpawnCreature( const QJsonObject& cmd );
	QJsonObject handleGetMetrics();

	void onHeartbeat( int value );
	void onLoadGameDone( bool success );

	ImGuiBridge* m_bridge;
	MainWindow* m_window;
	QThread m_readerThread;

	// For advance_ticks command
	int m_targetTicks = 0;
	int m_currentTicks = 0;
	bool m_waitingForTicks = false;
	bool m_waitingForLoad = false;
};
