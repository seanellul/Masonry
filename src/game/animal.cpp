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
#include "animal.h"

#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/game.h"
#include "../game/creaturemanager.h"
#include "../game/farmingmanager.h"
#include "../game/gnomemanager.h"
#include "../game/inventory.h"
#include "../game/stockpilemanager.h"
#include "../game/stockpile.h"
#include "../game/plant.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../game/world.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QPainter>
#include <QPixmap>

Animal::Animal( QString species, Position& pos, Gender gender, bool adult, Game* game ) :
	Creature( pos, species, gender, species, game )
{
	initTaskMap();

	QVariantMap avm = DB::selectRow( "Animals", m_species );

	m_aquatic  = avm.value( "Aquatic" ).toBool();
	m_btName   = avm.value( "BehaviorTree" ).toString();
	m_preyList = avm.value( "Prey" ).toString().split( "|" );
	m_diet     = avm.value( "Food" ).toString();

	m_isMulti = avm.value( "IsMulti" ).toBool();

	int hungerRand = ( rand() % 20 ) - 10;
	m_hunger       = 100 + hungerRand;

	if ( adult )
	{
		int numStates = DB::selectRows( "Animals_States", "ID", m_species ).size();
		setState( numStates - 1 );
	}
	else
	{
		setState( 0 );
	}

	m_type = CreatureType::ANIMAL;
}

Animal::Animal( QVariantMap in, Game* game ) :
	Creature( in, game ),
	//bool m_tame = false;
	m_tame( in.value( "Tame" ).toBool() ),
	m_birthTick( in.value( "BirthTick" ).value<quint64>() ),
	//unsigned int m_pastureID = 0;
	m_pastureID( in.value( "PastureID" ).toUInt() ),
	m_inJob( in.value( "InJob" ).toUInt() ),
	m_produceTick( in.value( "ProduceTick" ).value<quint64>() ),
	m_lastSex( in.value( "LastSex" ).value<quint64>() ),
	m_currentPrey( in.value( "CurrentPrey" ).toUInt() ),
	m_inShed( in.value( "InShed" ).toUInt() ),
	m_producedAmount( in.value( "ProducedAmount" ).value<quint8>() ),
	m_produce( in.value( "Produce" ).toString() ),
	m_corpseToEat( in.value( "CorpseToEat" ).toUInt() ),
	m_dye( in.value( "Dye" ).toString() ),
	m_isMulti( in.value( "IsMulti" ).toBool() ),
	m_toButcher( in.value( "ToButcher" ).toBool() )
{
	initTaskMap();

	QVariantMap avm = DB::selectRow( "Animals", m_species );

	m_stateMap = DB::selectRows( "Animals_States", "ID", m_species ).at( m_state );

	m_aquatic  = avm.value( "Aquatic" ).toBool();
	m_preyList = avm.value( "Prey" ).toString().split( "|" );
	m_diet     = avm.value( "Food" ).toString();

	m_hunger = qMax( -20.f, in.value( "Hunger" ).toFloat() );

	setState( m_state );
	m_stateChangeTick = in.value( "sct" ).value<quint64>();

	checkInJob();
}

void Animal::serialize( QVariantMap& out )
{
	checkInJob(); // workaround until i find where the job leak is

	// animal
	out.insert( "Tame", m_tame );
	out.insert( "BirthTick", m_birthTick );
	out.insert( "PastureID", m_pastureID );
	out.insert( "InJob", m_inJob );
	out.insert( "Hunger", m_hunger );

	out.insert( "BTName", m_btName );

	out.insert( "BirthTick", m_birthTick );
	out.insert( "ProduceTick", m_produceTick );
	out.insert( "LastSex", m_lastSex );
	out.insert( "CurrentPrey", m_currentPrey );
	out.insert( "InShed", m_inShed );
	out.insert( "ProducedAmount", m_producedAmount );
	out.insert( "Produce", m_produce );
	out.insert( "CorpseToEat", m_corpseToEat );
	out.insert( "Dye", m_dye );
	out.insert( "IsMulti", m_isMulti );
	out.insert( "ToButcher", m_toButcher );
	Creature::serialize( out );
}

Animal::Animal() :
	Creature( Position(), "", Gender::UNDEFINED, "", g )
{
	initTaskMap();
}

Animal::~Animal()
{
}

void Animal::checkInJob()
{
	if ( m_inJob )
	{
		if ( !g->jm()->getJob( m_inJob ) )
		{
			//qDebug() << "Animal:" << m_id << "has inJobID:" << m_inJob << " that doesn't exist";
			m_inJob    = 0;
			m_followID = 0;
		}
	}
}

void Animal::init()
{
	g->w()->insertCreatureAtPosition( m_position, m_id );


	m_anatomy.init( "Animal", m_aquatic );

	loadBehaviorTree( m_btName );

	if ( m_btBlackBoard.contains( "State" ) )
	{
		QVariantMap btm = m_btBlackBoard.value( "State" ).toMap();
		if ( m_behaviorTree )
		{
			m_behaviorTree->deserialize( btm );
			m_btBlackBoard.remove( "State" );
		}
	}
}

void Animal::updateSprite()
{
	if ( m_isMulti )
	{
		m_multiSprites.clear();
		QString spriteName = m_stateMap.value( "SpriteID" ).toString();
		switch ( m_facing )
		{
			case 0:
				spriteName += "FR";
				break;
			case 1:
				spriteName += "FL";
				break;
			case 2:
				spriteName += "BL";
				break;
			case 3:
				spriteName += "BR";
				break;
		}
		auto rows = DB::selectRows( "Creature_Layouts", spriteName );
		for ( const auto& row : rows )
		{
			auto spriteSID = row.value( "Sprite" ).toString();
			Sprite* sprite = g->sf()->createAnimalSprite( spriteSID );
			if ( sprite )
			{
				//m_multiSprites.append( QPair<Position, unsigned int>( m_position + Position( row.value( "Offset" ) ), sprite->uID ) );
				m_multiSprites.append( { m_position + Position( row.value( "Offset" ) ), sprite->uID } );
			}
		}
	}
	else
	{
		Sprite* sprite = nullptr;
		if ( m_dye.isEmpty() )
		{
			sprite = g->sf()->createAnimalSprite( m_stateMap.value( "SpriteID" ).toString() );
		}
		else
		{
			sprite = g->sf()->createAnimalSprite( m_stateMap.value( "SpriteID" ).toString() + m_dye );
		}
		if ( sprite )
		{
			m_hasTransparency = sprite->hasTransp;
			m_spriteID        = sprite->uID;
		}
		else
		{
			qDebug() << "*ERROR* failed to create sprite for " << m_stateMap.value( "ID" ).toString() << " state: " << m_stateMap.value( "ID2" ).toString();
		}
	}
}

void Animal::setDye( QString dye )
{
	m_dye = dye;
	updateSprite();
}

QString Animal::dye()
{
	return m_dye;
}

void Animal::updateMoveSpeed()
{
	//TODO add animal values to movespeed table
	//int skill = getSkillLevel( "Hauling" );
	int speed   = 50; //DB::execQuery( "SELECT Speed FROM MoveSpeed WHERE CREATURE = \"Gnome\" AND SkillLevel = \"" + QString::number( skill ) + "\"" ).toInt();
	m_moveSpeed = qMax( 30, speed );
}

void Animal::initTaskMap()
{
	using namespace std::placeholders;
	//m_behaviors.insert( "", std::bind( &Animal::condition, this, _1 ) );
	//m_behaviors.insert( "", std::bind( &Animal::action, this, _1 ) );
	m_behaviors.insert( "IsMale", std::bind( &Animal::conditionIsMale, this, _1 ) );
	m_behaviors.insert( "IsFemale", std::bind( &Animal::conditionIsFemale, this, _1 ) );
	m_behaviors.insert( "IsDay", std::bind( &Animal::conditionIsDay, this, _1 ) );
	m_behaviors.insert( "IsNight", std::bind( &Animal::conditionIsNight, this, _1 ) );
	m_behaviors.insert( "IsEgg", std::bind( &Animal::conditionIsEgg, this, _1 ) );
	m_behaviors.insert( "IsAdult", std::bind( &Animal::conditionIsAdult, this, _1 ) );
	m_behaviors.insert( "IsYoung", std::bind( &Animal::conditionIsYoung, this, _1 ) );
	m_behaviors.insert( "IsCarnivore", std::bind( &Animal::conditionIsCarnivore, this, _1 ) );
	m_behaviors.insert( "IsHerbivore", std::bind( &Animal::conditionIsHerbivore, this, _1 ) );
	m_behaviors.insert( "IsHungry", std::bind( &Animal::conditionIsHungry, this, _1 ) );
	m_behaviors.insert( "IsEggLayer", std::bind( &Animal::conditionIsEggLayer, this, _1 ) );
	m_behaviors.insert( "IsProducer", std::bind( &Animal::conditionIsProducer, this, _1 ) );
	m_behaviors.insert( "IsPregnant", std::bind( &Animal::conditionIsPregnant, this, _1 ) );
	m_behaviors.insert( "IsReadyToGiveBirth", std::bind( &Animal::conditionIsReadyToGiveBirth, this, _1 ) );
	m_behaviors.insert( "IsWoodVermin", std::bind( &Animal::conditionIsWoodVermin, this, _1 ) );
	m_behaviors.insert( "IsInShed", std::bind( &Animal::conditionIsInShed, this, _1 ) );
	m_behaviors.insert( "IsOnPasture", std::bind( &Animal::conditionIsOnPasture, this, _1 ) );
	m_behaviors.insert( "TargetAdjacent", std::bind( &Animal::conditionTargetAdjacent, this, _1 ) );

	m_behaviors.insert( "LayEgg", std::bind( &Animal::actionLayEgg, this, _1 ) );
	m_behaviors.insert( "Produce", std::bind( &Animal::actionProduce, this, _1 ) );
	m_behaviors.insert( "RandomMove", std::bind( &Animal::actionRandomMove, this, _1 ) );
	m_behaviors.insert( "TryHaveSex", std::bind( &Animal::actionTryHaveSex, this, _1 ) );
	m_behaviors.insert( "GiveBirth", std::bind( &Animal::actionGiveBirth, this, _1 ) );
	m_behaviors.insert( "FindPrey", std::bind( &Animal::actionFindPrey, this, _1 ) );
	m_behaviors.insert( "KillPrey", std::bind( &Animal::actionKillPrey, this, _1 ) );
	m_behaviors.insert( "EatPrey", std::bind( &Animal::actionEatPrey, this, _1 ) );
	m_behaviors.insert( "Move", std::bind( &Animal::actionMove, this, _1 ) );
	m_behaviors.insert( "FindRetreat", std::bind( &Animal::actionFindRetreat, this, _1 ) );
	m_behaviors.insert( "Sleep", std::bind( &Animal::actionSleep, this, _1 ) );
	m_behaviors.insert( "FindShed", std::bind( &Animal::actionFindShed, this, _1 ) );
	m_behaviors.insert( "EnterShed", std::bind( &Animal::actionEnterShed, this, _1 ) );
	m_behaviors.insert( "LeaveShed", std::bind( &Animal::actionLeaveShed, this, _1 ) );
	m_behaviors.insert( "FindRandomPastureField", std::bind( &Animal::actionFindRandomPastureField, this, _1 ) );

	m_behaviors.insert( "GuardDogGetTarget", std::bind( &Animal::actionGuardDogGetTarget, this, _1 ) );
	m_behaviors.insert( "GetTarget", std::bind( &Animal::actionGetTarget, this, _1 ) );
	m_behaviors.insert( "AttackTarget", std::bind( &Animal::actionAttackTarget, this, _1 ) );

	m_behaviors.insert( "Graze", std::bind( &Animal::actionGraze, this, _1 ) );

	m_behaviors.insert( "RandomMoveBig", std::bind( &Animal::actionRandomMoveBig, this, _1 ) );
}

void Animal::setState( int state )
{
	m_state = state;

	m_stateMap = DB::selectRows( "Animals_States", "ID", m_species ).at( state );

	loadBehaviorTree( m_stateMap.value( "BehaviorTree" ).toString() );

	updateSprite();

	int days = m_stateMap.value( "DaysToNextState" ).toInt();
	if ( days )
	{
		m_stateChangeTick = GameState::tick + ( Global::util->ticksPerDayRandomized( 5 ) * days );
	}
	else
	{
		m_stateChangeTick = 0;
	}

	QString behaviorID = m_species + m_stateMap.value( "ID2" ).toString();

	auto behaviorList = DB::selectRows( "Animals_States_Behavior", behaviorID );
	for ( const auto& vBehavior : behaviorList )
	{
		QString behavior = vBehavior.value( "ID2" ).toString();
		m_stateMap.insert( behavior, vBehavior );
	}

	m_isProducer = [this]() {
		if ( m_stateMap.contains( "Producer" ) )
		{
			QVariantMap def = m_stateMap.value( "Producer" ).toMap();
			if ( def.contains( "RequiredGender" ) )
			{
				Gender g = def.value( "RequiredGender" ).toString() == "male" ? Gender::MALE : Gender::FEMALE;
				if ( m_gender != g )
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}();

	m_isEggLayer = [this]() {
		if ( m_gender == Gender::FEMALE )
		{
			if ( m_stateMap.contains( "EggLayer" ) )
			{
				return true;
			}
		}
		return false;
	}();

	m_isGrazer = [this]() {
		if ( m_stateMap.contains( "Grazing" ) )
		{
			return true;
		}
		return false;
	}();

	m_isEgg   = isEgg();
	m_isYoung = isYoung();
	m_isAdult = isAdult();

	m_immobile            = m_stateMap.value( "Immobile" ).toBool();
	m_renderParamsChanged = true;

	m_foodValue = m_stateMap.value( "Grazing" ).toMap().value( "FoodValue" ).toInt();
}

CreatureTickResult Animal::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	processCooldowns( tickNumber );

	m_anatomy.setFluidLevelonTile( g->w()->fluidLevel( m_position ) );

	if ( m_anatomy.statusChanged() )
	{
		auto status = m_anatomy.status();
		if ( status & AS_DEAD )
		{
			Global::logger().log( LogType::COMBAT, "The " + m_name + " died. Bummer!", m_id );
			die();
			// TODO check for other statuses
		}
	}

	if ( m_toDestroy )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::TODESTROY;
	}
	if ( isDead() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::DEAD;
	}

	Position oldPos = m_position;

	if ( m_followID && !m_followPosition.isZero() && m_position != m_followPosition )
	{
		m_position = m_followPosition;
		move( oldPos );
		m_lastOnTick = tickNumber;
		return CreatureTickResult::OK;
	}

	if ( minuteChanged && !isEgg() )
	{
		m_hunger = m_hunger - 0.075;

		// Starvation death
		if ( m_hunger <= -20.0 )
		{
			Global::logger().log( LogType::WILDLIFE, "A " + S::s( "$CreatureName_" + m_species ) + " has starved to death.", m_id, m_position.x, m_position.y, m_position.z );
			die();
			return CreatureTickResult::DEAD;
		}

		// Log aggression onset
		bool canEatMeat = m_diet.contains( "Meat" );
		if ( m_hunger <= 10.0 && !m_tame && !m_starvingAggro && canEatMeat )
		{
			m_starvingAggro = true;
			Global::logger().log( LogType::WILDLIFE, "A starving " + S::s( "$CreatureName_" + m_species ) + " has become aggressive!", m_id, m_position.x, m_position.y, m_position.z );
		}
		else if ( m_hunger > 30.0 && m_starvingAggro )
		{
			m_starvingAggro = false;
			m_aggroList.clear();
			m_currentAttackTarget = 0;
		}
	}

	if ( m_stateChangeTick != 0 && tickNumber >= m_stateChangeTick )
	{
		++m_state;
		setState( m_state );
	}

	setThoughtBubble( "" );

	// =====================================================================
	// DESPERATION BEHAVIOR — priority-based food seeking
	// Priority: 1) Eat corpse in hand → 2) Find food items → 3) Find corpses
	//           → 4) Hunt weaker animals → 5) LAST RESORT: attack gnome
	// No cannibalism. After killing, eat the corpse before seeking more.
	// =====================================================================
	bool desperateMode = ( m_hunger < 20.0 && !m_tame && !isEgg() );

	if ( desperateMode )
	{
		bool canEatMeat = m_diet.contains( "Meat" );
		setThoughtBubble( "Hungry" );

		// ---- Currently eating a corpse? Continue eating. ----
		if ( m_desperateAction == DesperateAction::Eating && m_corpseToEat )
		{
			// Eat one body part worth of nutrition per tick
			if ( !m_corpsePartsNutrition.isEmpty() )
			{
				int nutrition = m_corpsePartsNutrition.takeFirst();
				m_hunger += nutrition;
				m_eatingTicks++;
			}

			// Done eating (all parts consumed or hunger satisfied)
			if ( m_corpsePartsNutrition.isEmpty() || m_hunger >= 80 )
			{
				// Destroy corpse, leave bones
				Position corpsePos( g->inv()->getItemPos( m_corpseToEat ) );
				g->inv()->createItem( corpsePos, "Bone", g->inv()->materialSID( m_corpseToEat ) + "Bone" );
				g->inv()->destroyObject( m_corpseToEat );
				m_corpseToEat = 0;
				m_desperateAction = DesperateAction::None;
				m_eatingTicks = 0;
				Global::logger().log( LogType::WILDLIFE, S::s( "$CreatureName_" + m_species ) + " finished feeding.", m_id, m_position.x, m_position.y, m_position.z );
			}
			m_lastOnTick = tickNumber;
			return CreatureTickResult::OK;
		}

		// ---- Need to find food. Use priority chain (only when no active path). ----
		if ( m_currentPath.empty() && m_desperateAction != DesperateAction::Eating && m_hunger < 10.0 )
		{
			m_desperateAction = DesperateAction::None;
			m_currentAttackTarget = 0;
			m_foodTarget = 0;
			m_preyTarget = 0;

			// Priority 1: Find existing corpse nearby to eat
			if ( canEatMeat )
			{
				auto items = g->inv()->getClosestItems( m_position, false, "AnimalCorpse", "any", 1 );
				if ( items.isEmpty() )
					items = g->inv()->getClosestItems( m_position, false, "GnomeCorpse", "any", 1 );

				if ( !items.isEmpty() )
				{
					m_foodTarget = items.first();
					Position itemPos( g->inv()->getItemPos( m_foodTarget ) );
					g->pf()->getPath( m_id, m_position, itemPos, true, m_currentPath );
					m_desperateAction = DesperateAction::SeekingCorpse;
				}
			}

			// Priority 2: Find food items on ground matching diet
			if ( m_desperateAction == DesperateAction::None )
			{
				for ( const auto& foodType : m_diet.split( "|" ) )
				{
					auto items = g->inv()->getClosestItems( m_position, false, foodType, "any", 1 );
					if ( !items.isEmpty() )
					{
						m_foodTarget = items.first();
						Position itemPos( g->inv()->getItemPos( m_foodTarget ) );
						g->pf()->getPath( m_id, m_position, itemPos, true, m_currentPath );
						m_desperateAction = DesperateAction::SeekingFood;
						break;
					}
				}
			}

			// Priority 3: Hunt a weaker animal (not same species, prefer prey list)
			if ( m_desperateAction == DesperateAction::None && canEatMeat )
			{
				Animal* bestPrey = nullptr;
				int bestDist = 999999;
				for ( auto* animal : g->cm()->animals() )
				{
					if ( animal->id() == m_id ) continue;
					if ( animal->isDead() ) continue;
					if ( animal->species() == m_species ) continue; // no cannibalism
					// Prefer prey from prey list, but accept any smaller animal
					int attackVal = m_stateMap.value( "Attack" ).toInt();
					int preyAttack = 3; // assume small if unknown
					bool isPreferredPrey = m_preyList.contains( animal->species() );
					if ( !isPreferredPrey && !animal->isTame() )
					{
						// Skip animals that are as strong or stronger
						// (rough estimate: only hunt things weaker than us)
						auto preyState = DB::selectRows( "Animals_States", "ID", animal->species() );
						if ( !preyState.isEmpty() )
							preyAttack = preyState.last().value( "Attack" ).toInt();
						if ( preyAttack >= attackVal ) continue;
					}
					Position aPos = animal->getPos();
					int dist = abs( m_position.x - aPos.x ) + abs( m_position.y - aPos.y ) + abs( m_position.z - aPos.z );
					// Prefer prey list targets, use distance as tiebreaker
					int priority = isPreferredPrey ? dist : dist + 100;
					if ( priority < bestDist )
					{
						bestDist = priority;
						bestPrey = animal;
					}
				}
				if ( bestPrey && bestDist < 160 )
				{
					m_preyTarget = bestPrey->id();
					m_currentAttackTarget = bestPrey->id();
					setCurrentTarget( bestPrey->getPos() );
					g->pf()->getPath( m_id, m_position, bestPrey->getPos(), true, m_currentPath );
					m_desperateAction = DesperateAction::HuntingPrey;
				}
			}

			// Priority 4: LAST RESORT — attack nearest gnome
			if ( m_desperateAction == DesperateAction::None && canEatMeat )
			{
				Gnome* nearest = nullptr;
				int nearestDist = 999999;
				for ( auto gn : g->gm()->gnomes() )
				{
					if ( gn->isDead() ) continue;
					Position gnPos = gn->getPos();
					int dist = abs( m_position.x - gnPos.x ) + abs( m_position.y - gnPos.y ) + abs( m_position.z - gnPos.z );
					if ( dist < nearestDist )
					{
						nearestDist = dist;
						nearest = gn;
					}
				}
				if ( nearest && nearestDist < 60 )
				{
					m_currentAttackTarget = nearest->id();
					setCurrentTarget( nearest->getPos() );
					g->pf()->getPath( m_id, m_position, nearest->getPos(), true, m_currentPath );
					m_desperateAction = DesperateAction::HuntingGnome;
				}
			}

			// Herbivore fallback: path to nearest stockpile
			if ( m_desperateAction == DesperateAction::None && !canEatMeat )
			{
				int nearestDist = 999999;
				Position nearestSP;
				bool found = false;
				for ( auto spID : g->spm()->allStockpiles() )
				{
					Stockpile* sp = g->spm()->getStockpile( spID );
					if ( sp && !sp->getFields().isEmpty() )
					{
						Position spPos( sp->getFields().firstKey() );
						int dist = abs( m_position.x - spPos.x ) + abs( m_position.y - spPos.y ) + abs( m_position.z - spPos.z );
						if ( dist < nearestDist ) { nearestDist = dist; nearestSP = spPos; found = true; }
					}
				}
				if ( found && nearestDist < 60 )
				{
					setCurrentTarget( nearestSP );
					g->pf()->getPath( m_id, m_position, nearestSP, true, m_currentPath );
					m_desperateAction = DesperateAction::SeekingFood;
				}
			}
		}

		// ---- Arrived at target: execute action ----
		if ( m_currentPath.empty() && m_desperateAction != DesperateAction::None && m_desperateAction != DesperateAction::Eating )
		{
			if ( m_desperateAction == DesperateAction::SeekingFood && m_foodTarget )
			{
				// Pick up and eat the food item
				m_hunger += 20; // food items give flat nutrition
				g->inv()->destroyObject( m_foodTarget );
				m_foodTarget = 0;
				m_desperateAction = DesperateAction::None;
			}
			else if ( m_desperateAction == DesperateAction::SeekingCorpse && m_foodTarget )
			{
				// Start eating the corpse body-part by body-part
				m_corpseToEat = m_foodTarget;
				m_foodTarget = 0;
				g->inv()->setInJob( m_corpseToEat, m_id );

				// Calculate nutrition from body parts (HP/2 per external part)
				m_corpsePartsNutrition.clear();
				// Standard corpse: torso(25) + head(10) + 4 limbs(15 each) = ~95 total
				m_corpsePartsNutrition << 25 << 15 << 15 << 15 << 15 << 10;
				m_eatingTicks = 0;
				m_desperateAction = DesperateAction::Eating;
				Global::logger().log( LogType::WILDLIFE, S::s( "$CreatureName_" + m_species ) + " begins feeding on a corpse.", m_id, m_position.x, m_position.y, m_position.z );
			}
			else if ( ( m_desperateAction == DesperateAction::HuntingPrey || m_desperateAction == DesperateAction::HuntingGnome ) && m_currentAttackTarget )
			{
				// Attack adjacent target
				Creature* target = g->gm()->gnome( m_currentAttackTarget );
				if ( !target )
					target = g->cm()->creature( m_currentAttackTarget );

				if ( target && !target->isDead() )
				{
					Position tPos = target->getPos();
					int dist = abs( m_position.x - tPos.x ) + abs( m_position.y - tPos.y ) + abs( m_position.z - tPos.z );

					if ( dist <= 1 && m_globalCooldown <= 0 )
					{
						int attackSkill  = m_stateMap.value( "Attack" ).toInt();
						int attackDamage = m_stateMap.value( "Damage" ).toInt();
						m_facing = getFacing( m_position, tPos );
						Global::logger().log( LogType::COMBAT, S::s( "$CreatureName_" + m_species ) + " attacks " + target->name() + "!", m_id, m_position.x, m_position.y, m_position.z );
						target->attack( DT_PIERCING, AH_LOW, attackSkill, attackDamage, m_position, m_id );
						m_globalCooldown = 8;

						// Check if target died — create corpse and start eating
						if ( target->isDead() )
						{
							QString corpseType = ( m_desperateAction == DesperateAction::HuntingGnome ) ? "GnomeCorpse" : "AnimalCorpse";
							QString material = ( m_desperateAction == DesperateAction::HuntingGnome ) ? "Gnome" : target->species();
							m_corpseToEat = g->inv()->createItem( tPos, corpseType, { material } );
							g->inv()->setInJob( m_corpseToEat, m_id );

							// Body-part nutrition based on anatomy
							m_corpsePartsNutrition.clear();
							const auto& parts = target->anatomy().parts();
							for ( auto it = parts.constBegin(); it != parts.constEnd(); ++it )
							{
								if ( !it.value().isInside ) // only external parts
								{
									m_corpsePartsNutrition.append( qMax( 1, it.value().hp / 2 ) );
								}
							}
							// A gnome with missing limbs = less nutrition (parts already gone)
							if ( m_corpsePartsNutrition.isEmpty() )
								m_corpsePartsNutrition << 10; // minimum scraps

							m_currentAttackTarget = 0;
							m_eatingTicks = 0;
							m_desperateAction = DesperateAction::Eating;
							Global::logger().log( LogType::WILDLIFE, S::s( "$CreatureName_" + m_species ) + " killed " + target->name() + " and begins feeding.", m_id, m_position.x, m_position.y, m_position.z );
						}
					}
					else if ( dist > 1 )
					{
						// Re-path toward moving target
						setCurrentTarget( tPos );
						g->pf()->getPath( m_id, m_position, tPos, true, m_currentPath );
					}
				}
				else
				{
					// Target dead or gone — reset and re-evaluate next minute
					m_currentAttackTarget = 0;
					m_desperateAction = DesperateAction::None;
				}
			}
		}

		// ---- Move on current path ----
		if ( !m_currentPath.empty() )
		{
			if ( m_moveCooldown <= 0 )
			{
				Position next = m_currentPath.back();
				m_currentPath.pop_back();
				if ( g->w()->isWalkable( next ) )
				{
					m_facing = getFacing( m_position, next );
					Position oldPos2 = m_position;
					m_position = next;
					move( oldPos2 );
				}
				m_moveCooldown = m_moveDelay * 0.5f; // double speed when desperate
			}
			m_lastOnTick = tickNumber;
			return CreatureTickResult::OK;
		}
	}
	else
	{
		// Not desperate — reset state
		if ( m_desperateAction != DesperateAction::None && m_desperateAction != DesperateAction::Eating )
		{
			m_desperateAction = DesperateAction::None;
			m_currentAttackTarget = 0;
			m_foodTarget = 0;
			m_preyTarget = 0;
		}
	}

	// Normal BT execution (only runs if not in active desperation)
	if ( m_behaviorTree )
	{
		m_behaviorTree->tick();
	}

	move( oldPos );

	m_lastOnTick = tickNumber;

	return CreatureTickResult::OK;
}

void Animal::move( Position oldPos )
{
	if ( m_isMulti )
	{
		if ( m_position != oldPos )
		{
			auto rows = DB::selectRows( "Creature_Layouts", m_species );
			for ( const auto& row : rows )
			{
				Position offset( row.value( "Offset" ) );
				g->w()->setWallSprite( oldPos + offset, 0 );
			}
			for ( const auto& row : rows )
			{
				Position offset( row.value( "Offset" ) );
				g->w()->setWallSprite( m_position + offset, g->sf()->createSprite( row.value( "Sprite" ).toString(), { "none" } )->uID );
			}
		}
	}
	else
	{
		Creature::move( oldPos );
	}
}

bool Animal::isEgg()
{
	return m_stateMap.value( "ID2" ).toString() == "Egg";
}

bool Animal::isYoung()
{
	return m_stateMap.value( "ID2" ).toString() == "Young";
}

bool Animal::isAdult()
{
	return m_stateMap.value( "ID2" ).toString() == "Adult";
}

BT_RESULT Animal::conditionIsEgg( bool halt )
{
	if ( m_isEgg )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsYoung( bool halt )
{
	if ( m_isYoung )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsAdult( bool halt )
{
	if ( m_isAdult )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsHungry( bool halt )
{
	if ( m_hunger < 30 )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsCarnivore( bool halt )
{
	if ( m_diet.contains( "Meat" ) )
		return BT_RESULT::SUCCESS;
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsHerbivore( bool halt )
{
	if ( m_diet.contains( "Vegetable" ) || m_diet.contains( "Hay" ) || m_diet.contains( "Grain" ) || m_diet.contains( "Fruit" ) )
		return BT_RESULT::SUCCESS;
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsPregnant( bool halt )
{
	if ( m_birthTick )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsReadyToGiveBirth( bool halt )
{
	if ( m_birthTick && GameState::tick > m_birthTick )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsWoodVermin( bool halt )
{
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsEggLayer( bool halt )
{
	if ( m_isEggLayer )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionLayEgg( bool halt )
{
	if ( m_pastureID == 0 )
	{
		return BT_RESULT::FAILURE;
	}

	if ( !m_birthTick )
	{
		QVariantMap def = m_stateMap.value( "EggLayer" ).toMap();
		//first visit;
		int days        = def.value( "DaysBetween" ).toInt();
		quint64 nextLay = GameState::tick + ( Global::util->ticksPerDayRandomized( 5 ) * days );
		m_birthTick     = nextLay;
		return BT_RESULT::FAILURE;
	}

	if ( m_birthTick && GameState::tick >= m_birthTick && m_hunger > 50 )
	{
		QVariantMap def  = m_stateMap.value( "EggLayer" ).toMap();
		bool collectEggs = false;
		if ( m_pastureID != 0 )
		{
			Pasture* pasture = g->fm()->getPasture( m_pastureID );
			if ( pasture )
			{
				collectEggs = pasture->harvest();
			}
		}
		int totalCount = g->cm()->count( m_species );
		if ( ( totalCount < GameState::maxAnimalsPerType && totalCount < 1000 ) || collectEggs )
		{
			for ( int i = 0; i < def.value( "Amount" ).toInt(); ++i )
			{
				if ( collectEggs )
				{
					g->inv()->createItem( m_position, "Egg", { m_species } );
				}
				else
				{
					g->cm()->addCreature( CreatureType::ANIMAL, def.value( "EggID" ).toString(), m_position, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE, false, m_tame );
					if ( g->cm()->count( m_species ) >= GameState::maxAnimalsPerType )
					{
						break;
					}
				}
			}
			m_birthTick = 0;
			return BT_RESULT::SUCCESS;
		}
		else
		{
			m_birthTick = 0;
		}
	}

	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsProducer( bool halt )
{
	if ( m_isProducer )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionProduce( bool halt )
{
	if ( !m_produceTick )
	{
		//first visit;
		QVariantMap def  = m_stateMap.value( "Producer" ).toMap();
		int days         = def.value( "DaysBetween" ).toInt();
		quint64 nextProd = GameState::tick + ( Global::util->ticksPerDayRandomized( 5 ) * days );
		m_produceTick    = nextProd;
		return BT_RESULT::FAILURE;
	}

	if ( m_produceTick && GameState::tick >= m_produceTick && m_hunger > 50 )
	{
		// produce item
		QVariantMap def = m_stateMap.value( "Producer" ).toMap();
		if ( def.value( "Auto" ).toBool() )
		{
			QString produce = def.value( "ItemID" ).toString();
			for ( int i = 0; i < def.value( "Amount" ).toInt(); ++i )
			{
				g->inv()->createItem( m_position, def.value( "ItemID" ).toString(), { m_species } );
			}
		}
		else
		{
			int amount = def.value( "Amount" ).toInt();
			if ( m_producedAmount < amount )
			{
				m_produce = def.value( "ItemID" ).toString();
				++m_producedAmount;
			}
		}
		int days         = def.value( "DaysBetween" ).toInt();
		quint64 nextProd = GameState::tick + ( Global::util->ticksPerDayRandomized( 5 ) * days );
		m_produceTick    = nextProd;
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionTryHaveSex( bool halt )
{
	if ( m_gender == Gender::MALE && m_isAdult && m_hunger > 50 )
	{
		int totalCount = g->cm()->count( m_species );
		if ( totalCount < GameState::maxAnimalsPerType && totalCount < 1000 )
		{
			//isOnPasture
			if ( m_pastureID )
			{
				//last sex longer than 24 hours ago
				if ( m_lastSex + Global::util->ticksPerDay < GameState::tick )
				{
					// non pregnant female on pasture
					Pasture* pasture = g->fm()->getPasture( m_pastureID );
					if ( pasture )
					{
						auto list = pasture->animals();
						for ( auto id : list )
						{
							auto a = g->cm()->animal( id );
							if ( a->gender() == Gender::FEMALE && !a->isPregnant() && a->isAdult() )
							{
								a->setPregnant( true );
								m_lastSex = GameState::tick;
								return BT_RESULT::SUCCESS;
							}
						}
					}
				}
			}
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionGiveBirth( bool halt )
{
	// birth tick reached
	if ( m_birthTick && GameState::tick > m_birthTick )
	{
		int totalCount = g->cm()->count( m_species );
		if ( ( totalCount < GameState::maxAnimalsPerType && totalCount < 1000 ) )
		{
			// create baby
			setPregnant( false );
			unsigned int babyID = g->cm()->addCreature( CreatureType::ANIMAL, m_species, m_position, rand() % 2 == 0 ? Gender::MALE : Gender::FEMALE, false, m_tame );
			// if mother on pasture and space on pasture
			Pasture* pasture = g->fm()->getPasture( m_pastureID );
			if ( pasture )
			{
				pasture->addAnimal( babyID );
			}
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

void Animal::setPregnant( bool pregnant )
{
	if ( pregnant )
	{
		int days      = DB::select( "GestationDays", "Animals", m_species ).toInt();
		quint64 ticks = Global::util->ticksPerDayRandomized( 5 ) * days;
		m_birthTick   = GameState::tick + ticks;
	}
	else
	{
		m_birthTick = 0;
	}
}

bool Animal::isPregnant()
{
	return m_birthTick != 0;
}

void Animal::setTame( bool tame )
{
	m_tame = tame;
}

bool Animal::isTame()
{
	return m_tame;
}

bool Animal::isAggro()
{
	return m_stateMap.value( "IsAggro" ).toBool() || m_starvingAggro;
}

void Animal::setPastureID( unsigned int id )
{
	m_pastureID = id;
}

unsigned int Animal::pastureID()
{
	return m_pastureID;
}

int Animal::numProduce()
{
	return m_producedAmount;
}

QString Animal::producedItem()
{
	return m_produce;
}

void Animal::harvest()
{
	m_producedAmount = 0;
	m_produce        = "";
	if ( !m_dye.isEmpty() )
	{
		m_dye = "";
		updateSprite();
	}
}

bool Animal::morph( QVariantMap def )
{
	g->cm()->addCreature( CreatureType::ANIMAL, def.value( "CreatureID" ).toString(), m_position, m_gender, false, m_tame );
	m_toDestroy = true;
	return true;
}

void Animal::setInJob( unsigned int id )
{
	m_inJob = id;
}

unsigned int Animal::inJob()
{
	return m_inJob;
}

float Animal::hunger()
{
	return m_hunger;
}

BT_RESULT Animal::actionGraze( bool halt )
{
	if ( m_hunger < 80 )
	{
		Tile& tile = g->w()->getTile( m_position );
		if ( m_isGrazer )
		{
			if ( tile.flags & TileFlag::TF_GRASS && tile.vegetationLevel > 10 )
			{
				if ( tile.flags & TileFlag::TF_PASTURE )
				{
					tile.vegetationLevel = qMax( 10, tile.vegetationLevel - 1 );
				}

				m_hunger += m_foodValue;

				g->w()->addToUpdateList( m_position );
				return BT_RESULT::SUCCESS;
			}
		}
		if ( m_pastureID && (bool)( tile.flags & TileFlag::TF_PASTURE ) )
		{
			auto pasture =  g->fm()->getPastureAtPos( m_position );
			if ( pasture && pasture->id() == m_pastureID )
			{
				if ( pasture->eatFromTrough() )
				{
					m_hunger += m_foodValue * 5;
					return BT_RESULT::SUCCESS;
				}
			}
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionFindPrey( bool halt )
{
	for ( const auto& sPrey : m_preyList )
	{
		Animal* prey = g->cm()->getClosestAnimal( m_position, sPrey );
		if ( prey )
		{
			QList<Position> neighbs = Global::util->neighbors8( prey->getPos() );
			auto distances          = PriorityQueue<Position, int>();
			for ( const auto& neigh : neighbs )
			{
				if ( g->w()->isWalkable( neigh ) && g->pf()->checkConnectedRegions( neigh, m_position ) )
				{
					distances.put( neigh, m_position.distSquare( neigh ) );
				}
			}
			if ( distances.empty() )
			{
				return BT_RESULT::FAILURE;
			}
			setCurrentTarget( distances.get() );
			m_currentPrey = prey->id();
			return BT_RESULT::SUCCESS;
		}
	}
	m_currentPrey = 0;
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionKillPrey( bool halt )
{
	//TODO check if adjacent

	Animal* prey = g->cm()->animal( m_currentPrey );
	if ( prey )
	{
		if ( prey->kill( true ) )
		{
			m_corpseToEat = g->inv()->createItem( prey->getPos(), "AnimalCorpse", { prey->species() } );
			m_currentPrey = 0;
			g->inv()->setInJob( m_corpseToEat, m_id );

			return BT_RESULT::SUCCESS;
		}
	}
	m_currentPrey = 0;
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionEatPrey( bool halt )
{
	if ( halt )
	{
		m_corpseToEat = 0;
		return BT_RESULT::IDLE;
	}
	if ( m_corpseToEat )
	{
		m_hunger += 0.5;
		if ( m_hunger >= 100 )
		{
			g->inv()->createItem( Position( g->inv()->getItemPos( m_corpseToEat ) ), "Bone", g->inv()->materialSID( m_corpseToEat ) + "Bone" );

			g->inv()->destroyObject( m_corpseToEat );
			m_corpseToEat = 0;
			return BT_RESULT::SUCCESS;
		}
		return BT_RESULT::RUNNING;
	}

	m_corpseToEat = 0;
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionMove( bool halt )
{
	if ( halt )
	{
		m_currentPrey = 0;
		m_currentPath.clear();
		return BT_RESULT::IDLE;
	}
	if ( m_immobile )
	{
		return BT_RESULT::SUCCESS;
	}
	// gnome has a path, move on path and return
	if ( !m_currentPath.empty() )
	{
		if ( m_currentAttackTarget )
		{
			if ( conditionTargetAdjacent( false ) == BT_RESULT::SUCCESS )
			{
				m_currentPath.clear();
				return BT_RESULT::SUCCESS;
			}

			if ( conditionTargetPositionValid( false ) == BT_RESULT::FAILURE )
			{
				m_currentPath.clear();
				return BT_RESULT::FAILURE;
			}
		}

		if ( !moveOnPath() )
		{
			m_currentPath.clear();
			return BT_RESULT::FAILURE;
		}
		return BT_RESULT::RUNNING;
	}

	Position targetPos = currentTarget();

	// check if we are already on the tile
	if ( m_position == targetPos )
	{
		return BT_RESULT::SUCCESS;
	}

	PathFinderResult pfr = g->pf()->getPath( m_id, m_position, targetPos, m_ignoreNoPass, m_currentPath );
	switch ( pfr )
	{
		case PathFinderResult::NoConnection:
			return BT_RESULT::FAILURE;
		case PathFinderResult::Running:
		case PathFinderResult::FoundPath:
			return BT_RESULT::RUNNING;
	}
	return BT_RESULT::RUNNING;
}

BT_RESULT Animal::actionFindRetreat( bool halt )
{
	if ( halt )
	{
		return BT_RESULT::IDLE;
	}
	int randPos = qMax( 2, rand() % ( Global::dimX - 2 ) );

	Position pos( 0, 0, Global::dimZ - 1 );
	int border = rand() % 4;
	switch ( border )
	{
		case 0: //north
			pos.x = qMax( 2, rand() % ( Global::dimX - 2 ) );
			pos.y = qMax( 2, rand() % 10 );
			break;
		case 1: //east
			pos.x = Global::dimX - qMax( 2, rand() % 10 );
			pos.y = qMax( 2, rand() % ( Global::dimX - 2 ) );
			break;
		case 2: //south
			pos.x = qMax( 2, rand() % ( Global::dimX - 2 ) );
			pos.y = Global::dimX - qMax( 2, rand() % 10 );
			break;
		case 3: // west
			pos.x = qMax( 2, rand() % 10 );
			pos.y = qMax( 2, rand() % ( Global::dimX - 2 ) );
			break;
		default:
			break;
	}
	g->w()->getFloorLevelBelow( pos, false );

	if ( g->w()->fluidLevel( pos ) == 0 )
	{
		if ( g->pf()->checkConnectedRegions( pos, m_position ) )
		{
			setCurrentTarget( pos );
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionSleep( bool halt )
{
	if ( halt )
	{
		return BT_RESULT::IDLE;
	}
	setThoughtBubble( "Sleeping" );
	return BT_RESULT::RUNNING;
}

BT_RESULT Animal::conditionIsInShed( bool halt )
{
	if ( m_inShed )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::conditionIsOnPasture( bool halt )
{
	if ( m_pastureID )
	{
		return BT_RESULT::SUCCESS;
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionFindShed( bool halt )
{
	if ( m_pastureID )
	{
		Pasture* pasture = g->fm()->getPasture( m_pastureID );
		if ( pasture )
		{
			Position pos = pasture->findShed();
			if ( !pos.isZero() )
			{
				setCurrentTarget( pos );
				return BT_RESULT::SUCCESS;
			}
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionFindRandomPastureField( bool halt )
{
	if ( m_pastureID )
	{
		Pasture* pasture = g->fm()->getPasture( m_pastureID );
		if ( pasture )
		{
			int random = rand() % pasture->countTiles();
			setCurrentTarget( pasture->randomFieldPos() );
			return BT_RESULT::SUCCESS;
		}
	}
	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionEnterShed( bool halt )
{
	m_inShed = true;
	return BT_RESULT::SUCCESS;
}

BT_RESULT Animal::actionLeaveShed( bool halt )
{
	m_inShed = false;
	return BT_RESULT::SUCCESS;
}

BT_RESULT Animal::actionGetTarget( bool halt )
{
	if ( m_aggroList.size() )
	{
		unsigned int targetID = m_aggroList.first().id;
		Creature* creature    = g->cm()->creature( targetID );
		if ( !creature )
		{
			creature = g->gm()->gnome( targetID );
		}
		if ( creature && !creature->isDead() )
		{
			m_currentAttackTarget = targetID;
			setCurrentTarget( creature->getPos() );
			return BT_RESULT::SUCCESS;
		}
		else
		{
			m_aggroList.removeFirst();
			return BT_RESULT::FAILURE;
		}
	}
	else
	{
		//TODO even though aggro list is empty, check for other threats around
	}

	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionGuardDogGetTarget( bool halt )
{
	auto foxes = g->cm()->animalsByDistance( m_position, "Fox" );
	if ( !foxes.empty() )
	{
		auto fox = foxes.get();
		if ( m_position.distSquare( fox->getPos() ) < 40 )
		{
			//qDebug() << "fox alert";
			AggroEntry ae { 100, fox->id() };
			m_aggroList.append( ae );
		}
	}
	return actionGetTarget( halt );
}

BT_RESULT Animal::actionAttackTarget( bool halt )
{
	Creature* creature = g->gm()->gnome( m_currentAttackTarget );
	if ( !creature )
	{
		creature = g->cm()->creature( m_currentAttackTarget );
	}

	if ( creature && !creature->isDead() )
	{
		m_facing = getFacing( m_position, creature->getPos() );

		if ( m_globalCooldown <= 0 )
		{
			if ( m_biteCooldown <= 0 )
			{
				Global::logger().log( LogType::COMBAT, m_name + " attacks " + creature->name(), m_id );
				// attack with main hand

				int attackSkill  = m_stateMap.value( "Attack" ).toInt();
				int attackDamage = m_stateMap.value( "Damage" ).toInt();

				creature->attack( DT_PIERCING, AH_LOW, attackSkill, attackDamage, m_position, m_id );
				m_biteCooldown   = 10;
				m_globalCooldown = 5;
			}
		}

		return BT_RESULT::RUNNING;
	}
	m_currentAttackTarget = 0;

	return BT_RESULT::FAILURE;
}

BT_RESULT Animal::actionRandomMoveBig( bool halt )
{
	// get new possible location

	// remove old sprites

	// set new sprites

	return BT_RESULT::FAILURE;
}

bool Animal::attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID )
{
	// from which side is the attack coming
	AnatomySide ds = AS_CENTER;
	switch ( m_facing )
	{
		case 0:
			if ( m_position.x < sourcePos.x )
				ds = AS_FRONT;
			else if ( m_position.x > sourcePos.x )
				ds = AS_BACK;
			if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_RIGHT );
			else if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_LEFT );
			break;
		case 1:
			if ( m_position.x < sourcePos.x )
				ds = AS_LEFT;
			else if ( m_position.x > sourcePos.x )
				ds = AS_RIGHT;
			if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_FRONT );
			else if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_BACK );
			break;
		case 2:
			if ( m_position.x > sourcePos.x )
				ds = AS_FRONT;
			else if ( m_position.x < sourcePos.x )
				ds = AS_BACK;
			if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_RIGHT );
			else if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_LEFT );
			break;
		case 3:
			if ( m_position.x > sourcePos.x )
				ds = AS_LEFT;
			else if ( m_position.x < sourcePos.x )
				ds = AS_RIGHT;
			if ( m_position.y > sourcePos.y )
				ds = AnatomySide( ds | AS_FRONT );
			else if ( m_position.y < sourcePos.y )
				ds = AnatomySide( ds | AS_BACK );
			break;
	}
	// attacker skill vs our dodge or block chance

	int dodge = getSkillLevel( "Dodge" );
	if ( ds & AS_BACK )
	{
		skill *= 1.5;
	}

	bool hit = skill >= dodge;
	if ( dodge > skill )
	{
		srand( std::chrono::system_clock::now().time_since_epoch().count() );
		int diff = dodge - skill;
		diff     = qMax( 5, 20 - diff );
		hit |= rand() % 100 > diff;
	}

	if ( hit )
	{
		Global::logger().log( LogType::COMBAT, m_name + " took " + QString::number( strength ) + " damage.", m_id );
		m_anatomy.damage( &m_equipment, dt, da, ds, strength );
	}
	else
	{
		Global::logger().log( LogType::COMBAT, m_name + " dogded the attack. Skill:" + QString::number( skill ) + " Dodge: " + QString::number( dodge ), m_id );
	}

	bool aeExists = false;
	for ( auto& ae : m_aggroList )
	{
		if ( ae.id == attackerID )
		{
			ae.aggro += strength;
			aeExists = true;
			break;
		}
	}
	if ( !aeExists )
	{
		AggroEntry newAE { strength, attackerID };
		m_aggroList.append( newAE );
	}

	return true;
}

bool Animal::toButcher()
{
	return m_toButcher;
}

void Animal::setToButcher( bool value )
{
	m_toButcher = value;
}