#include "ui_tileinfo.h"
#include "ui_helpers.h"
#include "../imguibridge.h"
#include <imgui.h>

void drawTileInfo( ImGuiBridge& bridge )
{
	if ( !bridge.showTileInfo )
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 330, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 320, 500 ), ImGuiCond_FirstUseEver );

	ImGui::Begin( "Tile Info", &bridge.showTileInfo );

	const auto& ti = bridge.tileInfo;

	// =================================================================
	// TERRAIN section
	// =================================================================
	if ( ImGui::CollapsingHeader( "Terrain", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::Indent( 4.0f );

		if ( !ti.floor.isEmpty() )
		{
			ImGui::Text( "Floor: %s", ti.floor.toStdString().c_str() );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Remove##floor" ) )
			{
				bridge.cmdTerrainCommand( bridge.selectedTileID, "RemoveFloor" );
			}
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Replace##floor" ) )
			{
				bridge.cmdTerrainCommand( bridge.selectedTileID, "ReplaceFloor" );
			}
		}

		if ( !ti.wall.isEmpty() )
		{
			ImGui::Text( "Wall: %s", ti.wall.toStdString().c_str() );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Remove##wall" ) )
			{
				bridge.cmdTerrainCommand( bridge.selectedTileID, "Mine" );
			}
		}

		if ( !ti.embedded.isEmpty() )
			ImGui::Text( "Embedded: %s", ti.embedded.toStdString().c_str() );

		if ( !ti.plant.isEmpty() )
		{
			ImGui::Text( "Plant: %s", ti.plant.toStdString().c_str() );
			ImGui::SameLine();
			if ( ti.plantIsTree )
			{
				if ( ImGui::SmallButton( "Fell" ) )
					bridge.cmdTerrainCommand( bridge.selectedTileID, "FellTree" );
			}
			if ( ti.plantIsHarvestable )
			{
				ImGui::SameLine();
				if ( ImGui::SmallButton( "Harvest" ) )
					bridge.cmdTerrainCommand( bridge.selectedTileID, "HarvestTree" );
			}
		}

		if ( !ti.water.isEmpty() )
			ImGui::Text( "Water: %s", ti.water.toStdString().c_str() );

		if ( !ti.constructed.isEmpty() )
			ImGui::Text( "Built: %s", ti.constructed.toStdString().c_str() );

		// Designation
		if ( !ti.designationName.isEmpty() )
		{
			ImGui::Spacing();
			sectionLabel( ti.designationName.toStdString().c_str() );
			if ( ImGui::SmallButton( "Manage" ) )
			{
				bridge.cmdManageCommand( bridge.selectedTileID );
			}

			// Room type management
			if ( ti.designationFlag == TileFlag::TF_ROOM && ti.designationID != 0 )
			{
				static const char* roomTypeNames[] = { "Not Set", "Personal", "Dorm", "Dining", "Hospital", "Safe Room" };
				int currentType = (int)ti.roomType;
				if ( currentType < 0 || currentType > 5 ) currentType = 0;

				ImGui::Text( "Room Type:" );
				ImGui::SameLine();
				ImGui::PushItemWidth( 130 );
				if ( ImGui::Combo( "##RoomType", &currentType, roomTypeNames, 6 ) )
				{
					bridge.cmdSetRoomType( ti.designationID, (RoomType)currentType );
				}
				ImGui::PopItemWidth();

				if ( ti.roomType == RoomType::SafeRoom )
				{
					ImGui::TextColored( ImVec4( 0.4f, 0.9f, 0.4f, 1.0f ), "Gnomes retreat here during lockdown" );
				}

				if ( ti.isEnclosed )
					ImGui::Text( "Enclosed: Yes" );
				else
					ImGui::TextColored( ImVec4( 1.0f, 0.6f, 0.2f, 1.0f ), "Enclosed: No" );

				if ( ti.hasRoof )
					ImGui::Text( "Roofed: Yes" );
				else
					ImGui::TextColored( ImVec4( 1.0f, 0.6f, 0.2f, 1.0f ), "Roofed: No" );

				ImGui::Text( "Value: %d", ti.roomValue );
				if ( !ti.beds.isEmpty() )
					ImGui::Text( "Beds: %s", ti.beds.toStdString().c_str() );
			}
		}

		// Active job
		if ( !ti.jobName.isEmpty() )
		{
			ImGui::Spacing();
			sectionLabel( "Active Job" );
			ImGui::Text( "%s", ti.jobName.toStdString().c_str() );
			if ( !ti.jobWorker.isEmpty() )
				ImGui::Text( "Worker: %s", ti.jobWorker.toStdString().c_str() );
			ImGui::Text( "Priority: %s", ti.jobPriority.toStdString().c_str() );
		}

		ImGui::Unindent( 4.0f );
	}

	// =================================================================
	// ITEMS section
	// =================================================================
	if ( !ti.items.isEmpty() )
	{
		QString itemHeader = QString( "Items (%1)###Items" ).arg( ti.items.size() );
		if ( ImGui::CollapsingHeader( itemHeader.toStdString().c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui::Indent( 4.0f );

			for ( const auto& item : ti.items )
			{
				if ( item.isContainer )
				{
					ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.6f, 0.8f, 1.0f, 1.0f ) );
					QString containerText = item.text + " [" + QString::number( item.containerUsed ) + "/" + QString::number( item.containerCap ) + "]";
					bool open = ImGui::TreeNode( containerText.toStdString().c_str() );
					ImGui::PopStyleColor();
					if ( open )
					{
						if ( item.containedItemNames.isEmpty() )
						{
							ImGui::TextDisabled( "  Empty" );
						}
						else
						{
							for ( const auto& cName : item.containedItemNames )
							{
								ImGui::Text( "  %s", cName.toStdString().c_str() );
							}
						}
						ImGui::TreePop();
					}
				}
				else
				{
					ImGui::BulletText( "%s", item.text.toStdString().c_str() );
				}
			}

			ImGui::Unindent( 4.0f );
		}
	}

	// =================================================================
	// CREATURES section
	// =================================================================
	if ( !ti.creatures.isEmpty() )
	{
		QString creatureHeader = QString( "Creatures (%1)###Creatures" ).arg( ti.creatures.size() );
		if ( ImGui::CollapsingHeader( creatureHeader.toStdString().c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui::Indent( 4.0f );

			for ( const auto& creature : ti.creatures )
			{
				ImGui::Text( "%s", creature.text.toStdString().c_str() );
				ImGui::SameLine();
				ImGui::PushID( creature.id );
				if ( ImGui::SmallButton( "Info" ) )
				{
					bridge.cmdRequestCreatureUpdate( creature.id );
					bridge.activeSidePanel = ImGuiBridge::SidePanel::CreatureInfo;
				}
				ImGui::PopID();
			}

			ImGui::Unindent( 4.0f );
		}
	}

	ImGui::End();
}
