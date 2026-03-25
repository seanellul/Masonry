#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QString>

class MainWindow;
class QTimer;

struct TestConfig
{
	bool enabled = false;
	QString loadSavePath;
	QString screenshotPath;
	int screenshotDelay = 60; // frames
	int runTicks = 0;
	QString metricsOutPath; // empty = stdout
};

class TestController : public QObject
{
	Q_OBJECT

public:
	TestController( const TestConfig& config, MainWindow* window, QObject* parent = nullptr );

	void start();

private:
	void onLoadGameDone( bool success );
	void onHeartbeat( int value );
	void onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void onKingdomInfo( QString name, QString info1, QString info2, QString info3 );

	void startFrameCountdown();
	void doScreenshotFrame();
	void takeScreenshotAndMaybeExit();
	void writeMetricsAndExit();
	void writeMetrics();

	TestConfig m_config;
	MainWindow* m_window;

	// State tracking
	bool m_saveLoaded = false;
	int m_ticksElapsed = 0;
	int m_framesRendered = 0;
	QElapsedTimer m_elapsed;

	// Collected metrics
	int m_minute = 0, m_hour = 0, m_day = 0, m_year = 0;
	QString m_season;
	QString m_sunStatus;
	QString m_kingdomName;
	QString m_kingdomInfo1, m_kingdomInfo2, m_kingdomInfo3;
	bool m_screenshotTaken = false;
	int m_screenshotFrames = 0;
	QTimer* m_screenshotTimer = nullptr;
};
