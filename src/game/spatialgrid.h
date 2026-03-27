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
#pragma once

#include "../base/position.h"

#include <QHash>
#include <QVector>

class SpatialGrid
{
public:
	SpatialGrid( int dimX, int dimY, int dimZ, int cellSize = 16 );

	uint32_t cellID( const Position& pos ) const;
	uint32_t cellID( int x, int y, int z ) const;

	// Entity (gnome/creature) operations
	void insertEntity( unsigned int entityID, const Position& pos );
	void removeEntity( unsigned int entityID, const Position& pos );
	void moveEntity( unsigned int entityID, const Position& oldPos, const Position& newPos );

	const QVector<unsigned int>& entitiesInCell( uint32_t cell ) const;
	void entitiesInRadius( const Position& center, int rings, QVector<unsigned int>& out ) const;

	// Job operations
	void insertJob( unsigned int jobID, const Position& pos );
	void removeJob( unsigned int jobID, const Position& pos );
	void jobsInRadius( const Position& center, int rings, QVector<unsigned int>& out ) const;

private:
	int m_cellSize;
	int m_cellsX;
	int m_cellsY;
	int m_cellsZ;

	QHash<uint32_t, QVector<unsigned int>> m_entityCells;
	QHash<uint32_t, QVector<unsigned int>> m_jobCells;

	// Reverse maps for O(1) move/remove
	QHash<unsigned int, uint32_t> m_entityToCell;
	QHash<unsigned int, uint32_t> m_jobToCell;

	static const QVector<unsigned int> EMPTY_VEC;

	void cellsInRadius( const Position& center, int rings, QVector<uint32_t>& out ) const;
};
