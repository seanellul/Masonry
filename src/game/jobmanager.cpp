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
#include "jobmanager.h"

#include "../base/db.h"
#include "../base/enums.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/farmingmanager.h"
#include "../game/game.h"
#include "../game/gnomemanager.h"
#include "../game/spatialgrid.h"
#include "../game/inventory.h"
#include "../game/mechanismmanager.h"
#include "../game/stockpilemanager.h"
#include "../game/workshopmanager.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QVariantMap>

JobManager::JobManager( Game* parent ) :
	m_startIndex( 0 ),
	g( parent ),
	QObject( parent )
{
	for ( auto job : DB::jobIds() )
	{
		m_jobsPerType.insert( job, QMultiHash<int, unsigned int>() );
	}

	m_skillToInt.insert( "Mining", SK_Mining );
	m_skillToInt.insert( "Masonry", SK_Masonry );
	m_skillToInt.insert( "Stonecarving", SK_Stonecarving );
	m_skillToInt.insert( "Woodcutting", SK_Woodcutting );
	m_skillToInt.insert( "Carpentry", SK_Carpentry );
	m_skillToInt.insert( "Woodcarving", SK_Woodcarving );
	m_skillToInt.insert( "Smelting", SK_Smelting );
	m_skillToInt.insert( "Blacksmithing", SK_Blacksmithing );
	m_skillToInt.insert( "Metalworking", SK_Metalworking );
	m_skillToInt.insert( "WeaponCrafting", SK_WeaponCrafting );
	m_skillToInt.insert( "ArmorCrafting", SK_ArmorCrafting );
	m_skillToInt.insert( "Gemcutting", SK_Gemcutting );
	m_skillToInt.insert( "JewelryMaking", SK_JewelryMaking );
	m_skillToInt.insert( "Weaving", SK_Weaving );
	m_skillToInt.insert( "Tailoring", SK_Tailoring );
	m_skillToInt.insert( "Dyeing", SK_Dyeing );
	m_skillToInt.insert( "Pottery", SK_Pottery );
	m_skillToInt.insert( "Leatherworking", SK_Leatherworking );
	m_skillToInt.insert( "Bonecarving", SK_Bonecarving );
	m_skillToInt.insert( "Prospecting", SK_Prospecting );
	m_skillToInt.insert( "Tinkering", SK_Tinkering );
	m_skillToInt.insert( "Machining", SK_Machining );
	m_skillToInt.insert( "Engineering", SK_Engineering );
	m_skillToInt.insert( "Mechanic", SK_Mechanic );
	m_skillToInt.insert( "AnimalHusbandry", SK_AnimalHusbandry );
	m_skillToInt.insert( "Butchery", SK_Butchery );
	m_skillToInt.insert( "Fishing", SK_Fishing );
	m_skillToInt.insert( "Horticulture", SK_Horticulture );
	m_skillToInt.insert( "Farming", SK_Farming );
	m_skillToInt.insert( "Cooking", SK_Cooking );
	m_skillToInt.insert( "Brewing", SK_Brewing );
	m_skillToInt.insert( "Construction", SK_Construction );
	m_skillToInt.insert( "Hauling", SK_Hauling );
	m_skillToInt.insert( "Unarmed", SK_Unarmed );
	m_skillToInt.insert( "Melee", SK_Melee );
	m_skillToInt.insert( "Ranged", SK_Ranged );
	m_skillToInt.insert( "Thrown", SK_Thrown );
	m_skillToInt.insert( "Dodge", SK_Dodge );
	m_skillToInt.insert( "Block", SK_Block );
	m_skillToInt.insert( "Armor", SK_Armor );
	m_skillToInt.insert( "Crossbow", SK_Crossbow );
	m_skillToInt.insert( "Gun", SK_Gun );
	m_skillToInt.insert( "Medic", SK_Medic );
	m_skillToInt.insert( "Caretaking", SK_Caretaking );
	m_skillToInt.insert( "MagicNature", SK_MagicNature );
	m_skillToInt.insert( "MagicGeomancy", SK_MagicGeomancy );

	for ( auto skillID : DB::ids( "Skills" ) )
	{
		QStringList jobs;
		for( auto jobID : DB::jobIds() )
		{
			auto dbjob = DB::job( jobID );
			if( dbjob->SkillID == skillID )
			{
				jobs.append( jobID );
			}
		}
		m_jobIDs.insert( skillID, jobs );
	}

	auto skillList = DB::execQuery2( "SELECT DISTINCT \"SkillID\" FROM Crafts" );
	for( auto skill : skillList )
	{
		m_workshopSkills.insert( skill.toString() );
	}
}

JobManager::~JobManager()
{
	m_jobList.clear();
}

void JobManager::onTick()
{
	QElapsedTimer timer;
	timer.start();

	unsigned int jobID = 0;

	int queueSize = m_returnedJobQueue.size();

	QQueue<unsigned int> skippedJobs;

	while ( !m_returnedJobQueue.empty() )
	{
		jobID = m_returnedJobQueue.dequeue();
		--queueSize;
		QSharedPointer<Job> job = m_jobList.value( jobID );

		if( job )
		{
			if ( workPositionWalkable( job->id() ) && requiredToolExists( job->id() ) )
			{
				if ( requiredItemsAvail( jobID ) )
				{
					m_jobsPerType[job->type()].insert( job->priority(), job->id() );
					pushJobToGnomes( jobID );
				}
				else
				{
					skippedJobs.enqueue( jobID );
				}
			}
			else
			{
				skippedJobs.enqueue( jobID );
			}
		}

		if ( timer.elapsed() > 3 )
		{
			break;
		}
	}
	m_returnedJobQueue += skippedJobs;

	// Process overflow pool periodically (jobs with no nearby eligible gnomes)
	if ( GameState::tick % 120 == 0 )
	{
		processOverflowPool();
	}
}

bool JobManager::requiredItemsAvail( unsigned int jobID )
{
	QSharedPointer<Job> job = m_jobList.value( jobID );

	// Skip expensive checks if inventory hasn't changed since last check
	quint64 invGen = g->inv()->itemCountGeneration();
	if ( job->m_availabilityCacheGeneration == invGen )
	{
		return job->m_cachedAvailability;
	}

	bool found_all = true;
	for ( auto rim : job->requiredItems() )
	{
		bool found = false;
		for ( auto pos : job->possibleWorkPositions() )
		{
			if ( g->inv()->checkReachableItems( pos, true, rim.count, rim.itemSID, rim.materialSID ) )
			{
				found         = true;
				rim.available = true;
				break;
			}
		}
		if ( !found )
		{
			found_all     = false;
			rim.available = false;

			//is item craftable?
			if ( Global::craftable.contains( rim.itemSID ) )
			{
				// create craft job
				g->wsm()->autoGenCraftJob( rim.itemSID, rim.materialSID, rim.count );
			}
		}
	}

	job->m_availabilityCacheGeneration = invGen;
	job->m_cachedAvailability          = found_all;
	return found_all;
}

bool JobManager::workPositionWalkable( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job     = m_jobList.value( jobID );
		Position pos = job->pos();
		int rotation = job->rotation();

		job->clearPossibleWorkPositions();
		// jobs on same tile — apply rotation to work position offsets
		auto wpl = job->origWorkPosOffsets();
		for ( const auto& offset : wpl )
		{
			Position rotatedOffset = offset;
			switch ( rotation )
			{
				case 1:
					rotatedOffset.x = -1 * offset.y;
					rotatedOffset.y = offset.x;
					break;
				case 2:
					rotatedOffset.x = -1 * offset.x;
					rotatedOffset.y = -1 * offset.y;
					break;
				case 3:
					rotatedOffset.x = offset.y;
					rotatedOffset.y = -1 * offset.x;
					break;
			}
			Position testPos( pos + rotatedOffset );
			if ( g->w()->isWalkableGnome( testPos ) )
			{
				job->addPossibleWorkPosition( testPos );
			}
		}

		return job->possibleWorkPositions().size() > 0;
	}
	return false;
}

bool JobManager::requiredToolExists( unsigned int jobID )
{
	QSharedPointer<Job> job = m_jobList.value( jobID );
	auto rt  = job->requiredTool();

	// need to figure out how to check rt 'inuse' & 'reachable'
	job->setRequiredToolAvailable( true );

	if ( rt.type.isEmpty() )
	{
		return true;
	}

	QMap<QString, int> mc = g->inv()->materialCountsForItem( rt.type, false );
	QStringList keys      = mc.keys();

	for ( auto key : keys )
	{
		if ( mc[key] > 0 )
		{
			if ( rt.level == 0 )
			{
				return true;
			}
			int tl = DBH::materialToolLevel( key );
			if ( tl >= rt.level )
			{
				return true;
			}
		}
	}

	job->setRequiredToolAvailable( false );
	return false;
}

bool JobManager::insertIntoPositionHash( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );
		if ( m_jobPositions.contains( job->pos() ) )
		{
			return false;
		}
		else
		{
			m_jobPositions.insert( job->pos(), job->id() );
		}
	}
	return true;
}

void JobManager::removeFromPositionHash( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );
		if ( m_jobPositions.contains( job->pos() ) )
		{
			m_jobPositions.remove( job->pos() );
		}
	}
}

void JobManager::addLoadedJob( QVariant vals )
{
	QSharedPointer<Job> job( new Job( vals.toMap() ) );

	m_jobList.insert( job->id(), job );

	if ( !insertIntoPositionHash( job->id() ) )
	{
		return;
	}

	if ( isReachable( job->id(), 0 ) )
	{
		m_jobsPerType[job->type()].insert( job->priority(), job->id() );
	}
	else
	{
		m_returnedJobQueue.enqueue( job->id() );
	}

	if ( g->sg() )
	{
		g->sg()->insertJob( job->id(), job->pos() );
	}

	setJobSprites( job->id(), job->isWorked(), false );
}

unsigned int JobManager::addJob( QString type, Position pos, int rotation, bool noJobSprite )
{
	if ( g->w()->hasJob( pos ) || m_jobPositions.contains( pos ) )
	{
		return 0;
	}
	QSharedPointer<Job> job( new Job );
	job->setType( type );
	job->setPos( pos );
	job->setRotation( rotation );
	job->setNoJobSprite( noJobSprite );
		
	job->setRequiredSkill( Global::util->requiredSkill( type ) );
	job->setRequiredTool( Global::util->requiredTool( type ), Global::util->requiredToolLevel( type, pos ) );

	auto dbjb = DB::job( type );
	if( dbjb )
	{
		job->setMayTrap( dbjb->MayTrapGnome );
		job->setOrigWorkPosOffsets( dbjb->WorkPositions );
	}

	m_jobList.insert( job->id(), job );

	insertIntoPositionHash( job->id() );

	m_returnedJobQueue.enqueue( job->id() );

	if ( g->sg() )
	{
		g->sg()->insertJob( job->id(), pos );
	}

	setJobSprites( job->id(), false, false );

	return job->id();
}

unsigned int JobManager::addJob( QString type, Position pos, QString item, QList<QString> materials, int rotation, bool noJobSprite )
{
	//qDebug() << "JobManager::addJob " << type << item << materials << pos.toString();

	if ( m_jobPositions.contains( pos ) )
	{
		return 0;
	}

	QSharedPointer<Job> job( new Job );
	job->setType( type );
	job->setRequiredSkill( Global::util->requiredSkill( type ) );
	job->setPos( pos );
	job->setItem( item );
	job->setMaterial( materials.first() );
	job->setRotation( rotation );
	job->setNoJobSprite( noJobSprite );
	QString constructionID;
	auto dbjb = DB::job( type );
	if( dbjb )
	{
		job->setOrigWorkPosOffsets( dbjb->WorkPositions );
		constructionID = dbjb->ConstructionType; 
	}

	if ( !constructionID.isEmpty() )
	{
		QList<QVariantMap> components;
		if ( constructionID == "Workshop" )
		{
			components = DB::selectRows( "Workshops_Components", "ID", item );
		}
		else if ( constructionID == "Item" )
		{
			// items that exist as different base items before being constructed i.e. VerticleAxle is an Axle
			if ( DB::select( "Type", "Constructions", item ).toString() == "Item" )
			{
				components = DB::selectRows( "Constructions_Components", "ID", item );
			}
			else
			{
				QVariantMap comp;
				comp.insert( "ItemID", item );
				comp.insert( "Type", constructionID );
				components.push_back( comp );
			}
		}
		else
		{
			components = DB::selectRows( "Constructions_Components", "ID", item );
		}

		int cID = 0;
		for ( auto comp : components )
		{
			QString itemID = comp.value( "ItemID" ).toString();
			if ( !itemID.isEmpty() )
			{
				int amount = comp.value( "Amount" ).toInt();
				if ( amount == 0 )
				{
					amount = 1;
				}
				//if( itemID.startsWith( "$" ) )
				{
					// not used right now
				}
				//qDebug() << "require item " << itemID << materials[cID];
				int itemCount = g->inv()->itemCount( itemID, materials[cID] );
				if ( itemCount - amount < 0 )
				{
					//qDebug() << "require " << amount << " items " << itemID << materials[cID] << " there are " << itemCount << " in the world";
					g->wsm()->autoGenCraftJob( itemID, materials[cID], amount - itemCount );
				}

				job->addRequiredItem( amount, itemID, materials[cID], comp.value( "MaterialTypes" ).toStringList() );
				job->setComponentMissing( true );
				++cID;
			}
		}
	}

	else if ( type == "PlantTree" )
	{
		QVariantMap row = DB::selectRow( "Plants", item );
		//qDebug() << row.value("SeedItemID").toString() << row.value("Material").toString();
		job->addRequiredItem( 1, row.value( "SeedItemID" ).toString(), row.value( "Material" ).toString(), QStringList() );
		job->setComponentMissing( true );
	}

	m_jobList.insert( job->id(), job );

	insertIntoPositionHash( job->id() );

	m_returnedJobQueue.enqueue( job->id() );

	if ( g->sg() )
	{
		g->sg()->insertJob( job->id(), pos );
	}

	setJobSprites( job->id(), false, false );
	return job->id();
}

void JobManager::setJobAvailable( unsigned int jobID )
{
	setJobSprites( jobID, false, false );
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );
		job->setIsWorked( false );
	}
}

void JobManager::setJobBeingWorked( unsigned int jobID, bool hasNeededTool )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );

		if ( !job->noJobSprite() )
		{
			if ( hasNeededTool )
			{
				setJobSprites( jobID, true, false );
			}
		}
		job->setIsWorked( true );
	}
}

unsigned int JobManager::getJob( QStringList skills, unsigned int gnomeID, Position& gnomePos )
{
	for ( auto skillID : skills )
	{
		unsigned int jobID = 0;

		int skillInt = m_skillToInt.value( skillID );

		switch ( skillInt )
		{
			case SK_Hauling:
			{
				jobID = g->spm()->getJob();
				{
					if ( jobID )
					{
						return jobID;
					}
				}
			}
			break;
		}
		QElapsedTimer et;
		et.start();

		auto possibleJobIDs = m_jobIDs.value( skillID );
		if( m_workshopSkills.contains( skillID ) )
		{
			possibleJobIDs.push_front( "CraftAtWorkshop" );
		}
		unsigned int regionID = g->w()->regionMap().regionID( gnomePos );
		for ( int prio = 9; prio >= 0; --prio )
		{
			for ( auto jobID : possibleJobIDs )
			{
				if ( m_jobsPerType.contains( jobID ) )
				{
					QList<unsigned int> jobs = m_jobsPerType[jobID].values( prio );
					PriorityQueue<unsigned int, int> pq;

					if ( jobID == "RemoveFloor" || jobID == "BuildWall" || jobID == "DigHole" )
					{
						int walkableNeighbors = 0;
						for ( int i = 0; i < jobs.size(); ++i )
						{
							walkableNeighbors = g->w()->walkableNeighbors( m_jobList[jobs[i]]->pos() );
							pq.put( jobs[i], walkableNeighbors );
						}
						while ( !pq.empty() )
						{
							QSharedPointer<Job> job = m_jobList[pq.get()];
							if ( !job->isWorked() && !job->isCanceled() )
							{
								if ( requiredItemsAvail( job->id() ) && requiredToolExists( job->id() ) )
								{
									if ( isReachable( job->id(), regionID ) && !isEnclosedBySameType( job->id() ) )
									{
										//qDebug() << "getJob " <<  j->id();
										return job->id();
									}
								}
								else
								{
									for ( auto& type : m_jobsPerType )
									{
										type.remove( prio, job->id() );
									}
									m_returnedJobQueue.enqueue( job->id() );
								}
							}
						}
					}
					else
					{
						int dist = 0;
						for ( int i = 0; i < jobs.size(); ++i )
						{
							if( jobID == "CraftAtWorkshop" && m_jobList[jobs[i]]->requiredSkill() != skillID )
							{
								continue;
							}

							dist = m_jobList[jobs[i]]->distanceSquare( gnomePos, 10 );

							if ( dist == 1 )
							{
								QSharedPointer<Job> job = m_jobList[jobs[i]];
								if ( !job->isWorked() && !job->isCanceled() )
								{
									if ( requiredItemsAvail( job->id() ) && requiredToolExists( job->id() ) )
									{
										if ( isReachable( job->id(), 0 ) )
										{
											//qDebug() << "getJob " <<  j.id();
											return job->id();
										}
									}
									else
									{
										for ( auto& type : m_jobsPerType )
										{
											type.remove( prio, job->id() );
										}
										m_returnedJobQueue.enqueue( job->id() );
									}
								}
								continue;
							}
							pq.put( jobs[i], dist );
						}

						while ( !pq.empty() )
						{
							QSharedPointer<Job> job = m_jobList[pq.get()];
							if ( !job->isWorked() && !job->isCanceled() )
							{
								if ( requiredItemsAvail( job->id() ) && requiredToolExists( job->id() ) )
								{
									if ( isReachable( job->id(), regionID ) )
									{
										//qDebug() << "getJob " <<  j->id();
										return job->id();
									}
								}
								else
								{
									for ( auto& type : m_jobsPerType )
									{
										type.remove( prio, job->id() );
									}
									m_returnedJobQueue.enqueue( job->id() );
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

QSharedPointer<Job> JobManager::getJob( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		return m_jobList.value( jobID );
	}
	if ( g->spm()->hasJobID( jobID ) )
	{
		return g->spm()->getJob( jobID );
	}
	return nullptr;
}

QSharedPointer<Job> JobManager::getJobAtPos( Position pos )
{
	if ( m_jobPositions.contains( pos ) )
	{
		unsigned jobID = m_jobPositions.value( pos );
		return m_jobList.value( jobID );
	}

	return nullptr;
}

bool JobManager::isEnclosedBySameType( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job     = m_jobList.value( jobID );
		Position pos = job->pos();
		QString type = job->type();
		if ( m_jobPositions.contains( pos.northOf() ) )
		{
			if ( m_jobPositions.contains( pos.southOf() ) )
			{
				if ( m_jobPositions.contains( pos.eastOf() ) )
				{
					if ( m_jobPositions.contains( pos.westOf() ) )
					{
						if ( m_jobList[m_jobPositions[pos.northOf()]]->type() == type )
						{
							if ( m_jobList[m_jobPositions[pos.southOf()]]->type() == type )
							{
								if ( m_jobList[m_jobPositions[pos.eastOf()]]->type() == type )
								{
									if ( m_jobList[m_jobPositions[pos.westOf()]]->type() == type )
									{
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

bool JobManager::isReachable( unsigned int jobID, unsigned int regionID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job     = m_jobList.value( jobID );
		Position pos = job->pos();
		job->clearPossibleWorkPositions();
		// jobs on same tile
		auto wpl = job->origWorkPosOffsets();
		//qDebug() << "### get staging for " << pos.toString();
		for ( const auto& offset : wpl )
		{
			Position testPos( pos + offset );
			if ( g->w()->isWalkable( testPos ) )
			{
				if ( regionID == 0 )
				{
					job->addPossibleWorkPosition( testPos );
				}
				else
				{
					unsigned int workRegionID = g->w()->regionMap().regionID( testPos );
					if ( g->w()->regionMap().checkConnectedRegions( regionID, workRegionID ) )
					{
						job->addPossibleWorkPosition( testPos );
					}
				}
			}
		}
		//if ( job.possibleWorkPositions().empty() ) qDebug() << "not reachable";
		return job->possibleWorkPositions().size() > 0;
	}
	return false;
}

void JobManager::finishJob( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );

		QString type = job->type();
		setJobSprites( jobID, false, true );

		std::vector<Position> neighs;
		Position pos = job->pos();

		neighs.push_back( pos.northOf() );
		neighs.push_back( pos.eastOf() );
		neighs.push_back( pos.southOf() );
		neighs.push_back( pos.westOf() );
		neighs.push_back( pos.aboveOf() );
		neighs.push_back( pos.belowOf() );

		for ( auto p : neighs )
		{
			if ( m_jobPositions.contains( p ) )
			{
				unsigned int j = m_jobPositions.value( p );

				if ( isReachable( j, 0 ) && !m_jobList[j]->componentMissing() )
				{
					m_jobsPerType[m_jobList[j]->type()].insert( m_jobList[j]->priority(), j );
				}
			}
		}

		removeFromPositionHash( jobID );

		if ( g->sg() )
		{
			g->sg()->removeJob( jobID, job->pos() );
		}

		for ( auto& mtype : m_jobsPerType )
		{
			mtype.remove( job->priority(), jobID );
		}
		m_jobList.remove( jobID );
	}

	if ( g->spm()->finishJob( jobID ) )
	{
		return;
	}

}

void JobManager::setJobSprites( unsigned int jobID, bool busy, bool remove )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job     = m_jobList.value( jobID );
		if( !job || job->noJobSprite() )
		{
			return;
		}
		QString type = job->type();
		int rotation = job->rotation();
		QList<QPair<Sprite*, Position>> sprites;
		QList<QPair<Sprite*, Position>> spritesInv;

		if ( type == "BuildWall" || type == "BuildFloor" || type == "BuildRamp" || type == "BuildRampCorner" || type == "BuildWorkshop" || type == "BuildItem" || type == "BuildStairs" )
		{
			QList<QVariantMap> components;
			if ( type == "BuildWall" || type == "BuildFloor" || type == "BuildRamp" || type == "BuildRampCorner" || type == "BuildStairs" )
			{
				components = DB::selectRows( "Constructions_Sprites", "ID", job->item() );
			}
			else if ( type == "BuildWorkshop" )
			{
				components = DB::selectRows( "Workshops_Components", "ID", job->item() );
			}
			else if ( type == "BuildItem" )
			{
				QVariantMap sprite;
				sprite.insert( "SpriteID", DBH::spriteID( job->item() ) );
				sprite.insert( "Offset", "0 0 0" );
				sprite.insert( "Type", "Item" );
				components.push_back( sprite );
			}
			for ( auto component : components )
			{
				auto cm = component;
				Position offset( cm.value( "Offset" ).toString() );
				int rotX = offset.x;
				int rotY = offset.y;
				switch ( rotation )
				{
					case 1:
						offset.x = -1 * rotY;
						offset.y = rotX;
						break;
					case 2:
						offset.x = -1 * rotX;
						offset.y = -1 * rotY;
						break;
					case 3:
						offset.x = rotY;
						offset.y = -1 * rotX;
						break;
				}

				if ( !cm.value( "SpriteID" ).toString().isEmpty() )
				{
					QString mat = job->material();
					if ( mat == "any" )
					{
						mat = "None";
					}
					if ( !cm.value( "SpriteIDOverride" ).toString().isEmpty() )
					{
						cm.insert( "SpriteID", cm.value( "SpriteIDOverride" ).toString() );
					}
					Sprite* sprite  = g->sf()->createSprite( cm["SpriteID"].toString(), { mat } );
					sprite->opacity = 0.5;

					bool isFloor = false;
					if ( cm.contains( "Type" ) )
					{
						if ( cm["Type"].toString() == "Floor" || cm["Type"].toString() == "StairsTop" )
						{
							isFloor = true;
						}
					}

					unsigned int spriteUID = sprite->uID;
					/*
					if( sprite->anim )
					{
						spriteUID += 2048;
					}
					*/
					if ( isFloor )
					{
						if ( remove )
						{
							g->w()->clearJobSprite( job->pos() + offset, true );
						}
						else
						{
							g->w()->setJobSprite( job->pos() + offset, spriteUID, rotation, isFloor, jobID, busy );
						}
					}
					else
					{
						QString rots = cm["WallRotation"].toString();
						int rot      = 0;
						if ( rots == "FL" )
							rot = 1;
						else if ( rots == "BL" )
							rot = 2;
						else if ( rots == "BR" )
							rot = 3;
						if ( remove )
						{
							g->w()->clearJobSprite( job->pos() + offset, false );
						}
						else
						{
							g->w()->setJobSprite( job->pos() + offset, spriteUID, ( rotation + rot ) % 4, isFloor, jobID, busy );
						}
					}
				}
				else
				{
					Sprite* sprite  = g->sf()->createSprite( "SolidSelectionFloor", { "None" } );
					sprite->opacity = 0.5;
					if ( remove )
					{
						g->w()->clearJobSprite( job->pos() + offset, true );
					}
					else
					{
						g->w()->setJobSprite( job->pos() + offset, sprite->uID, rotation, true, jobID, busy );
					}
				}
			}
		}
		else
		{

			QList<QVariantMap> spriteIds = DB::selectRows( "Jobs_SpriteID", "ID", type );

			if ( !spriteIds.empty() )
			{
				for ( const auto& entry : spriteIds )
				{
					if ( !entry.value( "SpriteID" ).toString().isEmpty() )
					{
						QString spriteID = entry["SpriteID"].toString();
						Sprite* sprite   = g->sf()->createSprite( spriteID, { "None" } );
						sprite->opacity  = 0.5;
						Position offset( 0, 0, 0 );
						if ( entry.contains( "Offset" ) )
						{
							offset = Position( entry["Offset"].toString() );
						}
						bool floor = false;
						if ( entry.contains( "Type" ) )
						{
							if ( entry["Type"].toString() == "Floor" )
							{
								floor = true;
							}
						}
						unsigned int spriteUID = sprite->uID;
						/*
						if( sprite->anim )
						{
							spriteUID += 2048;
						}
						*/
						if ( remove )
						{
							g->w()->clearJobSprite( job->pos() + offset, floor );
						}
						else
						{
							g->w()->setJobSprite( job->pos() + offset, spriteUID, rotation, floor, jobID, busy );
						}
					}
				}
			}
		}
	}
}

void JobManager::giveBackJob( unsigned int jobID )
{
	if ( m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );

		if ( job->isCanceled() || job->destroyOnAbort() )
		{
			job->setIsWorked( false );
			cancelJob( job->pos() );
			return;
		}

		if( job->type() == "LeadAnimalToPasture" || job->type() == "ButcherAnimal" || job->type() == "HarvestAnimal" )
		{
			if( job->animal() == 0 )
			{
				job->setIsWorked( false );
				cancelJob( job->pos() );
				return;
			}
		}

		//TODO clear stockpile reserve
		job->setIsWorked( false );
		job->setWorkedBy( 0 );
		job->clearPossibleWorkPositions();
		job->setComponentMissing( true );

		for ( auto& type : m_jobsPerType )
		{
			type.remove( job->priority(), jobID );
		}
		setJobSprites( jobID, false, false );

		m_returnedJobQueue.enqueue( jobID );
		return;
	}

	if ( g->spm()->giveBackJob( jobID ) )
	{
		return;
	}
}

void JobManager::cancelJob( const Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos ) )
	{
		jobID = m_jobPositions.value( pos );
	}
	else
	{
		jobID = g->w()->jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 && m_jobList.contains( jobID ) )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );

		if ( job->isWorked() )
		{
			job->setCanceled();
			job->setAborted( true );
		}
		else
		{
			if ( job->type() == "SoundAlarm" )
			{
				GameState::alarm       = 0;
				GameState::alarmRoomID = 0;
			}

			setJobSprites( jobID, false, true );

			removeFromPositionHash( jobID );
			if ( g->sg() )
			{
				g->sg()->removeJob( jobID, job->pos() );
			}
			for ( auto& type : m_jobsPerType )
			{
				type.remove( job->priority(), jobID );
			}
			m_jobList.remove( jobID );
		}
	}
	else if ( jobID == 0 )
	{
		// No job found at position — clean up any stale job sprites
		g->w()->clearJobSprite( pos, true );
	}
}

void JobManager::deleteJob( unsigned int jobID )
{
	if ( jobID != 0 && m_jobList.contains( jobID ) )
	{
		if ( m_jobList.value( jobID )->isWorked() )
		{
			m_jobList.value( jobID )->setCanceled();
		}
		else
		{
			QSharedPointer<Job> job = m_jobList.value( jobID );

			if ( job->type() == "SoundAlarm" )
			{
				GameState::alarm       = 0;
				GameState::alarmRoomID = 0;
			}

			setJobSprites( jobID, false, true );

			removeFromPositionHash( jobID );
			if ( g->sg() )
			{
				g->sg()->removeJob( jobID, job->pos() );
			}
			for ( auto& type : m_jobsPerType )
			{
				type.remove( job->priority(), jobID );
			}
			m_jobList.remove( jobID );
		}
	}
}

void JobManager::deleteJobAt( const Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos ) )
	{
		jobID = m_jobPositions.value( pos );
	}
	else
	{
		jobID = g->w()->jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 && m_jobList.contains( jobID ) )
	{
		deleteJob( jobID );
	}
}

QString JobManager::jobManagerInfo()
{
	QString out;

	out += QString::number( m_jobList.size() );
	return out;
}

void JobManager::raisePrio( Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos ) )
	{
		jobID = m_jobPositions.value( pos );
	}
	else
	{
		jobID = g->w()->jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );
		if ( job->priority() < 9 )
		{
			if ( m_jobsPerType[job->type()].count( job->priority(), job->id() ) )
			{
				//job is in m_jobsPerType so move it
				m_jobsPerType[job->type()].remove( job->priority(), job->id() );
				m_jobsPerType[job->type()].insert( job->priority() + 1, job->id() );
			}
			job->raisePrio();
		}
	}
}

void JobManager::lowerPrio( Position& pos )
{
	unsigned int jobID = 0;
	if ( m_jobPositions.contains( pos ) )
	{
		jobID = m_jobPositions.value( pos );
	}
	else
	{
		jobID = g->w()->jobSprite( pos ).value( "JobID" ).toUInt();
	}

	if ( jobID != 0 )
	{
		QSharedPointer<Job> job = m_jobList.value( jobID );
		if ( job->priority() > 0 )
		{
			if ( m_jobsPerType[job->type()].count( job->priority(), job->id() ) )
			{
				//job is in m_jobsPerType so move it
				m_jobsPerType[job->type()].remove( job->priority(), job->id() );
				m_jobsPerType[job->type()].insert( job->priority() - 1, job->id() );
			}
			job->lowerPrio();
		}
	}
}

// --- Push Model (Phase B) ---

void JobManager::registerGnomeSkill( unsigned int gnomeID, const QString& skill )
{
	if ( !m_gnomesBySkill.contains( skill, gnomeID ) )
	{
		m_gnomesBySkill.insert( skill, gnomeID );
	}
}

void JobManager::unregisterGnomeSkill( unsigned int gnomeID, const QString& skill )
{
	m_gnomesBySkill.remove( skill, gnomeID );
}

void JobManager::pushJobToGnomes( unsigned int jobID )
{
	auto job = m_jobList.value( jobID );
	if ( !job || job->isWorked() || job->isCanceled() )
		return;

	QString skill = job->requiredSkill();
	auto eligibleGnomes = m_gnomesBySkill.values( skill );
	if ( eligibleGnomes.isEmpty() )
	{
		m_overflowPool.append( jobID );
		return;
	}

	Position jobPos = job->pos();

	// Spatial filter: find K=5 nearest eligible gnomes using expanding rings
	QSet<unsigned int> eligibleSet( eligibleGnomes.begin(), eligibleGnomes.end() );
	QVector<QPair<int, unsigned int>> candidates; // <distSq, gnomeID>

	for ( int ring = 0; ring <= 3 && candidates.size() < 5; ++ring )
	{
		QVector<unsigned int> nearby;
		if ( g->sg() )
		{
			g->sg()->entitiesInRadius( jobPos, ring, nearby );
		}

		for ( auto entityID : nearby )
		{
			if ( !eligibleSet.contains( entityID ) ) continue;

			Gnome* gn = g->gm()->gnome( entityID );
			if ( !gn || gn->isDead() ) continue;

			Position gnPos = gn->getPos();
			int dx = gnPos.x - jobPos.x;
			int dy = gnPos.y - jobPos.y;
			int dz = gnPos.z - jobPos.z;
			int distSq = dx * dx + dy * dy + dz * dz;

			candidates.append( qMakePair( distSq, entityID ) );
		}
	}

	if ( candidates.isEmpty() )
	{
		m_overflowPool.append( jobID );
		return;
	}

	// Sort by distance and take top 5
	std::sort( candidates.begin(), candidates.end() );
	int pushCount = qMin( 5, candidates.size() );

	for ( int i = 0; i < pushCount; ++i )
	{
		unsigned int gnomeID = candidates[i].second;
		Gnome* gn = g->gm()->gnome( gnomeID );
		if ( !gn ) continue;

		// Insert into pending queue sorted by job priority (highest first)
		int prio = job->priority();
		int insertIdx = 0;
		for ( ; insertIdx < gn->pendingJobs().size(); ++insertIdx )
		{
			auto pendJob = m_jobList.value( gn->pendingJobs()[insertIdx] );
			if ( !pendJob || pendJob->priority() < prio )
				break;
		}
		gn->pendingJobs().insert( insertIdx, jobID );

		// Cap pending queue at 10
		while ( gn->pendingJobs().size() > 10 )
		{
			gn->pendingJobs().removeLast();
		}

		// Wake sleeping gnomes when a job is pushed to them
		if ( gn->isSleeping() )
		{
			g->gm()->wakeGnome( gn );
		}
	}
}

void JobManager::processOverflowPool()
{
	if ( m_overflowPool.isEmpty() ) return;

	QList<unsigned int> remaining;
	for ( auto jobID : m_overflowPool )
	{
		auto job = m_jobList.value( jobID );
		if ( !job || job->isWorked() || job->isCanceled() )
			continue;

		QString skill = job->requiredSkill();
		if ( m_gnomesBySkill.contains( skill ) )
		{
			// Re-attempt push now that gnomes may have moved
			pushJobToGnomes( jobID );
		}
		else
		{
			remaining.append( jobID );
		}
	}
	m_overflowPool = remaining;
}

void JobManager::rebuildPushState()
{
	m_gnomesBySkill.clear();
	m_overflowPool.clear();

	// Register all gnome skills
	for ( auto* gn : g->gm()->gnomes() )
	{
		for ( const auto& skill : gn->skillPrios() )
		{
			registerGnomeSkill( gn->id(), skill );
		}
	}

	// Push all unclaimed jobs
	for ( auto it = m_jobList.constBegin(); it != m_jobList.constEnd(); ++it )
	{
		auto job = it.value();
		if ( !job->isWorked() && !job->isCanceled() )
		{
			pushJobToGnomes( it.key() );
		}
	}
}

// --- Spatial Fallback Search (Phase H) ---

unsigned int JobManager::spatialFallbackSearch( const QStringList& skills, const Position& pos, unsigned int gnomeID )
{
	Q_UNUSED( gnomeID );
	unsigned int regionID = g->w()->regionMap().regionID( pos );
	QSet<QString> skillSet( skills.begin(), skills.end() );

	// Expanding ring search: own cell, then adjacent, then 2-ring, up to 5 rings
	for ( int ring = 0; ring <= 5; ++ring )
	{
		QVector<unsigned int> nearbyJobs;
		if ( g->sg() )
		{
			g->sg()->jobsInRadius( pos, ring, nearbyJobs );
		}

		for ( auto jobID : nearbyJobs )
		{
			auto job = m_jobList.value( jobID );
			if ( !job || job->isWorked() || job->isCanceled() )
				continue;
			if ( !skillSet.contains( job->requiredSkill() ) )
				continue;
			if ( !isReachable( jobID, regionID ) )
				continue;
			if ( !requiredItemsAvail( jobID ) )
				continue;

			return jobID;
		}
	}

	return 0;
}
