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
#include "spatialgrid.h"

#include <algorithm>

const QVector<unsigned int> SpatialGrid::EMPTY_VEC;

SpatialGrid::SpatialGrid( int dimX, int dimY, int dimZ, int cellSize ) :
	m_cellSize( cellSize ),
	m_cellsX( ( dimX + cellSize - 1 ) / cellSize ),
	m_cellsY( ( dimY + cellSize - 1 ) / cellSize ),
	m_cellsZ( dimZ )
{
}

uint32_t SpatialGrid::cellID( const Position& pos ) const
{
	return cellID( pos.x, pos.y, pos.z );
}

uint32_t SpatialGrid::cellID( int x, int y, int z ) const
{
	int cx = x / m_cellSize;
	int cy = y / m_cellSize;
	int cz = z;
	return static_cast<uint32_t>( cx + cy * m_cellsX + cz * m_cellsX * m_cellsY );
}

void SpatialGrid::insertEntity( unsigned int entityID, const Position& pos )
{
	uint32_t cell = cellID( pos );
	m_entityCells[cell].append( entityID );
	m_entityToCell[entityID] = cell;
}

void SpatialGrid::removeEntity( unsigned int entityID, const Position& pos )
{
	Q_UNUSED( pos );
	auto it = m_entityToCell.find( entityID );
	if ( it == m_entityToCell.end() )
		return;

	uint32_t cell = it.value();
	auto& vec     = m_entityCells[cell];
	int idx       = vec.indexOf( entityID );
	if ( idx >= 0 )
	{
		// Swap-and-pop for O(1) removal
		vec[idx] = vec.last();
		vec.removeLast();
	}
	m_entityToCell.erase( it );
}

void SpatialGrid::moveEntity( unsigned int entityID, const Position& oldPos, const Position& newPos )
{
	uint32_t oldCell = cellID( oldPos );
	uint32_t newCell = cellID( newPos );

	if ( oldCell == newCell )
	{
		// Same cell — just update reverse map (already correct)
		return;
	}

	// Remove from old cell
	auto& oldVec = m_entityCells[oldCell];
	int idx      = oldVec.indexOf( entityID );
	if ( idx >= 0 )
	{
		oldVec[idx] = oldVec.last();
		oldVec.removeLast();
	}

	// Insert into new cell
	m_entityCells[newCell].append( entityID );
	m_entityToCell[entityID] = newCell;
}

const QVector<unsigned int>& SpatialGrid::entitiesInCell( uint32_t cell ) const
{
	auto it = m_entityCells.constFind( cell );
	if ( it != m_entityCells.constEnd() )
		return it.value();
	return EMPTY_VEC;
}

void SpatialGrid::cellsInRadius( const Position& center, int rings, QVector<uint32_t>& out ) const
{
	int cx = center.x / m_cellSize;
	int cy = center.y / m_cellSize;
	int cz = center.z;

	for ( int dz = -rings; dz <= rings; ++dz )
	{
		int z = cz + dz;
		if ( z < 0 || z >= m_cellsZ )
			continue;

		for ( int dy = -rings; dy <= rings; ++dy )
		{
			int y = cy + dy;
			if ( y < 0 || y >= m_cellsY )
				continue;

			for ( int dx = -rings; dx <= rings; ++dx )
			{
				int x = cx + dx;
				if ( x < 0 || x >= m_cellsX )
					continue;

				out.append( static_cast<uint32_t>( x + y * m_cellsX + z * m_cellsX * m_cellsY ) );
			}
		}
	}
}

void SpatialGrid::entitiesInRadius( const Position& center, int rings, QVector<unsigned int>& out ) const
{
	QVector<uint32_t> cells;
	cellsInRadius( center, rings, cells );

	for ( uint32_t cell : cells )
	{
		auto it = m_entityCells.constFind( cell );
		if ( it != m_entityCells.constEnd() )
		{
			out.append( it.value() );
		}
	}
}

void SpatialGrid::insertJob( unsigned int jobID, const Position& pos )
{
	uint32_t cell = cellID( pos );
	m_jobCells[cell].append( jobID );
	m_jobToCell[jobID] = cell;
}

void SpatialGrid::removeJob( unsigned int jobID, const Position& pos )
{
	Q_UNUSED( pos );
	auto it = m_jobToCell.find( jobID );
	if ( it == m_jobToCell.end() )
		return;

	uint32_t cell = it.value();
	auto& vec     = m_jobCells[cell];
	int idx       = vec.indexOf( jobID );
	if ( idx >= 0 )
	{
		vec[idx] = vec.last();
		vec.removeLast();
	}
	m_jobToCell.erase( it );
}

void SpatialGrid::jobsInRadius( const Position& center, int rings, QVector<unsigned int>& out ) const
{
	QVector<uint32_t> cells;
	cellsInRadius( center, rings, cells );

	for ( uint32_t cell : cells )
	{
		auto it = m_jobCells.constFind( cell );
		if ( it != m_jobCells.constEnd() )
		{
			out.append( it.value() );
		}
	}
}
