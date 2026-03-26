#include "updatechecker.h"
#include "../version.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSysInfo>
#include <QDebug>

UpdateChecker::UpdateChecker( QObject* parent )
	: QObject( parent )
	, m_network( new QNetworkAccessManager( this ) )
{
	connect( m_network, &QNetworkAccessManager::finished, this, &UpdateChecker::onCheckFinished );
}

void UpdateChecker::checkForUpdates()
{
#ifndef GIT_REPO
	// No repo configured at build time — skip update check
	return;
#else
	QString url = QString( "https://api.github.com/repos/%1/releases/latest" ).arg( GIT_REPO );
	QNetworkRequest req{ QUrl( url ) };
	req.setHeader( QNetworkRequest::UserAgentHeader, "Ingnomia/" PROJECT_VERSION );
	req.setRawHeader( "Accept", "application/vnd.github+json" );
	m_network->get( req );
#endif
}

void UpdateChecker::onCheckFinished( QNetworkReply* reply )
{
	reply->deleteLater();

	if ( reply->error() != QNetworkReply::NoError )
	{
		qDebug() << "[UpdateChecker] Network error:" << reply->errorString();
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson( reply->readAll() );
	if ( !doc.isObject() )
		return;

	QJsonObject obj = doc.object();
	QString tagName = obj["tag_name"].toString(); // e.g. "v0.8.11.0+42"
	m_releaseNotes = obj["body"].toString();
	m_downloadUrl = obj["html_url"].toString();

	// Strip leading 'v' and any build metadata after '+' for comparison
	QString remoteVersion = tagName;
	if ( remoteVersion.startsWith( 'v' ) )
		remoteVersion = remoteVersion.mid( 1 );

	// Extract just the version part (before '+')
	int plusIdx = remoteVersion.indexOf( '+' );
	if ( plusIdx >= 0 )
		remoteVersion = remoteVersion.left( plusIdx );

	m_latestVersion = remoteVersion;

	// Compare dotted version strings
	QString localVersion = PROJECT_VERSION;
	QStringList local = localVersion.split( '.' );
	QStringList remote = remoteVersion.split( '.' );

	bool newer = false;
	int maxParts = qMax( local.size(), remote.size() );
	for ( int i = 0; i < maxParts; ++i )
	{
		int l = ( i < local.size() ) ? local[i].toInt() : 0;
		int r = ( i < remote.size() ) ? remote[i].toInt() : 0;
		if ( r > l ) { newer = true; break; }
		if ( r < l ) { break; }
	}

	// Also treat same version but higher build number as newer
	if ( !newer && plusIdx >= 0 )
	{
		int remoteBuild = tagName.mid( tagName.indexOf( '+' ) + 1 ).toInt();
#ifdef BUILD_ID
		int localBuild = QString( BUILD_ID ).toInt();
		if ( remoteBuild > localBuild )
			newer = true;
#endif
	}

	m_updateAvailable = newer;

	if ( newer )
	{
		qInfo() << "[UpdateChecker] New version available:" << m_latestVersion
				<< "(current:" << PROJECT_VERSION << ")";
	}
	else
	{
		qDebug() << "[UpdateChecker] Up to date (" << PROJECT_VERSION << ")";
	}
}
