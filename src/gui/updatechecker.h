#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class UpdateChecker : public QObject
{
	Q_OBJECT

public:
	explicit UpdateChecker( QObject* parent = nullptr );

	void checkForUpdates();

	bool updateAvailable() const { return m_updateAvailable; }
	QString latestVersion() const { return m_latestVersion; }
	QString releaseNotes() const { return m_releaseNotes; }
	QString downloadUrl() const { return m_downloadUrl; }

private slots:
	void onCheckFinished( QNetworkReply* reply );

private:
	QNetworkAccessManager* m_network = nullptr;
	bool m_updateAvailable = false;
	QString m_latestVersion;
	QString m_releaseNotes;
	QString m_downloadUrl;
};
