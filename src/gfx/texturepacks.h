/*
    This file is part of Masonry / Ingnomia
    Copyright (C) 2017-2026  Ralph Schurade, Sean Ellul, Ingnomia/Masonry Team

    GNU AGPL v3.
*/
#pragma once

#include <QString>
#include <QList>

// T-0022: Texture pack v1 — multi-pack discovery + metadata.
//
// A pack lives at content/texturepacks/<id>/ and contains:
//   - pack.json   (required: metadata)
//   - tilesheet/  (required: PNG atlases that override the defaults)
//   - preview.png (optional: thumbnail for the Settings UI)
//
// Packs are discovered at startup, listed in Settings → Texture Packs,
// and applied via an ordered config entry `activeTexturePacks`. Earlier
// entries override later ones; missing files cascade through the chain
// and ultimately fall back to content/tilesheet/.

struct TexturePackInfo
{
	QString id;            // Pack id (matches the directory name)
	QString name;          // Player-facing display name
	QString author;
	QString version;
	QString description;
	QString dirPath;       // Absolute path to the pack directory
	QString previewPath;   // Absolute path to the preview image, or empty
};

// Scan content/texturepacks/ and return all packs that have a valid
// pack.json. Sorted by id for stable ordering.
QList<TexturePackInfo> discoverTexturePacks();

// Resolve a tilesheet filename through the active pack chain. Returns
// the absolute path of the first pack that contains the file, or the
// default content/tilesheet/<filename> path as the final fallback.
// `activePackIDs` is the ordered list from config.
QString resolveTilesheetPath( const QStringList& activePackIDs, const QString& filename );
