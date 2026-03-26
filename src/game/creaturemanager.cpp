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
#include "creaturemanager.h"
#include "game.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/logger.h"
#include "../base/regionmap.h"
#include "../base/util.h"
#include "../game/world.h"
#include "../game/creaturefactory.h"
#include "../game/farmingmanager.h"
#include "../game/inventory.h"
#include "../game/jobmanager.h"
#include "../game/newgamesettings.h"

#include <QDebug>
#include <QElapsedTimer>

CreatureManager::CreatureManager( Game* parent ) :
	g( parent ),
	QObject( parent )
{
}

CreatureManager::~CreatureManager()
{
	for ( const auto& c : m_creatures )
	{
		delete c;
	}
}

void CreatureManager::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	QElapsedTimer timer;
	timer.start();

	if ( m_startIndex >= m_creatures.size() )
	{
		m_startIndex = 0;
	}
	int oldStartIndex = m_startIndex;
	QList<unsigned int> toDestroy;
	QList<unsigned int> toDead;
	for ( int i = m_startIndex; i < m_creatures.size(); ++i )
	{
		Creature* creature = m_creatures[i];

		CreatureTickResult ctr = creature->onTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );

		switch ( ctr )
		{
			case CreatureTickResult::TODESTROY:
				toDestroy.append( creature->id() );
				break;
			case CreatureTickResult::DEAD:
			{
				// Keep dead creatures as corpses (like gnomes) instead of destroying immediately
				creature->setExpires( GameState::tick + Global::util->ticksPerDay * 3 ); // 3 days to rot
				if( creature->type() == CreatureType::ANIMAL )
				{
					auto a = dynamic_cast<Animal*>( creature );
					g->inv()->createItem( a->getPos(), "AnimalCorpse", { a->species() } );
				}
				toDead.append( creature->id() );
				Global::logger().log( LogType::DEATH, creature->name() + " (" + creature->species() + ") has died.", creature->id() );
				break;
			}
		}

		m_startIndex = i + 1;
		if ( timer.elapsed() > 2 )
			break;
	}

	// Move dead creatures to corpse list (preserve for inspection + rot)
	for ( auto did : toDead )
	{
		for ( int i = 0; i < m_creatures.size(); ++i )
		{
			if ( did == m_creatures[i]->id() )
			{
				m_creatures[i]->setDeathTick( GameState::tick );
				m_deadCreatures.append( m_creatures[i] );
				m_creatures.removeAt( i );
				m_dirty = true;
				break;
			}
		}
	}

	// Process corpse rot — advance stages, create bones at final stage
	for ( int i = m_deadCreatures.size() - 1; i >= 0; --i )
	{
		auto corpse = m_deadCreatures[i];
		corpse->advanceRot( GameState::tick );

		if ( corpse->rotStage() == Creature::RotStage::Bones )
		{
			// Create bones — count based on creature's body part count (size proxy)
			int externalParts = 0;
			for ( const auto& part : corpse->anatomy().parts() )
			{
				if ( !part.isInside && part.id != KCP_NONE &&
					 part.id != CP_HAIR && part.id != CP_FACIAL_HAIR &&
					 part.id != CP_CLOTHING && part.id != CP_BOOTS &&
					 part.id != CP_HAT && part.id != CP_BACK )
				{
					++externalParts;
				}
			}
			// Small creature (chicken): ~4 parts → 1 bone
			// Medium creature (gnome): ~10 parts → 3 bones
			// Large creature (yak): ~12+ parts → 4 bones
			int boneCount = ( externalParts / 3 );
			if ( boneCount < 1 ) boneCount = 1;

			for ( int b = 0; b < boneCount; ++b )
			{
				g->inv()->createItem( corpse->getPos(), "Bone", { corpse->species() } );
			}

			g->m_world->addToUpdateList( corpse->getPos() );
			m_creaturesByID.remove( corpse->id() );
			delete corpse;
			m_deadCreatures.removeAt( i );
		}
	}

	if ( toDestroy.size() )
	{
		for ( auto aid : toDestroy )
		{
			for ( int i = 0; i < m_creatures.size(); ++i )
			{
				if ( aid == m_creatures[i]->id() )
				{
					removeCreature( aid );
					break;
				}
			}
		}
	}

	if ( hourChanged )
	{
		
	}
}

QList<Creature*> CreatureManager::creaturesAtPosition( Position& pos )
{
	QList<Creature*> out;
	unsigned int tileID = pos.toInt();
	auto& posMap = g->w()->creaturePositions();
	if ( posMap.contains( tileID ) )
	{
		for ( auto id : posMap[tileID] )
		{
			if ( m_creaturesByID.contains( id ) )
			{
				out.push_back( m_creaturesByID[id] );
			}
		}
	}
	return out;
}

QList<Animal*> CreatureManager::animalsAtPosition( Position& pos )
{
	QList<Animal*> out;
	unsigned int tileID = pos.toInt();
	auto& posMap = g->w()->creaturePositions();
	if ( posMap.contains( tileID ) )
	{
		for ( auto id : posMap[tileID] )
		{
			if ( m_creaturesByID.contains( id ) )
			{
				auto c = m_creaturesByID[id];
				if ( c->isAnimal() )
				{
					out.push_back( dynamic_cast<Animal*>( c ) );
				}
			}
		}
	}
	return out;
}

QList<Monster*> CreatureManager::monstersAtPosition( Position& pos )
{
	QList<Monster*> out;
	unsigned int tileID = pos.toInt();
	auto& posMap = g->w()->creaturePositions();
	if ( posMap.contains( tileID ) )
	{
		for ( auto id : posMap[tileID] )
		{
			if ( m_creaturesByID.contains( id ) )
			{
				auto c = m_creaturesByID[id];
				if ( c->isMonster() )
				{
					out.push_back( dynamic_cast<Monster*>( c ) );
				}
			}
		}
	}
	return out;
}

int CreatureManager::countWildAnimals()
{
	return m_creatures.size();
}

unsigned int CreatureManager::addCreature( CreatureType ct, QString type, Position pos, Gender gender, bool adult, bool tame, int level )
{
	Creature* creature = nullptr;
	CreatureFactory cf( g );
	switch ( ct )
	{
		case CreatureType::ANIMAL:
			creature = cf.createAnimal( type, pos, gender, adult, tame );
			break;
		case CreatureType::MONSTER:
			creature = cf.createMonster( type, level, pos, gender );
			break;
	}

	if( creature )
	{
		m_creatures.append( creature );
		unsigned int id = m_creatures.last()->id();
		m_creaturesByID.insert( id, m_creatures.last() );

		int count = m_countPerType.value( type );
	
		++count;
		m_countPerType.insert( type, count );

		auto& list = m_creaturesPerType[type];
		list.append( id );

		m_dirty = true;

		if( creature->hasTransparency() )
		{
			g->m_world->setTileFlag( creature->getPos(), TileFlag::TF_TRANSPARENT );
		}

		return id;
	}
	return 0;
}

unsigned int CreatureManager::addCreature( CreatureType ct, QVariantMap vals )
{
	Creature* creature = nullptr;
	CreatureFactory cf( g );
	switch ( ct )
	{
		case CreatureType::ANIMAL:
			creature = cf.createAnimal( vals );
			break;
		case CreatureType::MONSTER:
			creature = cf.createMonster( vals );
			break;
	}
	if( creature )
	{
		m_creatures.append( creature );
		unsigned int id = m_creatures.last()->id();
		m_creaturesByID.insert( id, m_creatures.last() );

		QString type = creature->species();
		int count    = m_countPerType.value( type );
		++count;
		m_countPerType.insert( type, count );

		auto& list = m_creaturesPerType[type];
		list.append( id );

		m_dirty = true;

		if( creature->hasTransparency() )
		{
			g->m_world->setTileFlag( creature->getPos(), TileFlag::TF_TRANSPARENT );
		}

		return id;
	}
	return 0;
}

Creature* CreatureManager::creature( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		return m_creaturesByID[id];
	}
	return nullptr;
}

Animal* CreatureManager::animal( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		auto c = m_creaturesByID[id];
		if( c->isAnimal() )
		{
			return dynamic_cast<Animal*>( c );
		}
	}
	return nullptr;
}
	
Monster* CreatureManager::monster( unsigned int id )
{
	if ( m_creaturesByID.contains( id ) )
	{
		auto c = m_creaturesByID[id];
		if( c->isMonster() )
		{
			return dynamic_cast<Monster*>( c );
		}
	}
	return nullptr;
}

int CreatureManager::count()
{
	return m_creatures.size();
}

int CreatureManager::count( QString type )
{
	return m_countPerType.value( type );
}

void CreatureManager::removeCreature( unsigned int id )
{
	if( m_creaturesByID.contains( id ) )
	{
		auto creature = m_creaturesByID[id];

		switch( creature->type() )
		{
			case CreatureType::ANIMAL:
			{
				Animal* a = dynamic_cast<Animal*>( creature );
				if ( a )
				{
					if ( a->pastureID() )
					{
						qDebug() << "remove animal from pasture";
						auto pasture = g->m_farmingManager->getPasture( a->pastureID() );
						if ( pasture )
						{
							pasture->removeAnimal( a->id() );
						}
					}
				}
			}
			break;
		}

		auto& perTypeList = m_creaturesPerType[creature->species()];
		perTypeList.removeAll( id );

		m_creaturesByID.remove( id );
		m_creatures.removeAll( creature );

		int count = m_countPerType.value( creature->species() );
		--count;
		m_countPerType.insert( creature->species(), count );

		delete creature;

		m_dirty = true;

		emit signalCreatureRemove( id );
	}
}

Animal* CreatureManager::getClosestAnimal( Position pos, QString type )
{
	auto distanceQueue = animalsByDistance( pos, type );

	if ( !distanceQueue.empty() )
	{
		return distanceQueue.get();
	}
	return nullptr;
}

PriorityQueue<Animal*, int> CreatureManager::animalsByDistance( Position pos, QString type )
{
	auto distanceQueue = PriorityQueue<Animal*, int>();
	for ( auto id : m_creaturesPerType[type] )
	{
		Creature* creature = m_creaturesByID[id];
		if( creature->isAnimal() )
		{
			Animal* a = dynamic_cast<Animal*>( creature );
			if ( a && !a->inJob() && !a->isDead() && !a->toDestroy() )
			{
				Position targetPos = a->getPos();
				if ( g->m_pf->checkConnectedRegions( pos, targetPos ) )
				{
					distanceQueue.put( a, pos.distSquare( targetPos ) );
				}
			}
		}
	}
	return distanceQueue;
}

QList<unsigned int> CreatureManager::animalsByType( QString type )
{
	return m_creaturesPerType.value( type );
}

void CreatureManager::forceMoveAnimals( const Position& from, const Position& to )
{
	for ( auto& a : m_creatures )
	{
		// check gnome position
		if ( a->getPos().toInt() == from.toInt() )
		{
			//qDebug() << "force move gnome from " << from.toString() << " to " << to.toString();
			// move gnome
			a->forceMove( to );
			// abort job if he has one
		}
	}
}

QList<QString> CreatureManager::types()
{
	return m_countPerType.keys();
}

	
QList<Animal*>& CreatureManager::animals()
{
	if( m_dirty )
	{
		updateLists();
	}

	return m_animals;
}

QList<Monster*>& CreatureManager::monsters()
{
	if( m_dirty )
	{
		updateLists();
	}

	return m_monsters;
}

void CreatureManager::updateLists()
{
	m_animals.clear();
	m_monsters.clear();

	for( const auto& c : m_creatures )
	{
		if( c->isAnimal() )
		{
			m_animals.append( dynamic_cast<Animal*>( c ) );
		}
		else
		{
			m_monsters.append( dynamic_cast<Monster*>( c ) );
		}
	}
	m_dirty = false;
}

bool CreatureManager::hasPathTo( const Position& pos, unsigned int creatureID )
{
	if( m_creaturesByID.contains( creatureID ) )
	{
		auto creature = m_creaturesByID[creatureID];
		if( creature )
		{
			return g->m_world->regionMap().checkConnectedRegions( pos, creature->getPos() );
		}
	}
	return false;
}

bool CreatureManager::hasLineOfSightTo( const Position& pos, unsigned int creatureID )
{
	if ( m_creaturesByID.contains( creatureID ) )
	{
		auto creature = m_creaturesByID[creatureID];
		if ( creature )
		{
			return g->m_world->isLineOfSight( pos, creature->getPos() );
		}
	}
	return false;
}
