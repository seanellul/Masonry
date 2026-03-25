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
#include "aggregatorcreatureinfo.h"

#include "../base/db.h"
#include "../base/dbhelper.h"
#include "../base/global.h"
#include "../base/util.h"

#include "../game/game.h"
#include "../game/creaturemanager.h"
#include "../game/gnomemanager.h"
#include "../game/militarymanager.h"

#include "../gfx/spritefactory.h"

#include <algorithm>

#include "../gui/strings.h"

AggregatorCreatureInfo::AggregatorCreatureInfo( QObject* parent ) :
	QObject(parent)
{
}

void AggregatorCreatureInfo::init( Game* game )
{
	g = game;
}

void AggregatorCreatureInfo::update()
{
	if( m_currentID != 0 )
	{
		onRequestCreatureUpdate( m_currentID );
	}
}

void AggregatorCreatureInfo::onRequestCreatureUpdate( unsigned int id )
{
	if( !g ) return;
	m_currentID = id;
	auto gnome = g->gm()->gnome( id );
	if( gnome )
	{
		m_info.name = gnome->name();
		m_info.id = id;
		m_info.profession = gnome->profession();

		m_info.str = gnome->attribute( "Str" );
		m_info.con = gnome->attribute( "Con" );
		m_info.dex = gnome->attribute( "Dex" );
		m_info.intel = gnome->attribute( "Int" );
		m_info.wis = gnome->attribute( "Wis" );
		m_info.cha = gnome->attribute( "Cha" );

		m_info.hunger = gnome->need( "Hunger" );
		m_info.thirst = gnome->need( "Thirst" );
		m_info.sleep = gnome->need( "Sleep" );
		m_info.happiness = gnome->need( "Happiness" );

		m_info.activity = gnome->getActivity();

		// Populate personality data (Milestone 2.0+)
		m_info.mood = gnome->mood();
		m_info.mentalBreak = ( gnome->mood() < 5 );

		// Backstory
		if ( !gnome->childhoodBackstory().isEmpty() )
		{
			auto row = DB::selectRow( "Backstories", gnome->childhoodBackstory() );
			m_info.childhoodTitle = row.value( "Title" ).toString();
			m_info.childhoodDesc = row.value( "Description" ).toString();
		}
		else
		{
			m_info.childhoodTitle.clear();
		}
		if ( !gnome->adulthoodBackstory().isEmpty() )
		{
			auto row = DB::selectRow( "Backstories", gnome->adulthoodBackstory() );
			m_info.adulthoodTitle = row.value( "Title" ).toString();
			m_info.adulthoodDesc = row.value( "Description" ).toString();
		}
		else
		{
			m_info.adulthoodTitle.clear();
		}

		// Traits
		m_info.traits.clear();
		for ( auto it = gnome->traits().constBegin(); it != gnome->traits().constEnd(); ++it )
		{
			GuiCreatureTraitInfo gti;
			gti.id = it.key();
			gti.value = it.value().toInt();
			auto traitRow = DB::selectRow( "Traits", gti.id );
			if ( gti.value < -25 )
			{
				gti.label = traitRow.value( "LowLabel" ).toString();
				gti.description = traitRow.value( "LowDesc" ).toString();
			}
			else if ( gti.value > 25 )
			{
				gti.label = traitRow.value( "HighLabel" ).toString();
				gti.description = traitRow.value( "HighDesc" ).toString();
			}
			m_info.traits.append( gti );
		}

		// Thoughts (sorted by absolute mood value, highest impact first)
		m_info.thoughts.clear();
		auto allThoughts = gnome->thoughts();
		std::sort( allThoughts.begin(), allThoughts.end(), []( const Thought& a, const Thought& b ) {
			return abs( a.moodValue ) > abs( b.moodValue );
		} );
		for ( const auto& t : allThoughts )
		{
			GuiCreatureThoughtInfo gti;
			gti.text = t.text;
			gti.cause = t.cause;
			gti.moodValue = t.moodValue;
			gti.ticksLeft = t.ticksLeft;
			gti.initialDuration = t.initialDuration;
			m_info.thoughts.append( gti );
		}

		// Social relationships
		m_info.relationships.clear();
		auto opinions = g->gm()->opinionsOf( gnome->id() );
		for ( auto it = opinions.constBegin(); it != opinions.constEnd(); ++it )
		{
			int op = it.value();
			if ( abs( op ) < 5 ) continue;
			GuiCreatureInfo::Relationship rel;
			Gnome* other = g->gm()->gnome( it.key() );
			rel.name = other ? other->name() : "Unknown";
			rel.opinion = op;
			rel.label = g->gm()->relationshipLabel( gnome->id(), it.key() );
			m_info.relationships.append( rel );
		}

		// Social memories (recent interactions)
		m_info.socialMemories.clear();
		auto memories = g->gm()->memoriesOf( gnome->id() );
		for ( const auto& mem : memories )
		{
			GuiCreatureInfo::MemoryEntry entry;
			Gnome* other = g->gm()->gnome( mem.otherID );
			QString otherName = other ? other->name() : "someone";
			entry.event = mem.event + " with " + otherName;
			entry.change = mem.opinionChange;
			entry.daysAgo = ( GameState::tick > mem.tick ) ? ( GameState::tick - mem.tick ) / 14400 : 0;
			m_info.socialMemories.append( entry );
		}

		// Mood breakdown
		gnome->moodBreakdown( m_info.baseMood, m_info.thoughtSum, m_info.needsPenalty );

		if( gnome->roleID() )
		{
			m_info.uniform = g->mil()->uniformCopy( gnome->roleID() );
		}
		m_info.equipment = gnome->equipment();
		
		if( m_previousID != m_currentID || gnome->equipmentChanged() )
		{
			m_previousID = m_currentID;

			m_info.itemPics.clear();
			if( m_info.equipment.head.itemID )
			{
				createUniformImg( "ArmorHead", m_info.uniform.parts.value( "HeadArmor" ), m_info.equipment.head );
			}
			if( m_info.equipment.chest.itemID )
			{
				createUniformImg( "ArmorChest", m_info.uniform.parts.value( "ChestArmor" ), m_info.equipment.chest );
			}
			if( m_info.equipment.arm.itemID )
			{
				createUniformImg( "ArmorArms", m_info.uniform.parts.value( "ArmArmor" ), m_info.equipment.arm );
			}
			if( m_info.equipment.hand.itemID )
			{
				createUniformImg( "ArmorHands", m_info.uniform.parts.value( "HandArmor" ), m_info.equipment.hand );
			}
			if( m_info.equipment.leg.itemID )
			{
				createUniformImg( "ArmorLegs", m_info.uniform.parts.value( "LegArmor" ), m_info.equipment.leg );
			}
			if( m_info.equipment.foot.itemID )
			{
				createUniformImg( "ArmorFeet", m_info.uniform.parts.value( "FootArmor" ), m_info.equipment.foot );
			}
			if( m_info.equipment.leftHandHeld.itemID )
			{
				createItemImg( "LeftHandHeld", m_info.equipment.leftHandHeld );
			}
			if( m_info.equipment.rightHandHeld.itemID )
			{
				createItemImg( "RightHandHeld", m_info.equipment.rightHandHeld );
			}
		}

		emit signalCreatureUpdate( m_info );
		return;
	}
	else
	{
		auto monster = g->cm()->monster( id );
		if( monster )
		{
			m_info.name = monster->name();
			m_info.id = id;
			//m_info.profession = monster->profession();

			m_info.str = monster->attribute( "Str" );
			m_info.con = monster->attribute( "Con" );
			m_info.dex = monster->attribute( "Dex" );
			m_info.intel = monster->attribute( "Int" );
			m_info.wis = monster->attribute( "Wis" );
			m_info.cha = monster->attribute( "Cha" );

			m_info.hunger = 100; //monster->need( "Hunger" );
			m_info.thirst = 100; //monster->need( "Thirst" );
			m_info.sleep = 100; //monster->need( "Sleep" );
			m_info.happiness = 100; //monster->need( "Happiness" );
			
			m_info.activity = "Doing something. tbi";
			emit signalCreatureUpdate( m_info );
			return;
		}
		else
		{
			auto animal = g->cm()->animal( id );
			if( animal )
			{
				m_info.name = animal->name();
				m_info.id = id;
				//m_info.profession = animal->profession();

				m_info.str = animal->attribute( "Str" );
				m_info.con = animal->attribute( "Con" );
				m_info.dex = animal->attribute( "Dex" );
				m_info.intel = animal->attribute( "Int" );
				m_info.wis = animal->attribute( "Wis" );
				m_info.cha = animal->attribute( "Cha" );

				m_info.hunger = animal->hunger();
				m_info.thirst = 100; //animal->need( "Thirst" );
				m_info.sleep = 100; //animal->need( "Sleep" );
				m_info.happiness = 100; //animal->need( "Happiness" );
			
				m_info.activity = "Doing something. tbi";
				emit signalCreatureUpdate( m_info );
				return;
			}
		}
		
	}
	m_currentID = 0;
}


void AggregatorCreatureInfo::onRequestProfessionList()
{
	if( !g ) return;
	emit signalProfessionList( g->gm()->professions() );
}

void AggregatorCreatureInfo::onSetProfession( unsigned int gnomeID, QString profession )
{
	if( !g ) return;
	auto gnome = g->gm()->gnome( gnomeID );
	if( gnome )
	{
		QString oldProf = gnome->profession();
		if( oldProf != profession )
		{
			gnome->selectProfession( profession );
			//onUpdateSingleGnome( gnomeID );
		}
	}
}

void AggregatorCreatureInfo::createItemImg( QString slot, EquipmentItem& eItem )
{
	if( !g ) return;
	if( eItem.itemID == 0 )
	{
		return;
	}
	QStringList mats;
	if( eItem.allMats.size() )
	{
		mats = eItem.allMats;
	}
	else
	{
		mats.append( eItem.material );
		mats.append( "Pine" );
	}

	auto sprite = g->sf()->createSprite( "UI" + eItem.item, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Global::util->createBufferForNoesisImage( pm, buffer );
		m_info.itemPics.insert( slot, buffer );
	}
	else
	{
		eItem.itemID = 0;
	}
}

void AggregatorCreatureInfo::createUniformImg( QString slot, const UniformItem& uItem, EquipmentItem& eItem )
{
	if( !g ) return;
	if( uItem.item.isEmpty() || eItem.itemID == 0 )
	{
		return; 
	}
	QStringList mats;
	mats.append( eItem.material );

	auto sprite = g->sf()->createSprite( "UI" + uItem.type + slot, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Global::util->createBufferForNoesisImage( pm, buffer );
		m_info.itemPics.insert( slot, buffer );
	}
	else
	{
		eItem.itemID = 0;
	}
}

void AggregatorCreatureInfo::createEmptyUniformImg( QString spriteID )
{
	if( !g ) return;
	QStringList mats; 
	mats.append( "any" );
	
	auto sprite = g->sf()->createSprite( spriteID, mats );
	if( sprite )
	{
		QPixmap pm = sprite->pixmap( "Spring", 0, 0 );

		std::vector<unsigned char> buffer;

		Global::util->createBufferForNoesisImage( pm, buffer );

		m_emptyPics.insert( spriteID, buffer );
	}
	else
	{
		std::vector<unsigned char> buffer;
		buffer.resize( 32 * 32 * 4, 0 );
		m_emptyPics.insert( spriteID, buffer );
	}
}

void AggregatorCreatureInfo::onRequestEmptySlotImages()
{
	if( !g ) return;
	m_emptyPics.clear();

	createEmptyUniformImg( "UIEmptySlotHead" );
	createEmptyUniformImg( "UIEmptySlotChest" );
	createEmptyUniformImg( "UIEmptySlotArms" );
	createEmptyUniformImg( "UIEmptySlotHands" );
	createEmptyUniformImg( "UIEmptySlotLegs" );
	createEmptyUniformImg( "UIEmptySlotFeet" );
	createEmptyUniformImg( "UIEmptySlotShield" );
	createEmptyUniformImg( "UIEmptySlotWeapon" );
	createEmptyUniformImg( "UIEmptySlotBack" );
	createEmptyUniformImg( "UIEmptySlotNeck" );
	createEmptyUniformImg( "UIEmptySlotRing" );
	createEmptyUniformImg( "UIEmptySlotRing" );

	emit signalEmptyPics( m_emptyPics );
}