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
#include "gnome.h"
#include "../game/gnomemanager.h"
#include "../game/game.h"
#include "../game/spatialgrid.h"

#include "../base/behaviortree/bt_tree.h"
#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/util.h"
#include "../game/creaturemanager.h"
#include "../game/eventmanager.h"
#include "../game/inventory.h"
#include "../game/militarymanager.h"
#include "../game/plant.h"
#include "../game/roommanager.h"
#include "../game/stockpilemanager.h"
#include "../game/world.h"
#include "../game/workshop.h"
#include "../game/workshopmanager.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QDebug>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QPainter>
#include <QThreadPool>

Gnome::Gnome( Position& pos, QString name, Gender gender, Game* game ) :
	CanWork( pos, name, gender, "Gnome", game )
{
	m_ignoreNoPass = false;

	int shirt         = rand() % 14 + 1;
	m_equipment.shirt = "GnomeShirt" + QString::number( shirt );
	int hair          = rand() % 14 + 1;
	m_equipment.hair  = "GnomeHair" + QString::number( hair );

	auto numHairColors = DB::ids( "HairColors" ).size();

	m_equipment.hairColor  = rand() % numHairColors;
	m_equipment.shirtColor = rand() % 6;

	int fhair = rand() % 15;
	if ( m_gender == Gender::MALE )
	{
		m_equipment.facialHair = "GnomeFacialHair" + QString::number( fhair );
	}

	m_type = CreatureType::GNOME;

	m_anatomy.init( "Humanoid", false );

	for ( int i = 0; i < 24; ++i )
	{
		m_schedule.append( ScheduleActivity::None );
	}

	log( GameState::currentYearAndSeason );
}

Gnome::Gnome( QVariantMap& in, Game* game ) :
	CanWork( in, game )
{
	m_skillActive     = in.value( "SkillActive" ).toMap();
	m_skillPriorities = in.value( "SkillPriorities" ).toStringList();

	m_needs = in.value( "Needs" ).toMap();

	// Load mood system (backward-compatible)
	if ( in.contains( "Mood" ) )
	{
		m_mood = in.value( "Mood" ).toInt();
	}
	if ( in.contains( "MentalBreak" ) )
	{
		m_mentalBreak = in.value( "MentalBreak" ).toBool();
	}
	if ( in.contains( "Thoughts" ) )
	{
		for ( const auto& tv : in.value( "Thoughts" ).toList() )
		{
			auto tm = tv.toMap();
			Thought t;
			t.id = tm.value( "ID" ).toString();
			t.text = tm.value( "Text" ).toString();
			t.cause = tm.value( "Cause" ).toString();
			t.moodValue = tm.value( "MoodValue" ).toInt();
			t.ticksLeft = tm.value( "TicksLeft" ).toInt();
			t.initialDuration = tm.value( "InitialDuration" ).toInt();
			t.maxStacks = tm.value( "MaxStacks" ).toInt();
			m_thoughts.append( t );
		}
	}

	if ( in.contains( "FoodExclusions" ) )
	{
		m_foodExclusions = in.value( "FoodExclusions" ).toStringList();
	}
	if ( in.contains( "RecentMeals" ) )
	{
		m_recentMeals = in.value( "RecentMeals" ).toStringList();
	}

	if ( in.contains( "Profession" ) )
	{
		m_profession = in.value( "Profession" ).toString();
	}
	else
	{
		m_profession = "Gnomad";
	}

	m_equipment = in.value( "Equipment" ).toMap();

	if ( in.contains( "Schedule" ) )
	{
		auto list = in.value( "Schedule" ).toList();
		m_schedule.clear();
		for( auto vs : list )
		{
			auto ss = vs.toString();
			ScheduleActivity sa = ScheduleActivity::None;
			if ( ss == "eat" ) sa = ScheduleActivity::Eat;
			else if ( ss == "sleep" ) sa = ScheduleActivity::Sleep;
			else if ( ss == "train" ) sa = ScheduleActivity::Training;

			m_schedule.append( sa );
		}
	}
	else
	{
		for ( int i = 0; i < 24; ++i )
		{
			m_schedule.append( ScheduleActivity::None );
		}
	}

	m_carryBandages = in.value( "CarryBandages" ).toBool();
	m_carryFood     = in.value( "CarryFood" ).toBool();
	m_carryDrinks   = in.value( "CarryDrinks" ).toBool();

	// TODO update carried counts from inventory
	for ( auto item : m_inventoryItems )
	{
		if ( g->inv()->itemSID( item ) == "Bandage" )
		{
			++m_carriedBandages;
		}
		else if ( g->inv()->nutritionalValue( item ) > 0 )
		{
			++m_carriedFood;
		}
		else if ( g->inv()->drinkValue( item ) > 0 )
		{
			++m_carriedDrinks;
		}
	}

	m_leftHandAttackSkill  = getSkillLevel( "Unarmed" );
	m_leftHandAttackValue  = m_leftHandAttackSkill;
	m_rightHandAttackSkill = getSkillLevel( "Unarmed" );
	m_rightHandAttackValue = m_rightHandAttackSkill;

	log( GameState::currentYearAndSeason );
}

void Gnome::serialize( QVariantMap& out )
{
	CanWork::serialize( out );
	////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////

	out.insert( "SkillActive", m_skillActive );
	out.insert( "SkillPriorities", m_skillPriorities );
	//basic needs
	out.insert( "Needs", m_needs );
	out.insert( "Equipment", m_equipment.serialize() );

	out.insert( "Profession", m_profession );

	// Mood system serialization
	out.insert( "Mood", m_mood );
	out.insert( "MentalBreak", m_mentalBreak );
	QVariantList vThoughts;
	for ( const auto& t : m_thoughts )
	{
		QVariantMap tm;
		tm.insert( "ID", t.id );
		tm.insert( "Text", t.text );
		tm.insert( "Cause", t.cause );
		tm.insert( "MoodValue", t.moodValue );
		tm.insert( "TicksLeft", t.ticksLeft );
		tm.insert( "InitialDuration", t.initialDuration );
		tm.insert( "MaxStacks", t.maxStacks );
		vThoughts.append( tm );
	}
	if ( !vThoughts.isEmpty() )
	{
		out.insert( "Thoughts", vThoughts );
	}
	if ( !m_foodExclusions.isEmpty() )
	{
		out.insert( "FoodExclusions", QVariant( m_foodExclusions ) );
	}
	if ( !m_recentMeals.isEmpty() )
	{
		out.insert( "RecentMeals", QVariant( m_recentMeals ) );
	}

	

	QVariantList vSchedule;
	for( const auto& sa : m_schedule )
	{
		switch ( sa )
		{
			case ScheduleActivity::Eat:
				vSchedule.append( "eat" );
				break;
			case ScheduleActivity::Sleep:
				vSchedule.append( "sleep" );
				break;
			case ScheduleActivity::Training:
				vSchedule.append( "train" );
				break;
			default:
				vSchedule.append( "none" );
		}
	}
	out.insert( "Schedule", vSchedule );

	out.insert( "CarryBandages", m_carryBandages );
	out.insert( "CarryFood", m_carryFood );
	out.insert( "CarryDrinks", m_carryDrinks );
}

Gnome::~Gnome()
{
}

void Gnome::init()
{
	m_bucket = static_cast<int>( m_id % GnomeManager::N_BUCKETS );

	g->w()->insertCreatureAtPosition( m_position, m_id );
	if ( g->sg() )
	{
		g->sg()->insertEntity( m_id, m_position );
	}

	updateSprite();
	initTaskMap();
	loadBehaviorTree( "Gnome" );

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

void Gnome::setConcealed( QString part, bool concealed )
{
	for ( int k = 0; k < m_spriteDef.size(); ++k )
	{
		auto cm = m_spriteDef[k].toMap();
		if ( cm.value( "Part" ).toString() == part )
		{
			cm.insert( "Hidden", concealed );
			m_spriteDef.replace( k, cm );
			break;
		}
	}

	for ( int k = 0; k < m_spriteDefBack.size(); ++k )
	{
		auto cm = m_spriteDefBack[k].toMap();
		if ( cm.value( "Part" ).toString() == part )
		{
			cm.insert( "Hidden", concealed );
			m_spriteDefBack.replace( k, cm );
			break;
		}
	}
}

void Gnome::updateSprite()
{
	m_spriteDef     = createSpriteDef( "Gnome", false );
	m_spriteDefBack = createSpriteDef( "GnomeBack", true );

	g->sf()->setCreatureSprite( m_id, m_spriteDef, m_spriteDefBack, isDead() );
	m_equipmentChanged = true;
	m_renderParamsChanged = true;
}

QVariantList Gnome::createSpriteDef( QString type, bool isBack )
{
	auto parts = DB::selectRows( "Creature_Parts", type );

	QVariantMap ordered;
	for ( auto pm : parts )
	{
		ordered.insert( pm.value( "Order" ).toString(), pm );
	}

	Uniform* uniform = nullptr;
	if ( m_roleID )
	{
		uniform = g->mil()->uniform( m_roleID );
	}

	QVariantList def;
	for ( auto vpm : ordered )
	{
		auto pm = vpm.toMap();

		CreaturePart part = Global::creaturePartLookUp.value( pm.value( "Part" ).toString() );

		QString tint = pm.value( "Tint" ).toString();

		QString bs = pm.value( "BaseSprite" ).toString();

		bool hairConcealed = false;
		if ( m_equipment.head.itemID )
		{
			hairConcealed = true;
		}

		unsigned int item = 0;

		switch ( part )
		{
			case CP_HEAD:
				break;
			case CP_TORSO:
				break;
			case CP_LEFT_ARM:
				break;
			case CP_RIGHT_ARM:
				break;
			case CP_LEFT_HAND:
				break;
			case CP_RIGHT_HAND:
				break;
			case CP_LEFT_LEG:
				break;
			case CP_RIGHT_LEG:
				break;
			case CP_LEFT_FOOT:
				break;
			case CP_RIGHT_FOOT:
				break;

			case CP_HAIR:
				pm.insert( "Tint", m_equipment.hairColor );
				bs = m_equipment.hair;
				pm.insert( "IsHair", true );
				pm.insert( "Hidden", hairConcealed );
				break;
			case CP_FACIAL_HAIR:
				pm.insert( "Tint", m_equipment.hairColor );
				bs = m_equipment.facialHair;
				break;
			case CP_CLOTHING:
				pm.insert( "Tint", m_equipment.shirtColor );
				bs = m_equipment.shirt;
				break;
			case CP_BOOTS:
				break;
			case CP_HAT:
				break;

			case CP_ARMOR_HEAD:
				if ( m_equipment.head.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.head.itemID );
					bs += "HeadArmor";
					hairConcealed = true;
					pm.insert( "Material", m_equipment.head.material );
				}
				break;
			case CP_ARMOR_TORSO:
				if ( m_equipment.chest.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.chest.itemID );
					bs += "ChestArmor";
					pm.insert( "Material", m_equipment.chest.material );
				}
				break;
			case CP_ARMOR_LEFT_ARM:
				if ( m_equipment.arm.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.arm.itemID );
					bs += "LeftArmArmor";
					pm.insert( "Material", m_equipment.arm.material );
				}
				break;
			case CP_ARMOR_RIGHT_ARM:
				if ( m_equipment.arm.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.arm.itemID );
					bs += "RightArmArmor";
					pm.insert( "Material", m_equipment.arm.material );
				}
				break;
			case CP_ARMOR_LEFT_HAND:
				if ( m_equipment.hand.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.hand.itemID );
					bs += "LeftHandArmor";
					pm.insert( "Material", m_equipment.hand.material );
				}
				break;
			case CP_ARMOR_RIGHT_HAND:
				if ( m_equipment.hand.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.hand.itemID );
					bs += "RightHandArmor";
					pm.insert( "Material", m_equipment.hand.material );
				}
				break;
			case CP_ARMOR_LEFT_FOOT:
				if ( m_equipment.foot.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.foot.itemID );
					bs += "LeftFootArmor";
					pm.insert( "Material", m_equipment.foot.material );
				}
				break;
			case CP_ARMOR_RIGHT_FOOT:
				if ( m_equipment.foot.itemID )
				{
					bs = "Gnome";
					bs += g->inv()->itemGroup( m_equipment.foot.itemID );
					bs += "RightFootArmor";
					pm.insert( "Material", m_equipment.foot.material );
				}
				break;
			case CP_LEFT_HAND_HELD:
				if ( m_equipment.leftHandHeld.itemID )
				{
					bs = "Gnome";
					bs += m_equipment.leftHandHeld.item;
					bs += "Left";
					pm.insert( "Material", m_equipment.leftHandHeld.material );
					pm.insert( "HasBase", true );
				}
				break;
			case CP_RIGHT_HAND_HELD:
				if ( m_equipment.rightHandHeld.itemID )
				{
					bs = "Gnome";
					bs += m_equipment.rightHandHeld.item;
					bs += "Right";
					pm.insert( "Material", m_equipment.leftHandHeld.material );
					pm.insert( "HasBase", true );
				}
				break;
			case CP_BACK:
				if ( isBack && m_equipment.back.itemID )
				{
					bs = "Gnome";
					bs += m_equipment.back.item;
					pm.insert( "Material", m_equipment.leftHandHeld.material );
				}
				break;
		}
		

		if ( isBack && !bs.endsWith( "Back" ) )
		{
			bs += "Back";
		}

		pm.insert( "BaseSprite", bs );

		def.append( pm );
	}
	return def;
}

void Gnome::initTaskMap()
{
	using namespace std::placeholders;

	m_behaviors.insert( "IsSleepy", std::bind( &Gnome::conditionIsSleepy, this, _1 ) );
	m_behaviors.insert( "IsHungry", std::bind( &Gnome::conditionIsHungry, this, _1 ) );
	m_behaviors.insert( "IsVeryHungry", std::bind( &Gnome::conditionIsVeryHungry, this, _1 ) );
	m_behaviors.insert( "IsThirsty", std::bind( &Gnome::conditionIsThirsty, this, _1 ) );
	m_behaviors.insert( "IsVeryThirsty", std::bind( &Gnome::conditionIsVeryThirsty, this, _1 ) );
	m_behaviors.insert( "AllItemsInPlaceForJob", std::bind( &Gnome::conditionAllItemsInPlaceForJob, this, _1 ) );
	m_behaviors.insert( "IsButcherJobJob", std::bind( &Gnome::conditionIsButcherJob, this, _1 ) );
	m_behaviors.insert( "AllPickedUp", std::bind( &Gnome::conditionAllPickedUp, this, _1 ) );
	m_behaviors.insert( "IsFull", std::bind( &Gnome::conditionIsFull, this, _1 ) );
	m_behaviors.insert( "IsDrinkFull", std::bind( &Gnome::conditionIsDrinkFull, this, _1 ) );
	m_behaviors.insert( "IsCivilian", std::bind( &Gnome::conditionIsCivilian, this, _1 ) );
	m_behaviors.insert( "Alarm", std::bind( &Gnome::conditionAlarm, this, _1 ) );
	m_behaviors.insert( "IsInSafeRoom", std::bind( &Gnome::conditionIsInSafeRoom, this, _1 ) );

	// combat conditions
	m_behaviors.insert( "TargetAdjacent", std::bind( &Gnome::conditionTargetAdjacent, this, _1 ) );
	m_behaviors.insert( "IsInCombat", std::bind( &Gnome::conditionIsInCombat, this, _1 ) );
	m_behaviors.insert( "IsTrainingTime", std::bind( &Gnome::conditionIsTrainingTime, this, _1 ) );
	m_behaviors.insert( "IsTrainer", std::bind( &Gnome::conditionIsTrainer, this, _1 ) );

	m_behaviors.insert( "HasHuntTarget", std::bind( &Gnome::conditionHasHuntTarget, this, _1 ) );

	m_behaviors.insert( "IsOnMission", std::bind( &Gnome::conditionIsOnMission, this, _1 ) );

	//m_behaviors.insert( "", std::bind( &Gnome::condition, this, _1 ) );

	m_behaviors.insert( "Sleep", std::bind( &Gnome::actionSleep, this, _1 ) );
	m_behaviors.insert( "FindBed", std::bind( &Gnome::actionFindBed, this, _1 ) );

	m_behaviors.insert( "FindFood", std::bind( &Gnome::actionFindFood, this, _1 ) );
	m_behaviors.insert( "FindDining", std::bind( &Gnome::actionFindDining, this, _1 ) );
	m_behaviors.insert( "Eat", std::bind( &Gnome::actionEat, this, _1 ) );
	m_behaviors.insert( "FindDrink", std::bind( &Gnome::actionFindDrink, this, _1 ) );
	m_behaviors.insert( "Drink", std::bind( &Gnome::actionDrink, this, _1 ) );

	m_behaviors.insert( "Move", std::bind( &Gnome::actionMove, this, _1 ) );
	m_behaviors.insert( "RandomMove", std::bind( &Gnome::actionRandomMove, this, _1 ) );

	m_behaviors.insert( "PickUpItem", std::bind( &Gnome::actionPickUpItem, this, _1 ) );

	m_behaviors.insert( "ClaimItems", std::bind( &Gnome::actionClaimItems, this, _1 ) );
	m_behaviors.insert( "DropItem", std::bind( &Gnome::actionDropItem, this, _1 ) );
	m_behaviors.insert( "EquipTool", std::bind( &Gnome::actionEquipTool, this, _1 ) );
	m_behaviors.insert( "FindTool", std::bind( &Gnome::actionFindTool, this, _1 ) );

	m_behaviors.insert( "CheckUniform", std::bind( &Gnome::actionCheckUniform, this, _1 ) );
	m_behaviors.insert( "CheckBandages", std::bind( &Gnome::actionCheckBandages, this, _1 ) );
	m_behaviors.insert( "UniformCleanUp", std::bind( &Gnome::actionUniformCleanUp, this, _1 ) );

	m_behaviors.insert( "GetItemDropPosition", std::bind( &Gnome::actionGetItemDropPosition, this, _1 ) );
	m_behaviors.insert( "GetWorkPosition", std::bind( &Gnome::actionGetWorkPosition, this, _1 ) );
	m_behaviors.insert( "GetSafeRoomPosition", std::bind( &Gnome::actionGetSafeRoomPosition, this, _1 ) );

	m_behaviors.insert( "GetJob", std::bind( &Gnome::actionGetJob, this, _1 ) );
	m_behaviors.insert( "InitJob", std::bind( &Gnome::actionInitJob, this, _1 ) );
	m_behaviors.insert( "FinishJob", std::bind( &Gnome::actionFinishJob, this, _1 ) );
	m_behaviors.insert( "AbortJob", std::bind( &Gnome::actionAbortJob, this, _1 ) );
	m_behaviors.insert( "Work", std::bind( &Gnome::actionWork, this, _1 ) );

	m_behaviors.insert( "InitAnimalJob", std::bind( &Gnome::actionInitAnimalJob, this, _1 ) );
	m_behaviors.insert( "GrabAnimal", std::bind( &Gnome::actionGrabAnimal, this, _1 ) );
	m_behaviors.insert( "ReleaseAnimal", std::bind( &Gnome::actionReleaseAnimal, this, _1 ) );
	m_behaviors.insert( "ButcherAnimal", std::bind( &Gnome::actionButcherAnimal, this, _1 ) );
	m_behaviors.insert( "DyeAnimal", std::bind( &Gnome::actionDyeAnimal, this, _1 ) );
	m_behaviors.insert( "HarvestAnimal", std::bind( &Gnome::actionHarvestAnimal, this, _1 ) );
	m_behaviors.insert( "TameAnimal", std::bind( &Gnome::actionTameAnimal, this, _1 ) );

	m_behaviors.insert( "FinalMoveAnimal", std::bind( &Gnome::actionFinalMoveAnimal, this, _1 ) );

	m_behaviors.insert( "DropAllItems", std::bind( &Gnome::actionDropAllItems, this, _1 ) );
	m_behaviors.insert( "ReturnAlwaysRunning", std::bind( &Gnome::actionAlwaysRunning, this, _1 ) );

	m_behaviors.insert( "GetTarget", std::bind( &Gnome::actionGetTarget, this, _1 ) );
	m_behaviors.insert( "AttackTarget", std::bind( &Gnome::actionAttackTarget, this, _1 ) );
	m_behaviors.insert( "FindTrainingGround", std::bind( &Gnome::actionFindTrainingGround, this, _1 ) );
	m_behaviors.insert( "Train", std::bind( &Gnome::actionTrain, this, _1 ) );
	m_behaviors.insert( "FindTrainerPosition", std::bind( &Gnome::actionFindTrainerPosition, this, _1 ) );
	m_behaviors.insert( "SuperviseTraining", std::bind( &Gnome::actionSuperviseTraining, this, _1 ) );

	m_behaviors.insert( "GetExitPosition", std::bind( &Gnome::actionGetExitPosition, this, _1 ) );
	m_behaviors.insert( "LeaveMap", std::bind( &Gnome::actionLeaveMap, this, _1 ) );
	m_behaviors.insert( "EnterMap", std::bind( &Gnome::actionEnterMap, this, _1 ) );

	m_behaviors.insert( "DoMission", std::bind( &Gnome::actionDoMission, this, _1 ) );
	m_behaviors.insert( "LeaveForMission", std::bind( &Gnome::actionLeaveForMission, this, _1 ) );
	m_behaviors.insert( "ReturnFromMission", std::bind( &Gnome::actionReturnFromMission, this, _1 ) );

	//m_behaviors.insert( "", std::bind( &Gnome::action, this, _1 ) );

	m_taskFunctions.insert( "Deconstruct", std::bind( &Gnome::deconstruct, this ) );

	m_taskFunctions.insert( "MineWall", std::bind( &Gnome::mineWall, this ) );
	m_taskFunctions.insert( "MineFloor", std::bind( &Gnome::mineFloor, this ) );
	m_taskFunctions.insert( "DigHole", std::bind( &Gnome::digHole, this ) );
	m_taskFunctions.insert( "ExplorativeMineWall", std::bind( &Gnome::explorativeMineWall, this ) );
	m_taskFunctions.insert( "RemoveWall", std::bind( &Gnome::removeWall, this ) );
	m_taskFunctions.insert( "RemoveRamp", std::bind( &Gnome::removeRamp, this ) );
	m_taskFunctions.insert( "RemoveFloor", std::bind( &Gnome::removeFloor, this ) );

	m_taskFunctions.insert( "Construct", std::bind( &Gnome::construct, this ) );
	m_taskFunctions.insert( "ConstructAnimate", std::bind( &Gnome::constructAnimate, this ) );
	m_taskFunctions.insert( "ConstructDugStairs", std::bind( &Gnome::constructDugStairs, this ) );
	m_taskFunctions.insert( "ConstructDugRamp", std::bind( &Gnome::constructDugRamp, this ) );

	m_taskFunctions.insert( "CreateItem", std::bind( &Gnome::createItem, this ) );
	m_taskFunctions.insert( "PlantTree", std::bind( &Gnome::plantTree, this ) );
	m_taskFunctions.insert( "FellTree", std::bind( &Gnome::fellTree, this ) );
	m_taskFunctions.insert( "Harvest", std::bind( &Gnome::harvest, this ) );
	m_taskFunctions.insert( "HarvestHay", std::bind( &Gnome::harvestHay, this ) );
	m_taskFunctions.insert( "Plant", std::bind( &Gnome::plant, this ) );
	m_taskFunctions.insert( "RemovePlant", std::bind( &Gnome::removePlant, this ) );
	m_taskFunctions.insert( "Till", std::bind( &Gnome::till, this ) );
	m_taskFunctions.insert( "Craft", std::bind( &Gnome::craft, this ) );

	m_taskFunctions.insert( "Fish", std::bind( &Gnome::fish, this ) );
	m_taskFunctions.insert( "ButcherFish", std::bind( &Gnome::butcherFish, this ) );
	m_taskFunctions.insert( "ButcherCorpse", std::bind( &Gnome::butcherCorpse, this ) );
	m_taskFunctions.insert( "FillTrough", std::bind( &Gnome::fillTrough, this ) );

	m_taskFunctions.insert( "PrepareSpell", std::bind( &Gnome::prepareSpell, this ) );
	m_taskFunctions.insert( "CastSpell", std::bind( &Gnome::castSpell, this ) );
	m_taskFunctions.insert( "CastSpellAnimate", std::bind( &Gnome::castSpellAnimate, this ) );
	m_taskFunctions.insert( "FinishSpell", std::bind( &Gnome::finishSpell, this ) );

	m_taskFunctions.insert( "SwitchMechanism", std::bind( &Gnome::switchMechanism, this ) );
	m_taskFunctions.insert( "InvertMechanism", std::bind( &Gnome::invertMechanism, this ) );
	m_taskFunctions.insert( "Refuel", std::bind( &Gnome::refuel, this ) );
	m_taskFunctions.insert( "Install", std::bind( &Gnome::install, this ) );
	m_taskFunctions.insert( "Uninstall", std::bind( &Gnome::uninstall, this ) );
	m_taskFunctions.insert( "SoundAlarm", std::bind( &Gnome::soundAlarm, this ) );
	m_taskFunctions.insert( "EquipItem", std::bind( &Gnome::equipItem, this ) );
}

void Gnome::addNeed( QString id, int level )
{
	m_needs.insert( id, level );
}

int Gnome::need( QString id )
{
	if ( m_needs.contains( id ) )
	{
		return m_needs[id].toInt();
	}
	return 0;
}

// =============================================================================
// Mood / Thought System (Milestone 2.1)
// =============================================================================

void Gnome::addThought( QString id, QString text, int moodValue, int durationTicks, int maxStacks, QString cause )
{
	// Check stack count
	int stacks = 0;
	for ( const auto& t : m_thoughts )
	{
		if ( t.id == id ) stacks++;
	}
	if ( stacks >= maxStacks ) return;

	// Apply trait modulation to mood value
	int modulated = moodValue;
	QString modSource;
	if ( id.contains( "Death" ) || id.contains( "Died" ) )
	{
		int empathy = trait( "Empathy" );
		modulated = moodValue + ( moodValue * empathy / 100 );
		if ( empathy != 0 ) modSource = QString( " (Empathy %1%2)" ).arg( empathy > 0 ? "+" : "" ).arg( empathy );
	}
	else if ( id.contains( "Social" ) || id.contains( "Friend" ) || id.contains( "Rival" ) || id.contains( "Lonely" ) || id.contains( "Crowd" ) )
	{
		int sociability = trait( "Sociability" );
		modulated = moodValue + ( moodValue * sociability / 100 );
		if ( sociability != 0 ) modSource = QString( " (Sociability %1%2)" ).arg( sociability > 0 ? "+" : "" ).arg( sociability );
	}
	else if ( id.contains( "Room" ) || id.contains( "Bed" ) || id.contains( "Nice" ) )
	{
		int greed = trait( "Greed" );
		modulated = moodValue + ( moodValue * greed / 100 );
		if ( greed != 0 ) modSource = QString( " (Greed %1%2)" ).arg( greed > 0 ? "+" : "" ).arg( greed );
	}
	else if ( id.contains( "Food" ) || id.contains( "Meal" ) || id.contains( "Hungry" ) || id.contains( "Fed" ) )
	{
		int appetite = trait( "Appetite" );
		modulated = moodValue + ( moodValue * appetite / 100 );
		if ( appetite != 0 ) modSource = QString( " (Appetite %1%2)" ).arg( appetite > 0 ? "+" : "" ).arg( appetite );
	}

	Thought thought;
	thought.id = id;
	thought.text = text;
	thought.cause = cause.isEmpty() ? text + modSource : cause + modSource;
	thought.moodValue = qBound( -30, modulated, 30 );
	thought.ticksLeft = durationTicks;
	thought.initialDuration = durationTicks;
	thought.maxStacks = maxStacks;
	m_thoughts.append( thought );
}

void Gnome::removeThought( QString id )
{
	for ( int i = m_thoughts.size() - 1; i >= 0; --i )
	{
		if ( m_thoughts[i].id == id )
		{
			m_thoughts.removeAt( i );
		}
	}
}

void Gnome::tickThoughts()
{
	for ( int i = m_thoughts.size() - 1; i >= 0; --i )
	{
		m_thoughts[i].ticksLeft--;
		if ( m_thoughts[i].ticksLeft <= 0 )
		{
			m_thoughts.removeAt( i );
		}
	}

	m_mood = calculateMood();

	// Store mood in the existing Happiness need for display compatibility
	m_needs.insert( "Happiness", m_mood );
}

int Gnome::calculateMood() const
{
	// Base mood from Optimism trait: -6 to +6
	int optimism = trait( "Optimism" );
	int baseMood = 50 + ( optimism * 6 / 50 ); // maps -50..+50 to 44..56

	// Sum all active thought values
	int thoughtSum = 0;
	for ( const auto& t : m_thoughts )
	{
		thoughtSum += t.moodValue;
	}

	// Needs affect mood
	int hunger = m_needs.contains( "Hunger" ) ? m_needs["Hunger"].toInt() : 50;
	int thirst = m_needs.contains( "Thirst" ) ? m_needs["Thirst"].toInt() : 50;
	int sleep = m_needs.contains( "Sleep" ) ? m_needs["Sleep"].toInt() : 50;

	int needsPenalty = 0;
	if ( hunger < 20 ) needsPenalty -= ( 20 - hunger ) / 4;  // up to -5
	if ( thirst < 20 ) needsPenalty -= ( 20 - thirst ) / 4;
	if ( sleep < 20 ) needsPenalty -= ( 20 - sleep ) / 4;

	return qBound( 0, baseMood + thoughtSum + needsPenalty, 100 );
}

void Gnome::moodBreakdown( int& outBase, int& outThoughtSum, int& outNeedsPenalty ) const
{
	int optimism = trait( "Optimism" );
	outBase = 50 + ( optimism * 6 / 50 );

	outThoughtSum = 0;
	for ( const auto& t : m_thoughts )
	{
		outThoughtSum += t.moodValue;
	}

	int hunger = m_needs.contains( "Hunger" ) ? m_needs["Hunger"].toInt() : 50;
	int thirst = m_needs.contains( "Thirst" ) ? m_needs["Thirst"].toInt() : 50;
	int sleep = m_needs.contains( "Sleep" ) ? m_needs["Sleep"].toInt() : 50;

	outNeedsPenalty = 0;
	if ( hunger < 20 ) outNeedsPenalty -= ( 20 - hunger ) / 4;
	if ( thirst < 20 ) outNeedsPenalty -= ( 20 - thirst ) / 4;
	if ( sleep < 20 ) outNeedsPenalty -= ( 20 - sleep ) / 4;
}

float Gnome::moodWorkSpeedModifier() const
{
	// Mood 50 = 1.0x, mood 0 = 0.7x, mood 100 = 1.3x
	return 0.7f + ( m_mood / 100.0f ) * 0.6f;
}

QString Gnome::mentalBreakType() const
{
	int temper = trait( "Temper" );
	int nerve = trait( "Nerve" );
	int optimism = trait( "Optimism" );

	if ( temper > 15 ) return "Tantrum";
	if ( nerve < -15 ) return "Catatonic";
	if ( optimism < -15 ) return "SadWander";
	return "SadWander"; // default
}

void Gnome::selectProfession( QString profession )
{
	if ( m_profession != profession )
	{
		m_profession = profession;

		clearAllSkills();

		m_skillPriorities = g->gm()->professionSkills( profession );

		for ( auto skill : m_skillPriorities )
		{
			setSkillActive( skill, true );
		}
	}
}

// Dispatcher: routes to fullTick or cheapTick based on bucket assignment
CreatureTickResult Gnome::onTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( m_forceFullTick || ( m_bucket == static_cast<int>( tickNumber % GnomeManager::N_BUCKETS ) ) )
	{
		return fullTick( tickNumber, seasonChanged, dayChanged, hourChanged, minuteChanged );
	}
	return cheapTick( tickNumber );
}

// Cheap tick: survival checks, movement along cached path, work timer check
// No BT evaluation, no need evaluation, no job search
CreatureTickResult Gnome::cheapTick( quint64 tickNumber )
{
	processCooldowns( tickNumber );

	// Survival-critical checks
	if ( checkFloor() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::NOFLOOR;
	}
	m_anatomy.setFluidLevelonTile( g->w()->fluidLevel( m_position ) );
	if ( m_anatomy.statusChanged() )
	{
		auto status = m_anatomy.status();
		if ( status & AS_DEAD )
		{
			die();
		}
	}
	if ( isDead() )
	{
		m_expires    = GameState::tick + Global::util->ticksPerDay * 2;
		m_lastOnTick = tickNumber;
		return CreatureTickResult::DEAD;
	}

	Position oldPos = m_position;

	// State-aware cheap tick (Phase F)
	switch ( m_gnomeState )
	{
		case GnomeState::MOVING:
			if ( !m_currentPath.empty() && m_moveCooldown <= 0 )
			{
				if ( !moveOnPath() )
				{
					m_gnomeState = GnomeState::IDLE;
					m_forceFullTick = true;
				}
				if ( m_currentPath.empty() )
				{
					m_forceFullTick = true;
				}
			}
			break;

		case GnomeState::WORKING:
			if ( m_taskFinishTick > 0 && GameState::tick >= m_taskFinishTick )
			{
				m_forceFullTick = true;
			}
			break;

		default:
			// IDLE, THINKING, NEEDS, COMBAT — should only happen on full tick
			m_forceFullTick = true;
			break;
	}

	// Job canceled/aborted while in cheap tick
	if ( m_job && ( m_job->isAborted() || m_job->isCanceled() ) )
	{
		m_forceFullTick = true;
	}

	// Interrupt and combat flags
	if ( m_combatAlert || m_interruptFlag )
	{
		m_gnomeState = GnomeState::IDLE;
		m_forceFullTick = true;
	}

	move( oldPos );
	updateLight( oldPos, m_position );

	m_lastOnTick = tickNumber;
	return CreatureTickResult::OK;
}

// Full tick: complete BT evaluation, needs, job decisions
CreatureTickResult Gnome::fullTick( quint64 tickNumber, bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	m_forceFullTick = false;
	m_interruptFlag = false;
	m_combatAlert   = false;

	processCooldowns( tickNumber );

	if ( !m_isOnMission )
	{
		evalNeeds( seasonChanged, dayChanged, hourChanged, minuteChanged );
	}

	m_jobChanged    = false;
	Position oldPos = m_position;

	if ( checkFloor() )
	{
		m_lastOnTick = tickNumber;
		return CreatureTickResult::NOFLOOR;
	}
	m_anatomy.setFluidLevelonTile( g->w()->fluidLevel( m_position ) );
	if ( m_anatomy.statusChanged() )
	{
		auto status = m_anatomy.status();
		if ( status & AS_DEAD )
		{
			Global::logger().log( LogType::COMBAT, m_name + "died. Bummer!", m_id );
			die();
			// TODO check for other statuses
		}
	}

	if ( isDead() )
	{
		qDebug() << m_name << " expires " << GameState::tick + Global::util->ticksPerDay;
		m_expires    = GameState::tick + Global::util->ticksPerDay * 2;
		m_lastOnTick = tickNumber;
		return CreatureTickResult::DEAD;
	}

	if ( m_job && ( m_job->isAborted() || m_job->isCanceled() ) )
	{
		qDebug() << m_job->type() << " job is canceled";
		cleanUpJob( false );
		m_behaviorTree->halt();
	}

	if ( !m_carryBandages && m_carriedBandages > 0 )
	{
		while ( m_carriedBandages > 0 )
		{
			if ( m_inventoryItems.size() > 0 )
			{
				auto item = m_inventoryItems.takeFirst();
				if ( g->inv()->itemSID( item ) == "Bandage" )
				{
					g->inv()->setInJob( item, 0 );
					g->inv()->putDownItem( item, m_position );
					--m_carriedBandages;
				}
				else
				{
					m_carriedItems.append( item );
				}
			}
			else
			{
				m_carriedBandages = 0;
				m_carriedFood     = 0;
				m_carriedDrinks   = 0;
			}
		}
	}
	if ( !m_carryFood && m_carriedFood > 0 )
	{
		while ( m_carriedFood > 0 )
		{
			if ( m_inventoryItems.size() > 0 )
			{
				auto item = m_inventoryItems.takeFirst();
				if ( g->inv()->nutritionalValue( item ) > 0 )
				{
					g->inv()->setInJob( item, 0 );
					g->inv()->putDownItem( item, m_position );
					--m_carriedFood;
				}
				else
				{
					m_carriedItems.append( item );
				}
			}
			else
			{
				m_carriedBandages = 0;
				m_carriedFood     = 0;
				m_carriedDrinks   = 0;
			}
		}
	}
	if ( !m_carryDrinks && m_carriedDrinks > 0 )
	{
		while ( m_carriedDrinks > 0 )
		{
			if ( m_inventoryItems.size() > 0 )
			{
				auto item = m_inventoryItems.takeFirst();
				if ( g->inv()->drinkValue( item ) > 0 )
				{
					g->inv()->setInJob( item, 0 );
					g->inv()->putDownItem( item, m_position );
					--m_carriedDrinks;
				}
				else
				{
					m_carriedItems.append( item );
				}
			}
			else
			{
				m_carriedBandages = 0;
				m_carriedFood     = 0;
				m_carriedDrinks   = 0;
			}
		}
	}

#ifdef CHECKTIME
	QElapsedTimer timer;
	timer.start();
	if ( m_behaviorTree )
	{
		m_behaviorTree->tick();
	}
	auto elapsed = timer.elapsed();
	if ( elapsed > 100 )
	{
		qDebug() << m_name << "just needed" << elapsed << "ms for bt tick";
		Global::cfg->set( "Pause", true );
	}
#else
	if ( m_behaviorTree )
	{
		if ( Global::debugMode )
		{
			QElapsedTimer et;
			et.start();

			m_behaviorTree->tick();

			auto ela = et.elapsed();
			if ( ela > 30 )
			{
				if ( m_job )
				{
					qDebug() << "LOOPTIME" << m_name << ela << "ms"
							 << "job:" << m_job->type() << m_job->pos().toString();
				}
			}
		}
		else
		{
			m_behaviorTree->tick();
		}
	}
#endif

	move( oldPos );
	updateLight( oldPos, m_position );

	m_lastOnTick = tickNumber;

	if ( m_jobChanged )
	{
		m_idleTickCount = 0;
		return CreatureTickResult::JOBCHANGED;
	}

	// Phase D: sleep trigger — if idle with no job and no pending jobs for 30+ ticks
	if ( !m_job && m_pendingJobs.isEmpty() )
	{
		m_idleTickCount++;
		if ( m_idleTickCount >= 30 )
		{
			m_idleTickCount = 0;
			g->gm()->sleepGnome( this );
		}
	}
	else
	{
		m_idleTickCount = 0;
	}

	return CreatureTickResult::OK;
}

void Gnome::die()
{
	Creature::die();
	cleanUpJob( false );

	// Clear workshop assignments
	for ( Workshop* w : g->wsm()->workshops() )
	{
		if ( w->assignedGnome() == id() )
		{
			w->assignGnome( 0 );
		}
	}

	// Clear room ownership
	for ( auto room : g->rm()->allRooms() )
	{
		if ( room->owner() == id() )
		{
			room->setOwner( 0 );
		}
	}

	Global::logger().log( LogType::DEATH, m_name + " has died.", m_id );
}

bool Gnome::checkFloor()
{
	FloorType ft = g->w()->floorType( m_position );
	if ( ft == FloorType::FT_NOFLOOR )
	{
		if ( m_job )
		{
			m_job->setAborted( true );
		}

		Position oneDown( m_position.x, m_position.y, m_position.z - 1 );
		forceMove( oneDown );
		g->w()->discover( oneDown );
		return true;
	}
	return false;
}

void Gnome::setJobAborted( QString caller )
{
	if ( m_job )
	{
		m_job->setAborted( true );
		log( "GnomeManager aborted my job." );
	}
}

bool Gnome::evalNeeds( bool seasonChanged, bool dayChanged, bool hourChanged, bool minuteChanged )
{
	if ( seasonChanged )
	{
		//log( GameState::CurrentYearAndSeason" ).toString() );
	}

	bool needAction        = false;
	unsigned char priority = 0;
	QString action;
	QString needKey;

	if ( minuteChanged )
	{
		for ( auto need : Global::needIDs )
		{
			//update need values
			float decay  = Global::needDecays.value( need );
			float oldVal = m_needs[need].toFloat();
			float newVal = qMax( -100.f, qMin( 150.f, oldVal + decay ) );

			m_needs.insert( need, newVal );

			if ( need == "Hunger" || need == "Thirst" )
			{
				if ( newVal <= -100.f )
				{
					m_thoughtBubble = "";
					die();
					if ( need == "Hunger" )
					{
						log( "Starved to death." );
					}
					else if ( need == "Thirst" )
					{
						log( "Died from thirst." );
					}
				}
			}
		}

		// =====================================================================
		// Generate mood thoughts from various sources (~120 distinct thoughts)
		// =====================================================================

		int hunger = m_needs.contains( "Hunger" ) ? m_needs["Hunger"].toInt() : 50;
		int thirst = m_needs.contains( "Thirst" ) ? m_needs["Thirst"].toInt() : 50;
		int sleepNeed = m_needs.contains( "Sleep" ) ? m_needs["Sleep"].toInt() : 50;
		int bravery = trait( "Bravery" );
		int sociability = trait( "Sociability" );
		int industriousness = trait( "Industriousness" );
		int appetite = trait( "Appetite" );
		int temper = trait( "Temper" );
		int creativity = trait( "Creativity" );
		int greed = trait( "Greed" );
		int curiosity = trait( "Curiosity" );
		int empathy = trait( "Empathy" );
		int stubbornness = trait( "Stubbornness" );
		int optimism = trait( "Optimism" );
		int nerve = trait( "Nerve" );

		// =================================================================
		// 1. NEEDS-BASED THOUGHTS (12)
		// =================================================================
		if ( hunger < 5 )         addThought( "Starving", "Starving to death", -10, 600, 1 );
		else if ( hunger < 15 )   addThought( "VeryHungry", "Desperately hungry", -6, 500, 1 );
		else if ( hunger < 30 )   addThought( "Hungry", "Getting hungry", -3, 300, 1 );
		else if ( hunger > 90 )   addThought( "Stuffed", "Pleasantly full", 3, 400, 1 );
		else if ( hunger > 75 )   addThought( "WellFed", "Well fed", 2, 400, 1 );

		if ( thirst < 5 )         addThought( "DyingOfThirst", "Dying of thirst", -10, 600, 1 );
		else if ( thirst < 15 )   addThought( "VeryThirsty", "Parched", -6, 500, 1 );
		else if ( thirst < 30 )   addThought( "Thirsty", "Getting thirsty", -3, 300, 1 );
		else if ( thirst > 90 )   addThought( "WellHydrated", "Nicely hydrated", 3, 400, 1 );
		else if ( thirst > 75 )   addThought( "Refreshed", "Had a good drink", 2, 400, 1 );

		if ( sleepNeed < 10 )     addThought( "PassingOut", "About to collapse from exhaustion", -8, 400, 1 );
		else if ( sleepNeed < 20 ) addThought( "Exhausted", "Exhausted", -5, 400, 1 );
		else if ( sleepNeed < 35 ) addThought( "Tired", "Getting tired", -2, 300, 1 );
		else if ( sleepNeed > 90 ) addThought( "WellRested", "Well rested and alert", 4, 500, 1 );
		else if ( sleepNeed > 75 ) addThought( "Rested", "Feeling rested", 2, 400, 1 );

		// =================================================================
		// 2. ACTIVITY / WORK THOUGHTS (30)
		// =================================================================
		if ( m_job )
		{
			QString skill = m_job->requiredSkill();

			// General work satisfaction
			addThought( "Working", "Productive day", 1, 200, 1 );

			// Industriousness-driven
			if ( industriousness > 25 )
				addThought( "LovesWork", "In the zone — nothing beats hard work", 3, 400, 1 );
			else if ( industriousness > 10 )
				addThought( "EnjoyWork", "Enjoying the work", 2, 300, 1 );
			else if ( industriousness < -25 )
				addThought( "GrudgingWork", "Would rather be napping", -2, 200, 1 );

			// Craft skill satisfaction
			if ( creativity > 15 && ( skill == "Stonecarving" || skill == "Woodcarving" ||
				skill == "Pottery" || skill == "JewelryMaking" || skill == "Weaving" ||
				skill == "Tailoring" || skill == "GlassMaking" ) )
				addThought( "CreativeWork", "Doing creative work", 3, 400, 1 );

			// Combat training / military
			if ( skill == "Melee" || skill == "Ranged" || skill == "Dodge" ||
				skill == "Block" || skill == "Unarmed" || skill == "Crossbow" )
			{
				if ( bravery > 15 )
					addThought( "EnjoysCombatTraining", "Enjoying combat training", 3, 400, 1 );
				else if ( bravery < -15 )
					addThought( "DreadsCombat", "Dreading this fighting", -3, 300, 1 );
			}

			// Mining
			if ( skill == "Mining" )
			{
				if ( curiosity > 15 )
					addThought( "MiningExcitement", "Wondering what's deeper down", 2, 300, 1 );
				addThought( "MiningWork", "Swinging the pickaxe", 1, 200, 1 );
			}

			// Farming
			if ( skill == "Farming" || skill == "Horticulture" )
			{
				addThought( "FarmingWork", "Working the soil", 1, 200, 1 );
				if ( appetite > 15 )
					addThought( "FarmerGourmand", "Growing ingredients for fine meals", 2, 300, 1 );
			}

			// Cooking / Brewing
			if ( skill == "Cooking" )
				addThought( "CookingWork", "Preparing a meal", 2, 300, 1 );
			if ( skill == "Brewing" )
				addThought( "BrewingWork", "Crafting a fine brew", 2, 300, 1 );

			// Construction
			if ( skill == "Construction" || skill == "Masonry" )
				addThought( "BuildingWork", "Building something lasting", 2, 300, 1 );

			// Engineering / Tinkering
			if ( skill == "Tinkering" || skill == "Engineering" || skill == "Machining" )
			{
				if ( curiosity > 15 )
					addThought( "TinkeringJoy", "Fascinated by the mechanism", 3, 400, 1 );
				addThought( "EngineerWork", "Working with gears and levers", 1, 200, 1 );
			}

			// Smithing
			if ( skill == "Blacksmithing" || skill == "Smelting" || skill == "WeaponCrafting" || skill == "ArmorCrafting" )
				addThought( "SmithWork", "The heat of the forge", 2, 300, 1 );

			// Woodcutting
			if ( skill == "Woodcutting" )
				addThought( "WoodcuttingWork", "Felling timber", 1, 200, 1 );

			// Medical
			if ( skill == "Medic" || skill == "Caretaking" )
			{
				if ( empathy > 15 )
					addThought( "HealingOthers", "Helping someone in pain", 4, 400, 1 );
				else
					addThought( "MedicalDuty", "Tending to the wounded", 1, 200, 1 );
			}

			// Hauling (the boring job)
			if ( skill == "Hauling" )
			{
				if ( industriousness > 20 )
					addThought( "HaulingDiligent", "Keeping things organized", 1, 200, 1 );
				else if ( industriousness < -10 )
					addThought( "HaulingBoring", "Hauling is so tedious", -1, 200, 1 );
			}

			// Butchery
			if ( skill == "Butchery" )
			{
				if ( empathy > 20 )
					addThought( "ButcheryDiscomfort", "Unpleasant work at the butcher's block", -2, 300, 1 );
			}

			// Gemcutting / Jewelry
			if ( skill == "Gemcutting" || skill == "JewelryMaking" )
			{
				if ( greed > 15 )
					addThought( "GemGreed", "Admiring the sparkle of gems", 3, 400, 1 );
			}
		}
		else
		{
			// Idle thoughts
			if ( industriousness > 25 )
				addThought( "IdleFrustrated", "Restless — nothing to do", -3, 200, 1 );
			else if ( industriousness > 10 )
				addThought( "IdleAntsy", "Looking for something useful to do", -1, 200, 1 );
			else if ( industriousness < -25 )
				addThought( "IdleHappy", "Enjoying a well-deserved break", 3, 300, 1 );
			else if ( industriousness < -10 )
				addThought( "IdleRelaxed", "Taking it easy", 1, 200, 1 );
		}

		// =================================================================
		// 3. ENVIRONMENT THOUGHTS (25)
		// =================================================================
		bool inSunlight = g->w()->hasSunlight( m_position );
		bool underground = ( m_position.z < GameState::groundLevel - 3 );
		bool deepUnderground = ( m_position.z < GameState::groundLevel - 15 );
		TileFlag flags = g->w()->getTileFlag( m_position );

		// Sunlight
		if ( inSunlight )
		{
			addThought( "Sunlight", "Enjoying the sunshine", 2, 300, 1 );
			if ( optimism > 15 )
				addThought( "SunnyOptimist", "What a beautiful day", 2, 300, 1 );
		}

		// Underground depth
		if ( deepUnderground )
		{
			if ( curiosity > 20 )
				addThought( "DeepExplorer", "Exploring the deep places of the world", 4, 500, 1 );
			else if ( curiosity > 0 )
				addThought( "DeepCurious", "Wondering what lies further down", 1, 300, 1 );
			else if ( nerve < -15 )
				addThought( "DeepFear", "The darkness presses in from all sides", -4, 400, 1 );
			else if ( curiosity < -15 )
				addThought( "DeepUnease", "Uneasy this far underground", -3, 300, 1 );
		}
		else if ( underground )
		{
			if ( curiosity > 20 )
				addThought( "UndergroundExplorer", "Interesting geology down here", 2, 300, 1 );
			else if ( curiosity < -20 )
				addThought( "UndergroundDislike", "Miss the open sky", -2, 300, 1 );
		}

		// On the surface
		if ( !underground && !inSunlight )
		{
			// Overcast / night
			if ( optimism < -15 )
				addThought( "GloomyDay", "Another grey day", -1, 200, 1 );
		}

		// Near water (on a tile with fluid)
		if ( flags & TileFlag::TF_WATER )
			addThought( "NearWater", "The sound of flowing water is calming", 2, 300, 1 );

		// In a stockpile (standing on stored goods)
		if ( flags & TileFlag::TF_STOCKPILE )
		{
			if ( greed > 20 )
				addThought( "SurroundedByGoods", "Surrounded by wealth", 2, 300, 1 );
		}

		// In a workshop
		if ( flags & TileFlag::TF_WORKSHOP )
		{
			if ( creativity > 20 )
				addThought( "WorkshopInspiration", "Inspired by the workshop", 1, 200, 1 );
		}

		// In a room (bedroom, dining hall, etc.)
		if ( flags & TileFlag::TF_ROOM )
		{
			addThought( "Indoors", "Sheltered and comfortable", 1, 200, 1 );
			if ( greed > 20 )
				addThought( "NiceRoom", "Appreciating the room", 2, 300, 1 );
		}

		// On a farm
		if ( flags & TileFlag::TF_FARM )
		{
			addThought( "OnFarm", "Fresh air and growing things", 1, 200, 1 );
		}

		// In a grove
		if ( flags & TileFlag::TF_GROVE )
		{
			addThought( "InGrove", "Peaceful among the trees", 2, 300, 1 );
		}

		// =================================================================
		// 4. SOCIAL THOUGHTS (20)
		// =================================================================
		bool anyoneNearby = false;
		int nearbyGnomeCount = 0;
		int nearbyFriends = 0;
		int nearbyRivals = 0;
		for ( auto other : g->gm()->gnomes() )
		{
			if ( other->id() == id() || other->isDead() ) continue;
			Position otherPos = other->getPos();
			int dist = abs( m_position.x - otherPos.x ) + abs( m_position.y - otherPos.y ) + abs( m_position.z - otherPos.z );
			if ( dist > 8 ) continue;

			anyoneNearby = true;
			nearbyGnomeCount++;

			if ( dist <= 5 )
			{
				int op = g->gm()->opinion( id(), other->id() );
				if ( op > 50 )
				{
					addThought( "BestFriendNearby", "Near my best friend", 5, 250, 1 );
					nearbyFriends++;
				}
				else if ( op > 30 )
				{
					addThought( "FriendNearby", "Near a close friend", 3, 200, 2 );
					nearbyFriends++;
				}
				else if ( op > 10 )
				{
					addThought( "FriendlyNearby", "Pleasant company", 1, 200, 2 );
				}
				else if ( op < -50 )
				{
					addThought( "ArchRivalNearby", "Near someone I despise", -5, 250, 1 );
					nearbyRivals++;
				}
				else if ( op < -30 )
				{
					addThought( "RivalNearby", "Near a rival", -3, 200, 2 );
					nearbyRivals++;
				}
				else if ( op < -10 )
				{
					addThought( "DislikedNearby", "Near someone annoying", -1, 200, 2 );
				}
			}
		}

		// Crowd thoughts
		if ( nearbyGnomeCount >= 5 )
		{
			if ( sociability > 15 )
				addThought( "LovesCrowds", "Great to be around so many people", 3, 300, 1 );
			else if ( sociability < -15 )
				addThought( "Crowded", "Too many people around", -3, 300, 1 );
		}

		// Loneliness / solitude
		if ( !anyoneNearby )
		{
			if ( sociability > 25 )
				addThought( "VeryLonely", "So alone — need company", -4, 400, 1 );
			else if ( sociability > 10 )
				addThought( "Lonely", "Feeling lonely", -2, 300, 1 );
			else if ( sociability < -25 )
				addThought( "BlissfulSolitude", "Perfect peace and quiet", 4, 400, 1 );
			else if ( sociability < -10 )
				addThought( "Solitude", "Enjoying the solitude", 2, 300, 1 );
		}

		// Social satisfaction
		if ( nearbyFriends >= 2 )
			addThought( "AmongFriends", "Surrounded by friends", 4, 300, 1 );
		if ( nearbyRivals >= 2 )
			addThought( "TooManyRivals", "Too many enemies around", -4, 300, 1 );

		// =================================================================
		// 5. PERSONALITY-DRIVEN AMBIENT THOUGHTS (25)
		// =================================================================

		// Optimism / Pessimism (always-on baseline)
		if ( optimism > 30 )
			addThought( "NaturallyHappy", "Life is good", 3, 500, 1 );
		else if ( optimism > 15 )
			addThought( "OptimisticOutlook", "Things are looking up", 1, 400, 1 );
		else if ( optimism < -30 )
			addThought( "NaturallyGloomy", "Everything feels hopeless", -3, 500, 1 );
		else if ( optimism < -15 )
			addThought( "PessimisticOutlook", "Something bad will happen soon", -1, 400, 1 );

		// Bravery-related ambient thoughts
		if ( bravery > 30 )
			addThought( "FeelingBrave", "Ready for anything", 2, 400, 1 );
		else if ( bravery < -30 )
			addThought( "FeelingAnxious", "Jumping at shadows", -2, 400, 1 );

		// Greed-related
		if ( greed > 30 && hunger > 50 && thirst > 50 && sleepNeed > 50 )
			addThought( "WantsMore", "Need finer things in life", -1, 300, 1 );
		else if ( greed < -20 && hunger > 40 && thirst > 40 )
			addThought( "ContentSimple", "Simple life is the best life", 2, 400, 1 );

		// Appetite-related
		if ( appetite > 25 && hunger > 50 && hunger < 80 )
			addThought( "CravingBetterFood", "Could really go for a proper meal", -1, 300, 1 );
		else if ( appetite < -20 && hunger > 30 )
			addThought( "FoodIsJustFuel", "Food is fuel, nothing more", 1, 300, 1 );

		// Temper
		if ( temper > 30 )
			addThought( "SimmingAnger", "Irritated at the world", -2, 400, 1 );
		else if ( temper < -25 )
			addThought( "InnerPeace", "Feeling calm and centered", 2, 400, 1 );

		// Stubbornness
		if ( stubbornness > 30 )
			addThought( "Determined", "Won't give up", 1, 300, 1 );
		else if ( stubbornness < -25 )
			addThought( "Adaptable", "Rolling with the punches", 1, 300, 1 );

		// Empathy ambient
		if ( empathy > 30 && g->gm()->numGnomes() > 0 )
			addThought( "CaresAboutOthers", "Hoping everyone is doing well", 1, 300, 1 );
		else if ( empathy < -25 )
			addThought( "SelfFocused", "Looking out for number one", 0, 300, 1 );

		// Nerve
		if ( nerve > 25 )
			addThought( "SteadyNerves", "Cool under pressure", 2, 400, 1 );
		else if ( nerve < -25 )
			addThought( "OnEdge", "On edge — something could go wrong", -2, 400, 1 );

		// Curiosity ambient
		if ( curiosity > 25 && !m_job )
			addThought( "WantsToExplore", "Itching to discover something new", 1, 300, 1 );

		// =================================================================
		// 6. KINGDOM / SITUATION THOUGHTS (10)
		// =================================================================
		int gnomeCount = g->gm()->numGnomes();

		// Colony size feelings
		if ( gnomeCount == 1 )
			addThought( "OnlyOne", "All alone in this kingdom", -4, 500, 1 );
		else if ( gnomeCount <= 3 )
			addThought( "SmallColony", "We're a tiny group — need more gnomads", -1, 400, 1 );
		else if ( gnomeCount >= 15 )
			addThought( "ThrivingColony", "The kingdom is thriving", 3, 500, 1 );
		else if ( gnomeCount >= 8 )
			addThought( "GrowingColony", "The colony is growing nicely", 1, 400, 1 );

		// Season thoughts
		if ( GameState::seasonString == "Spring" )
			addThought( "SpringFeeling", "The world feels fresh and new", 2, 600, 1 );
		else if ( GameState::seasonString == "Summer" )
			addThought( "SummerWarmth", "Warm days and long evenings", 1, 600, 1 );
		else if ( GameState::seasonString == "Autumn" )
			addThought( "AutumnMelancholy", "The leaves are turning", 0, 600, 1 );
		else if ( GameState::seasonString == "Winter" )
		{
			if ( optimism > 10 )
				addThought( "WinterCozy", "Cozy season by the fire", 1, 600, 1 );
			else
				addThought( "WinterCold", "The cold seeps in", -1, 600, 1 );
		}

		// Time of day thoughts
		if ( GameState::hour >= 22 || GameState::hour < 5 )
		{
			if ( sleepNeed < 50 )
				addThought( "LateNight", "Should be sleeping", -2, 100, 1 );
		}
		else if ( GameState::hour >= 6 && GameState::hour < 9 )
		{
			if ( sleepNeed > 60 )
				addThought( "MorningFresh", "Fresh start to the day", 2, 200, 1 );
		}

		// =================================================================
		// 7. STATUS THOUGHTS (5)
		// =================================================================
		if ( m_isTrapped )
			addThought( "Trapped", "Trapped! Can't reach anything!", -10, 100, 1 );

		if ( GameState::lockdown )
		{
			if ( bravery > 15 )
				addThought( "LockdownBrave", "We should be out there fighting", -1, 200, 1 );
			else if ( nerve < -10 )
				addThought( "LockdownScared", "Thank goodness for the lockdown", 2, 200, 1 );
			else
				addThought( "LockdownNeutral", "Confined during lockdown", -1, 200, 1 );
		}
	}

	// Corpse mood penalty — check for nearby unburied corpses (hourly)
	if ( hourChanged )
	{
		// Check dead gnomes nearby
		for ( auto dg : g->gm()->deadGnomes() )
		{
			if ( dg->isBuried() ) continue;
			int dist = m_position.distSquare( dg->getPos() );
			if ( dist <= 64 ) // within ~8 tiles
			{
				switch ( dg->rotStage() )
				{
					case RotStage::Decaying:
						addThought( "CorpseDecay", "Saw a decaying corpse", -3, Global::util->ticksPerDay, 2 );
						break;
					case RotStage::Rotting:
						addThought( "CorpseRot", "Horrible rotting corpse nearby", -6, Global::util->ticksPerDay * 2, 2 );
						// Disease risk: 5% chance per day
						if ( rand() % 20 == 0 )
						{
							addThought( "CorpseSick", "Feeling sick from the rot", -10, Global::util->ticksPerDay * 3, 1 );
						}
						break;
					case RotStage::Skeleton:
						addThought( "CorpseSkeleton", "Unburied remains", -2, Global::util->ticksPerDay, 2 );
						break;
					default:
						break;
				}
				break; // one corpse penalty at a time
			}
		}
		// Check dead creatures nearby
		for ( auto dc : g->cm()->deadCreatures() )
		{
			if ( dc->isBuried() ) continue;
			int dist = m_position.distSquare( dc->getPos() );
			if ( dist <= 64 && dc->rotStage() == RotStage::Rotting )
			{
				addThought( "AnimalCorpseRot", "Rotting animal corpse nearby", -4, Global::util->ticksPerDay, 2 );
				break;
			}
		}
	}

	// Tick thoughts every tick (decrement timers, recalculate mood)
	tickThoughts();

	// Mental break check
	if ( m_mood < 5 && !m_mentalBreak )
	{
		m_mentalBreak = true;
		QString breakType = mentalBreakType();
		Global::logger().log( LogType::WARNING, m_name + " is having a mental break! (" + breakType + ")", m_id );
	}
	else if ( m_mood > 35 )
	{
		if ( m_mentalBreak )
		{
			m_mentalBreak = false;
			// Catharsis: +40 mood equivalent for 2500 ticks (~2.5 in-game days)
			addThought( "Catharsis", "Feeling better after a breakdown", 15, 2500, 1 );
		}
	}

	// Trapped gnome detection — check once per in-game hour
	if ( hourChanged && GameState::tick > m_trappedCheckTick )
	{
		m_trappedCheckTick = GameState::tick + Global::util->ticksPerDay / 24;

		bool canReachAnything = false;
		auto stockpiles      = g->spm()->allStockpiles();
		for ( auto spID : stockpiles )
		{
			Stockpile* sp = g->spm()->getStockpile( spID );
			if ( sp && !sp->getFields().isEmpty() )
			{
				Position spPos( sp->getFields().firstKey() );
				if ( g->m_world->regionMap().checkConnectedRegions( m_position, spPos ) )
				{
					canReachAnything = true;
					break;
				}
			}
		}

		if ( !canReachAnything && !stockpiles.isEmpty() )
		{
			if ( !m_isTrapped )
			{
				m_isTrapped = true;
				setThoughtBubble( "Trapped" );
				Global::logger().log( LogType::WARNING, m_name + " is trapped and cannot reach any stockpile!", m_id );
				qDebug() << m_name << "is TRAPPED at" << m_position.toString();
			}
		}
		else
		{
			m_isTrapped = false;
		}
	}

	return true;
}

void Gnome::updateLight( Position oldPos, Position newPos )
{
#if 0
	if( oldPos == newPos )
	{
		return;
	}
	World& w = Global::w();
	int intensity = 10;
	w.putLight( oldPos, -intensity );
	w.putLight( oldPos.x - 1, oldPos.y, oldPos.z, -intensity );
	w.putLight( oldPos.x, oldPos.y - 1, oldPos.z, -intensity );
	w.putLight( oldPos.x + 1, oldPos.y, oldPos.z, -intensity );
	w.putLight( oldPos.x, oldPos.y + 1, oldPos.z, -intensity );

	w.putLight( newPos, intensity );
	w.putLight( newPos.x - 1, newPos.y, newPos.z, intensity );
	w.putLight( newPos.x, newPos.y - 1, newPos.z, intensity );
	w.putLight( newPos.x + 1, newPos.y, newPos.z, intensity );
	w.putLight( newPos.x, newPos.y + 1, newPos.z, intensity );
#endif
}

QString Gnome::getActivity()
{
	if ( m_job )
	{
		return m_job->requiredSkill();
	}
	else
	{
		return "idle";
	}
}

void Gnome::setOwnedRoom( unsigned int id )
{
	m_equipment.roomID = id;
	if ( id )
	{
		log( GameState::currentDayTime + ": I got a new room." );
	}
	else
	{
		log( GameState::currentDayTime + ": They took my room away." );
	}
}

unsigned int Gnome::ownedRoom()
{
	return m_equipment.roomID;
}

ScheduleActivity Gnome::schedule( unsigned char hour )
{
	if ( hour < 24 && m_schedule.size() == 24 )
	{
		return m_schedule[hour];
	}
	return ScheduleActivity::None;
}

void Gnome::setSchedule( unsigned char hour, ScheduleActivity activity )
{
	if ( hour < 24 && m_schedule.size() == 24 )
	{
		m_schedule[hour] = activity;
	}
	else
	{
		m_schedule.clear();
		for ( int i = 0; i < 24; ++i )
		{
			m_schedule.append( ScheduleActivity::None );
		}
		m_schedule[hour] = activity;
	}
}

void Gnome::updateMoveSpeed()
{
	int skill   = getSkillLevel( "Hauling" );
	int speed   = DB::execQuery( "SELECT Speed FROM MoveSpeed WHERE CREATURE = \"Gnome\" AND Skill = \"" + QString::number( skill ) + "\"" ).toInt();
	m_moveSpeed = qMax( 30, speed );
}

void Gnome::assignWorkshop( unsigned int id )
{
	m_assignedWorkshop = id;
}

Equipment Gnome::equipment()
{
	return m_equipment;
}

QString Gnome::rightHandItem()
{
	return m_equipment.rightHandHeld.material + " " + m_equipment.rightHandHeld.item;
}

QString Gnome::rightHandAttackSkill()
{
	return QString::number( m_rightHandAttackSkill );
}

QString Gnome::rightHandAttackValue()
{
	return QString::number( m_rightHandAttackValue );
}

void Gnome::updateAttackValues()
{
	m_leftHandAttackSkill  = getSkillLevel( "Unarmed" );
	m_leftHandAttackValue  = attribute( "Str" );
	m_rightHandAttackSkill = getSkillLevel( "Unarmed" );
	m_rightHandAttackValue = attribute( "Str" );
}

bool Gnome::attack( DamageType dt, AnatomyHeight da, int skill, int strength, Position sourcePos, unsigned int attackerID )
{
	srand( std::chrono::system_clock::now().time_since_epoch().count() );

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
		int diff = dodge - skill;
		diff     = qMax( 5, 20 - diff );
		hit |= rand() % 100 > diff;
	}

	if ( hit ) // check block
	{
	}

	if ( hit )
	{
		Global::logger().log( LogType::COMBAT, m_name + " took " + QString::number( strength ) + " damage.", m_id );
		m_anatomy.damage( &m_equipment, dt, da, ds, strength );
		g->em()->recordCombatEvent( true, false, true ); // gnome wounded
	}
	else
	{
		Global::logger().log( LogType::COMBAT, m_name + " dodged the attack. Skill:" + QString::number( skill ) + " Dodge: " + QString::number( dodge ), m_id );
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

void Gnome::setAllowedCarryItems( bool bandages, bool food, bool drinks )
{
	m_carryBandages = bandages;
	m_carryFood     = food;
	m_carryDrinks   = drinks;
}