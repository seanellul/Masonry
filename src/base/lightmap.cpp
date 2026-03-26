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
#include "lightmap.h"

#include "../base/config.h"
#include "../base/gamestate.h"

#include <QDebug>
#include <QList>
#include <QPair>
#include <QQueue>
#include <QVector3D>

LightMap::LightMap()
{
}

LightMap::~LightMap()
{
}

void LightMap::init()
{
	m_lightMap.clear();
	m_lights.clear();

	m_dimX = Global::dimX;
	m_dimY = Global::dimY;
	m_dimZ = Global::dimZ;
}

void LightMap::addLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, unsigned int id, Position pos, int intensity )
{
	Light light;
	light.id        = id;
	light.pos       = pos;
	light.intensity = intensity;

	QQueue<QPair<Position, int>> wq;
	QSet<unsigned int> visited;
	wq.enqueue( QPair<Position, int>( pos, 0 ) );
	float decay = Global::cfg->get( "lightDecay" ).toFloat();
	if ( decay < 0.1f ) decay = 1.5f;

	int range = (int)( intensity / decay );

	visited.reserve( range * range * range );

	while ( !wq.empty() )
	{
		QPair<Position, int> current = wq.dequeue();
		Position curPos              = current.first;
		unsigned int curPosID        = curPos.toInt();
		if ( !visited.contains( curPosID ) )
		{
			visited.insert( curPosID );

			int curRadius = current.second;

			// Simple distance-based intensity — no LOS raycast (BFS already respects walls)
			int curIntensity = qMax( 0, (int)( intensity - decay * curRadius ) );

			if ( curIntensity > 0 )
			{
				m_lightMap[curPosID].insert( id, curIntensity );

				Tile& tile      = getTile( world, curPos );
				tile.lightLevel = calcIntensity( curPosID );

				light.effectTiles.append( curPosID );
				updateList.insert( curPosID );

				if ( !( tile.wallType & WallType::WT_VIEWBLOCKING ) )
				{
					// Cardinal directions
					if ( curPos.y > 0 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x, curPos.y - 1, curPos.z ), curRadius + 1 ) );
					if ( curPos.y < m_dimY - 1 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x, curPos.y + 1, curPos.z ), curRadius + 1 ) );
					if ( curPos.x < m_dimX - 1 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x + 1, curPos.y, curPos.z ), curRadius + 1 ) );
					if ( curPos.x > 0 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x - 1, curPos.y, curPos.z ), curRadius + 1 ) );

					// Diagonal directions — light wraps around corners
					if ( curPos.x > 0 && curPos.y > 0 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x - 1, curPos.y - 1, curPos.z ), curRadius + 1 ) );
					if ( curPos.x < m_dimX - 1 && curPos.y > 0 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x + 1, curPos.y - 1, curPos.z ), curRadius + 1 ) );
					if ( curPos.x > 0 && curPos.y < m_dimY - 1 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x - 1, curPos.y + 1, curPos.z ), curRadius + 1 ) );
					if ( curPos.x < m_dimX - 1 && curPos.y < m_dimY - 1 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x + 1, curPos.y + 1, curPos.z ), curRadius + 1 ) );

					// Vertical — through stairs, ramps, and open floors
					bool canGoDown = !( tile.floorType & FloorType::FT_SOLIDFLOOR )
						|| ( tile.wallType & ( WallType::WT_STAIR | WallType::WT_RAMP ) )
						|| ( tile.floorType & ( FloorType::FT_STAIRTOP | FloorType::FT_RAMPTOP ) );
					if ( canGoDown && curPos.z > 0 )
						wq.enqueue( QPair<Position, int>( Position( curPos.x, curPos.y, curPos.z - 1 ), curRadius + 1 ) );

					if ( curPos.z < m_dimZ - 1 )
					{
						Tile& aboveTile = getTile( world, curPos.x, curPos.y, curPos.z + 1 );
						bool canGoUp = !( aboveTile.floorType & FloorType::FT_SOLIDFLOOR )
							|| ( tile.wallType & ( WallType::WT_STAIR | WallType::WT_RAMP ) )
							|| ( aboveTile.floorType & ( FloorType::FT_STAIRTOP | FloorType::FT_RAMPTOP ) );
						if ( canGoUp )
							wq.enqueue( QPair<Position, int>( Position( curPos.x, curPos.y, curPos.z + 1 ), curRadius + 1 ) );
					}
				}
			}
		}
	}
	m_lights.insert( id, light );
}

unsigned char LightMap::calcIntensity( unsigned int posID )
{
	if ( m_lightMap.contains( posID ) )
	{
		const auto& maps = m_lightMap[posID];
		int light = 0;
		for ( const auto& value : maps )
		{
			light += value;
		}
		return qMin( 255, light );
	}
	else
	{
		return 0;
	}
}

void LightMap::removeLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, unsigned int id )
{
	if ( m_lights.contains( id ) )
	{
		Light light = m_lights[id];
		m_lights.remove( id );

		for ( auto posID : light.effectTiles )
		{
			m_lightMap[posID].remove( id );

			Tile& tile      = world[posID];
			tile.lightLevel = calcIntensity( posID );
			updateList.insert( posID );
		}
	}
}

void LightMap::updateLight( QSet<unsigned int>& updateList, std::vector<Tile>& world, Position pos )
{
	unsigned int posID = pos.toInt();
	if ( m_lightMap.contains( posID ) )
	{
		auto maps = m_lightMap[posID];

		for ( auto key : maps.keys() )
		{
			Light light = m_lights[key];

			removeLight( updateList, world, key );
			addLight( updateList, world, key, light.pos, light.intensity );
		}
	}
}