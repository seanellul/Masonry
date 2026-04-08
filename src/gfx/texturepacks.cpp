/*
    This file is part of Masonry / Ingnomia
    Copyright (C) 2017-2026  Ralph Schurade, Sean Ellul, Ingnomia/Masonry Team

    GNU AGPL v3.
*/
#include "texturepacks.h"

#include "../base/global.h"
#include "../base/config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

QList<TexturePackInfo> discoverTexturePacks()
{
	QList<TexturePackInfo> packs;

	if ( !Global::cfg ) return packs;
	const QString dataPath = Global::cfg->get( "dataPath" ).toString();
	const QString packsRoot = dataPath + "/texturepacks";

	QDir root( packsRoot );
	if ( !root.exists() )
	{
		qDebug() << "TexturePacks: no texturepacks directory at" << packsRoot;
		return packs;
	}

	const QStringList entries = root.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
	for ( const QString& entry : entries )
	{
		const QString packDir = root.absoluteFilePath( entry );
		const QString manifest = packDir + "/pack.json";
		if ( !QFile::exists( manifest ) )
		{
			qDebug() << "TexturePacks: skipping" << entry << "— no pack.json";
			continue;
		}

		QFile mf( manifest );
		if ( !mf.open( QIODevice::ReadOnly ) )
		{
			qDebug() << "TexturePacks: cannot open" << manifest;
			continue;
		}
		const QByteArray bytes = mf.readAll();
		mf.close();

		QJsonParseError err{};
		const QJsonDocument doc = QJsonDocument::fromJson( bytes, &err );
		if ( err.error != QJsonParseError::NoError || !doc.isObject() )
		{
			qDebug() << "TexturePacks: invalid pack.json in" << entry << "—" << err.errorString();
			continue;
		}
		const QJsonObject obj = doc.object();

		TexturePackInfo info;
		info.id          = obj.value( "id" ).toString( entry );
		info.name        = obj.value( "name" ).toString( info.id );
		info.author      = obj.value( "author" ).toString();
		info.version     = obj.value( "version" ).toString();
		info.description = obj.value( "description" ).toString();
		info.dirPath     = packDir;

		const QString previewRel = obj.value( "preview" ).toString();
		if ( !previewRel.isEmpty() )
		{
			const QString previewAbs = packDir + "/" + previewRel;
			if ( QFile::exists( previewAbs ) )
				info.previewPath = previewAbs;
		}

		packs.append( info );
	}

	return packs;
}

QString resolveTilesheetPath( const QStringList& activePackIDs, const QString& filename )
{
	if ( !Global::cfg ) return filename;
	const QString dataPath = Global::cfg->get( "dataPath" ).toString();

	for ( const QString& packID : activePackIDs )
	{
		if ( packID.isEmpty() ) continue;
		const QString candidate = dataPath + "/texturepacks/" + packID + "/tilesheet/" + filename;
		if ( QFile::exists( candidate ) )
			return candidate;
	}

	// Final fallback: the default tilesheet directory.
	return dataPath + "/tilesheet/" + filename;
}
