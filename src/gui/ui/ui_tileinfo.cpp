#include "ui_tileinfo.h"
#include "ui_helpers.h"
#include "../imguibridge.h"
#include "../IconsFontAwesome6.h"
#include "../IconsRpgAwesome.h"
#include <imgui.h>

// Compact action button — icon-only (no rectangular frame) with tooltip.
// T-0005: was rendering with SmallButton's default frame, which produced a
// distracting pill behind every icon. The button is still clickable and
// hovers; only the resting background is transparent.
static bool actionButton( const char* icon, const char* tooltip, const char* idSuffix )
{
	ImGui::PushID( idSuffix );
	ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 1, 1, 1, 0.12f ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 1, 1, 1, 0.20f ) );
	bool clicked = ImGui::SmallButton( icon );
	ImGui::PopStyleColor( 3 );
	ImGui::PopID();
	if ( ImGui::IsItemHovered() )
		ImGui::SetTooltip( "%s", tooltip );
	return clicked;
}

// Draw a terrain row: label text on left, action buttons on right
static void terrainRow( const char* text, float actionsWidth )
{
	ImGui::Text( "%s", text );
	ImGui::SameLine( ImGui::GetContentRegionAvail().x - actionsWidth + ImGui::GetCursorPosX() );
}

void drawTileInfo( ImGuiBridge& bridge )
{
	if ( !bridge.showTileInfo )
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 330, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 320, 500 ), ImGuiCond_FirstUseEver );

	ImGui::Begin( "Tile Info", &bridge.showTileInfo );

	const auto& ti = bridge.tileInfo;
	float btnSize = ImGui::GetFrameHeight(); // square button size

	// =================================================================
	// TERRAIN section
	// =================================================================
	if ( ImGui::CollapsingHeader( "Terrain", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		// Use a table for clean alignment: [info] [actions]
		if ( ImGui::BeginTable( "##terrain", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX ) )
		{
			ImGui::TableSetupColumn( "info", 0, 1.0f );
			ImGui::TableSetupColumn( "acts", ImGuiTableColumnFlags_WidthFixed, 80.0f );

			if ( !ti.floor.isEmpty() )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextWrapped( "%s", ti.floor.toStdString().c_str() );
				ImGui::TableNextColumn();
				if ( actionButton( ICON_FA_TRASH, "Remove floor", "rmfloor" ) )
					bridge.cmdTerrainCommand( bridge.selectedTileID, "RemoveFloor" );
				ImGui::SameLine();
				if ( actionButton( ICON_FA_RIGHT_LEFT, "Replace floor", "rpfloor" ) )
					bridge.cmdTerrainCommand( bridge.selectedTileID, "ReplaceFloor" );
			}

			if ( !ti.wall.isEmpty() )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextWrapped( "%s", ti.wall.toStdString().c_str() );
				ImGui::TableNextColumn();
				if ( actionButton( ICON_FA_HAMMER, "Mine / Remove wall", "rmwall" ) )
					bridge.cmdTerrainCommand( bridge.selectedTileID, "Mine" );
			}

			if ( !ti.embedded.isEmpty() )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextWrapped( "%s", ti.embedded.toStdString().c_str() );
				ImGui::TableNextColumn();
			}

			if ( !ti.plant.isEmpty() )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextWrapped( "%s", ti.plant.toStdString().c_str() );
				ImGui::TableNextColumn();
				if ( ti.plantIsTree )
				{
					if ( actionButton( ICON_RA_AXE, "Fell tree", "fell" ) )
						bridge.cmdTerrainCommand( bridge.selectedTileID, "FellTree" );
					ImGui::SameLine();
				}
				if ( ti.plantIsHarvestable )
				{
					if ( actionButton( ICON_FA_SEEDLING, "Harvest", "harvest" ) )
						bridge.cmdTerrainCommand( bridge.selectedTileID, "HarvestTree" );
				}
			}

			if ( !ti.water.isEmpty() )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextWrapped( "%s", ti.water.toStdString().c_str() );
				ImGui::TableNextColumn();
			}

			if ( !ti.constructed.isEmpty() )
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextWrapped( "%s", ti.constructed.toStdString().c_str() );
				ImGui::TableNextColumn();
			}

			ImGui::EndTable();
		}

		// Designation
		if ( !ti.designationName.isEmpty() )
		{
			ImGui::Spacing();
			sectionLabel( ti.designationName.toStdString().c_str() );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Manage" ) )
			{
				bridge.cmdManageCommand( bridge.selectedTileID );
			}

			if ( ti.designationFlag == TileFlag::TF_ROOM && ti.designationID != 0 )
			{
				static const char* roomTypeNames[] = { "Not Set", "Personal", "Dorm", "Dining", "Hospital", "Safe Room" };
				int currentType = (int)ti.roomType;
				if ( currentType < 0 || currentType > 5 ) currentType = 0;

				ImGui::SetNextItemWidth( 140 );
				if ( ImGui::Combo( "##RoomType", &currentType, roomTypeNames, 6 ) )
				{
					bridge.cmdSetRoomType( ti.designationID, (RoomType)currentType );
				}

				if ( ti.roomType == RoomType::SafeRoom )
					ImGui::TextColored( ImVec4( 0.4f, 0.9f, 0.4f, 1.0f ), "Gnomes retreat here during lockdown" );

				// Compact room status line
				ImVec4 yesCol( 0.5f, 0.8f, 0.5f, 1.0f );
				ImVec4 noCol( 1.0f, 0.6f, 0.2f, 1.0f );
				ImGui::TextColored( ti.isEnclosed ? yesCol : noCol, "Enclosed: %s", ti.isEnclosed ? "Yes" : "No" );
				ImGui::SameLine( 0, 16 );
				ImGui::TextColored( ti.hasRoof ? yesCol : noCol, "Roofed: %s", ti.hasRoof ? "Yes" : "No" );

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
			{
				ImGui::SameLine( 0, 10 );
				ImGui::TextDisabled( "(%s)", ti.jobWorker.toStdString().c_str() );
			}

			// T-0023: required materials + status. Surfaces *why* a
			// queued construction is sitting unbuilt — the recurring
			// "I built a Distillery and nothing happened" failure
			// mode where the player has no idea barrels are missing.
			if ( !ti.requiredItems.isEmpty() )
			{
				bool anyMissing = false;
				for ( const auto& req : ti.requiredItems )
				{
					if ( !req.available ) { anyMissing = true; break; }
				}

				if ( anyMissing && ti.jobWorker.isEmpty() )
				{
					ImGui::TextColored( ImVec4( 1.0f, 0.85f, 0.3f, 1.0f ),
						ICON_FA_TRIANGLE_EXCLAMATION " Waiting for materials" );
				}

				ImGui::Indent( 8.0f );
				for ( const auto& req : ti.requiredItems )
				{
					ImVec4 col = req.available
						? ImVec4( 0.5f, 0.85f, 0.5f, 1.0f )    // green check
						: ImVec4( 1.0f, 0.55f, 0.45f, 1.0f );  // red warning
					const char* glyph = req.available ? "[OK]" : "[ -]";
					QString matLabel = ( req.material.isEmpty() || req.material == "any" )
						? QString() : ( req.material + " " );
					ImGui::TextColored( col, "%s %d x %s%s", glyph,
						req.count, matLabel.toStdString().c_str(),
						req.text.toStdString().c_str() );
				}
				ImGui::Unindent( 8.0f );
			}

			// Cancel button — works for any queued job, not just construction
			if ( bridge.selectedTileID != 0 )
			{
				ImGui::Spacing();
				if ( ImGui::SmallButton( ICON_FA_XMARK " Cancel job" ) )
				{
					bridge.cmdTerrainCommand( bridge.selectedTileID, "CancelJob" );
				}
				if ( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "Cancel this queued job and free the tile." );
			}
		}
	}

	// =================================================================
	// ITEMS section
	// =================================================================
	if ( !ti.items.isEmpty() )
	{
		QString itemHeader = QString( "Items (%1)###Items" ).arg( ti.items.size() );
		if ( ImGui::CollapsingHeader( itemHeader.toStdString().c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
		{
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
							ImGui::TextDisabled( "Empty" );
						}
						else
						{
							for ( const auto& cName : item.containedItemNames )
							{
								ImGui::BulletText( "%s", cName.toStdString().c_str() );
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
			for ( const auto& creature : ti.creatures )
			{
				ImGui::Text( "%s", creature.text.toStdString().c_str() );
				ImGui::SameLine();
				ImGui::PushID( creature.id );
				// T-0005: icon-only info button (no button frame).
				ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
				ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 1, 1, 1, 0.12f ) );
				ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 1, 1, 1, 0.20f ) );
				if ( ImGui::SmallButton( ICON_FA_CIRCLE_INFO ) )
				{
					bridge.cmdRequestCreatureUpdate( creature.id );
					bridge.activeSidePanel = ImGuiBridge::SidePanel::CreatureInfo;
				}
				ImGui::PopStyleColor( 3 );
				if ( ImGui::IsItemHovered() )
					ImGui::SetTooltip( "View creature details" );
				ImGui::PopID();
			}
		}
	}

	ImGui::End();
}
