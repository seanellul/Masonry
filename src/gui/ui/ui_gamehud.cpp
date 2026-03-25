#include "ui_gamehud.h"
#include "../imguibridge.h"
#include "../../base/global.h"
#include "../../base/gamestate.h"

#include <imgui.h>

namespace
{

struct ToolbarButton
{
	const char* label;
	ButtonSelection selection;
};

const ToolbarButton toolbarButtons[] = {
	{ "Build", ButtonSelection::Build },
	{ "Mine", ButtonSelection::Mine },
	{ "Agriculture", ButtonSelection::Agriculture },
	{ "Designations", ButtonSelection::Designation },
	{ "Job", ButtonSelection::Job },
	{ "Magic", ButtonSelection::Magic },
};

// Build subcategories
struct BuildCategoryButton
{
	const char* label;
	BuildSelection selection;
};

const BuildCategoryButton buildCategories[] = {
	{ "Workshop", BuildSelection::Workshop },
	{ "Wall", BuildSelection::Wall },
	{ "Floor", BuildSelection::Floor },
	{ "Stairs", BuildSelection::Stairs },
	{ "Ramp", BuildSelection::Ramps },
	{ "Fence", BuildSelection::Fence },
	{ "Containers", BuildSelection::Containers },
	{ "Furniture", BuildSelection::Furniture },
	{ "Utility", BuildSelection::Utility },
};

// Subcategory definitions for build categories
struct BuildSubcategory
{
	const char* label;   // Display label
	const char* dbKey;   // Key passed to DB query
};

struct BuildCategorySubcats
{
	BuildSelection selection;
	const BuildSubcategory* subcats;
	int count;
};

const BuildSubcategory subcatsWallFloorStairsRamp[] = {
	{ "Wood", "Wood" }, { "Soil", "Soil" }, { "Stone", "Stone" }, { "Metal", "Metal" }, { "Other", "Other" }
};
const BuildSubcategory subcatsFence[] = {
	{ "Wood", "Wood" }, { "Stone", "Stone" }
};
const BuildSubcategory subcatsWorkshop[] = {
	{ "Wood", "Wood" }, { "Stone", "Stone" }, { "Metal", "Metal" }, { "Food", "Food" }, { "Crafts", "Craft" }, { "Mechanics", "Mechanics" }, { "Misc", "Misc" }
};
const BuildSubcategory subcatsFurniture[] = {
	{ "Chairs", "Chairs" }, { "Tables", "Tables" }, { "Beds", "Beds" }, { "Cabinets", "Cabinets" }, { "Misc", "Misc" }
};
const BuildSubcategory subcatsUtility[] = {
	{ "Doors", "Doors" }, { "Lights", "Lights" }, { "Farm", "Farm" }, { "Mechanism", "Mechanism" }, { "Hydraulics", "Hydraulics" }, { "Other", "Other" }
};

const BuildCategorySubcats allSubcats[] = {
	{ BuildSelection::Wall,       subcatsWallFloorStairsRamp, 5 },
	{ BuildSelection::Floor,      subcatsWallFloorStairsRamp, 5 },
	{ BuildSelection::Stairs,     subcatsWallFloorStairsRamp, 5 },
	{ BuildSelection::Ramps,      subcatsWallFloorStairsRamp, 5 },
	{ BuildSelection::Fence,      subcatsFence,               2 },
	{ BuildSelection::Workshop,   subcatsWorkshop,            7 },
	{ BuildSelection::Furniture,  subcatsFurniture,           5 },
	{ BuildSelection::Utility,    subcatsUtility,             6 },
};
const int allSubcatsCount = sizeof( allSubcats ) / sizeof( allSubcats[0] );

const BuildCategorySubcats* getSubcatsFor( BuildSelection sel )
{
	for ( int i = 0; i < allSubcatsCount; ++i )
	{
		if ( allSubcats[i].selection == sel )
			return &allSubcats[i];
	}
	return nullptr;
}

// Mine actions
const char* mineActions[] = { "Mine", "ExplorativeMine", "RemoveFloor", "DigStairsDown", "MineStairsUp", "DigRampDown", "DigHole" };
const char* mineLabels[] = { "Mine", "Explorative Mine", "Remove floor", "Stairs down", "Stairs up", "Ramp", "Hole" };

// Agriculture actions
const char* agriActions[] = { "FellTree", "", "HarvestTree", "Forage", "RemovePlant" };
const char* agriLabels[] = { "Cut tree", "Plant tree", "Harvest tree", "Forage", "Remove plant" };

// Designation zones
const char* designActions[] = { "CreateStockpile", "CreateFarm", "CreateGrove", "CreatePasture", "CreateRoom", "CreateDorm", "CreateDining", "CreateHospital", "CreateNoPass", "CreateGuardArea", "RemoveDesignation" };
const char* designLabels[] = { "Stockpile", "Farm", "Grove", "Pasture", "Personal Room", "Dormitory", "Dining Hall", "Hospital", "Forbidden", "Guard", "Remove" };

// Job actions
const char* jobActions[] = { "SuspendJob", "ResumeJob", "CancelJob", "LowerPrio", "RaisePrio" };
const char* jobLabels[] = { "Suspend", "Resume", "Cancel", "Lower priority", "Raise priority" };

// Track selected material index per build item (keyed by item ID)
static QMap<QString, QMap<int, int>> s_selectedMats;

void drawBuildItemList( ImGuiBridge& bridge, float subcatPanelRight )
{
	if ( bridge.buildItems.isEmpty() )
		return;

	ImGuiIO& io = ImGui::GetIO();
	float panelX = subcatPanelRight + 5;
	float panelY = 80;
	float panelW = 420;
	float panelH = io.DisplaySize.y - panelY - 70;

	ImGui::SetNextWindowPos( ImVec2( panelX, panelY ) );
	ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
	ImGui::Begin( "##builditems", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

	for ( const auto& item : bridge.buildItems )
	{
		ImGui::PushID( item.id.toStdString().c_str() );

		ImGui::Separator();
		ImGui::Text( "%s", item.name.toStdString().c_str() );

		// Material dropdowns per required item
		QStringList mats;
		bool canBuild = true;

		for ( int r = 0; r < item.requiredItems.size(); ++r )
		{
			const auto& req = item.requiredItems[r];
			if ( req.availableMats.isEmpty() )
			{
				canBuild = false;
				ImGui::TextDisabled( "  %d x %s (unavailable)", req.amount, req.itemID.toStdString().c_str() );
				mats.append( "" );
				continue;
			}

			// Get or init selected index
			int& selIdx = s_selectedMats[item.id][r];
			if ( selIdx >= req.availableMats.size() ) selIdx = 0;

			ImGui::Text( "  %d", req.amount );
			ImGui::SameLine();

			// Material combo
			QString comboLabel = "##mat" + QString::number( r );
			QString preview = req.availableMats[selIdx].first + " (" + QString::number( req.availableMats[selIdx].second ) + ")";

			ImGui::SetNextItemWidth( 180 );
			if ( ImGui::BeginCombo( comboLabel.toStdString().c_str(), preview.toStdString().c_str() ) )
			{
				for ( int m = 0; m < req.availableMats.size(); ++m )
				{
					QString label = req.availableMats[m].first + " (" + QString::number( req.availableMats[m].second ) + ")";
					if ( ImGui::Selectable( label.toStdString().c_str(), m == selIdx ) )
					{
						selIdx = m;
					}
				}
				ImGui::EndCombo();
			}

			mats.append( req.availableMats[selIdx].first );
		}

		// Action buttons: Fill Hole, Replace, Build
		if ( item.biType == BuildItemType::Terrain )
		{
			bool isWall = item.id.endsWith( "Wall" ) || item.id.startsWith( "FancyWall" );
			bool isFloor = item.id.endsWith( "Floor" ) || item.id.startsWith( "FancyFloor" );

			if ( isWall )
			{
				ImGui::SameLine();
				if ( !canBuild ) ImGui::BeginDisabled();
				if ( ImGui::Button( "Fill Hole" ) )
				{
					bridge.cmdBuild( item.biType, "FillHole", item.id, mats );
				}
				if ( !canBuild ) ImGui::EndDisabled();
			}
			if ( isWall || isFloor )
			{
				ImGui::SameLine();
				if ( !canBuild ) ImGui::BeginDisabled();
				if ( ImGui::Button( "Replace" ) )
				{
					bridge.cmdBuild( item.biType, "Replace", item.id, mats );
				}
				if ( !canBuild ) ImGui::EndDisabled();
			}
		}

		ImGui::SameLine();
		if ( !canBuild ) ImGui::BeginDisabled();
		if ( ImGui::Button( "Build" ) )
		{
			bridge.cmdBuild( item.biType, "", item.id, mats );
		}
		if ( !canBuild ) ImGui::EndDisabled();

		ImGui::PopID();
	}

	ImGui::End();
}

} // namespace

void drawGameHUD( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();

	// =========================================================================
	// Top-left: Kingdom info + render toggles
	// =========================================================================
	ImGui::SetNextWindowPos( ImVec2( 5, 5 ) );
	ImGui::SetNextWindowSize( ImVec2( 250, 0 ) );
	ImGui::Begin( "##kingdom", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground );

	ImGui::Text( "%s", bridge.kingdomName.toStdString().c_str() );
	ImGui::Text( "%s", bridge.kingdomInfo1.toStdString().c_str() );
	ImGui::Text( "%s", bridge.kingdomInfo2.toStdString().c_str() );
	ImGui::Text( "%s", bridge.kingdomInfo3.toStdString().c_str() );
	ImGui::Text( "Level: %d", bridge.viewLevel );

	// DJWA toggles
	bool d = bridge.renderDesignations, j = bridge.renderJobs, w = bridge.renderWalls, a = bridge.renderAxles;
	bool changed = false;
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
	if ( ImGui::Checkbox( "D", &d ) ) { bridge.renderDesignations = d; changed = true; }
	ImGui::SameLine();
	if ( ImGui::Checkbox( "J", &j ) ) { bridge.renderJobs = j; changed = true; }
	ImGui::SameLine();
	if ( ImGui::Checkbox( "W", &w ) ) { bridge.renderWalls = w; changed = true; }
	ImGui::SameLine();
	if ( ImGui::Checkbox( "A", &a ) ) { bridge.renderAxles = a; changed = true; }
	ImGui::PopStyleVar();

	if ( changed )
	{
		bridge.cmdSetRenderOptions( d, j, w, a );
	}

	ImGui::End();

	// =========================================================================
	// Top-right: Time + speed controls
	// =========================================================================
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 200, 5 ) );
	ImGui::SetNextWindowSize( ImVec2( 195, 0 ) );
	ImGui::Begin( "##time", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground );

	ImGui::Text( "Day %d of %s", bridge.day, bridge.season.toStdString().c_str() );
	ImGui::Text( "Year %d   %02d:%02d", bridge.year, bridge.hour, bridge.minute );

	// Speed controls
	if ( ImGui::SmallButton( bridge.paused ? ">" : "||" ) )
	{
		bridge.cmdSetPaused( !bridge.paused );
	}
	ImGui::SameLine();
	bool isNormal = ( bridge.gameSpeed == GameSpeed::Normal && !bridge.paused );
	bool isFast = ( bridge.gameSpeed == GameSpeed::Fast && !bridge.paused );
	if ( isNormal ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.6f, 0.2f, 1.0f ) );
	if ( ImGui::SmallButton( ">>" ) )
	{
		bridge.cmdSetPaused( false );
		bridge.cmdSetGameSpeed( GameSpeed::Normal );
	}
	if ( isNormal ) ImGui::PopStyleColor();
	ImGui::SameLine();
	if ( isFast ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.6f, 0.2f, 1.0f ) );
	if ( ImGui::SmallButton( ">>>" ) )
	{
		bridge.cmdSetPaused( false );
		bridge.cmdSetGameSpeed( GameSpeed::Fast );
	}
	if ( isFast ) ImGui::PopStyleColor();

	ImGui::End();

	// =========================================================================
	// Bottom toolbar
	// =========================================================================
	float toolbarHeight = 50;
	ImGui::SetNextWindowPos( ImVec2( 0, io.DisplaySize.y - toolbarHeight ) );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x, toolbarHeight ) );
	ImGui::Begin( "##toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

	float leftWidth = io.DisplaySize.x * 0.5f;
	float buttonW = leftWidth / 6.0f - 5.0f;

	// Left side: action buttons
	for ( int i = 0; i < 6; ++i )
	{
		if ( i > 0 ) ImGui::SameLine();
		bool active = ( bridge.currentToolbar == toolbarButtons[i].selection );
		bool isMagic = ( toolbarButtons[i].selection == ButtonSelection::Magic );
		if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );
		if ( isMagic ) ImGui::BeginDisabled();
		if ( ImGui::Button( toolbarButtons[i].label, ImVec2( buttonW, 30 ) ) )
		{
			if ( active )
			{
				bridge.currentToolbar = ButtonSelection::None;
				bridge.currentBuildCategory = BuildSelection::None;
				bridge.buildItems.clear();
			}
			else
			{
				bridge.currentToolbar = toolbarButtons[i].selection;
				bridge.currentBuildCategory = BuildSelection::None;
				bridge.buildItems.clear();
			}
		}
		if ( active ) ImGui::PopStyleColor();
		if ( isMagic ) ImGui::EndDisabled();
	}

	// Right side: management panels
	float rightStart = io.DisplaySize.x * 0.5f + 10;
	float rightButtonW = ( io.DisplaySize.x - rightStart - 10 ) / 5.0f - 5.0f;
	ImGui::SameLine( rightStart );
	if ( ImGui::Button( "Kingdom", ImVec2( rightButtonW, 30 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Kingdom ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Kingdom;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Kingdom )
			bridge.cmdRequestInventoryCategories();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Stockpiles", ImVec2( rightButtonW, 30 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Stockpile ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Stockpile;
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Military", ImVec2( rightButtonW, 30 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Military ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Military;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Military )
			bridge.cmdRequestMilitaryUpdate();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Population", ImVec2( rightButtonW, 30 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Population ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Population;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Population )
			bridge.cmdRequestPopulationUpdate();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Missions", ImVec2( rightButtonW, 30 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Missions ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Missions;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Missions )
			bridge.cmdRequestNeighborsUpdate();
	}

	ImGui::End();

	// =========================================================================
	// Toolbar expansion panels (above the bottom toolbar)
	// =========================================================================
	if ( bridge.currentToolbar == ButtonSelection::Build )
	{
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - 340 ) );
		ImGui::SetNextWindowSize( ImVec2( 120, 330 ) );
		ImGui::Begin( "##buildcats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

		for ( int i = 0; i < 9; ++i )
		{
			bool active = ( bridge.currentBuildCategory == buildCategories[i].selection );
			if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );
			if ( ImGui::Button( buildCategories[i].label, ImVec2( 100, 25 ) ) )
			{
				bridge.currentBuildCategory = buildCategories[i].selection;
				bridge.currentBuildMaterial.clear();
				bridge.buildItems.clear();
				s_selectedMats.clear();

				// Check if this category has subcategories
				const BuildCategorySubcats* sc = getSubcatsFor( buildCategories[i].selection );
				if ( sc && sc->count > 0 )
				{
					// Auto-select first subcategory and request items
					bridge.currentBuildMaterial = sc->subcats[0].dbKey;
					bridge.cmdRequestBuildItems( buildCategories[i].selection, bridge.currentBuildMaterial );
				}
				else
				{
					// No subcategories (e.g. Containers) — request items directly
					bridge.cmdRequestBuildItems( buildCategories[i].selection, "" );
				}
			}
			if ( active ) ImGui::PopStyleColor();
		}

		if ( ImGui::Button( "Deconstruct", ImVec2( 100, 25 ) ) )
		{
			bridge.currentBuildCategory = BuildSelection::None;
			bridge.buildItems.clear();
			bridge.cmdSetSelectionAction( "Deconstruct" );
		}

		ImGui::End();

		// Subcategory buttons (materials, workshop tabs, furniture groups, etc.)
		float subcatPanelRight = 130; // default if no subcats
		const BuildCategorySubcats* sc = getSubcatsFor( bridge.currentBuildCategory );
		if ( sc && sc->count > 0 )
		{
			float subcatH = sc->count * 32.0f + 10.0f;
			ImGui::SetNextWindowPos( ImVec2( 130, io.DisplaySize.y - toolbarHeight - subcatH - 10 ) );
			ImGui::SetNextWindowSize( ImVec2( 100, subcatH ) );
			ImGui::Begin( "##buildsubcats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

			for ( int m = 0; m < sc->count; ++m )
			{
				bool matActive = ( bridge.currentBuildMaterial == sc->subcats[m].dbKey );
				if ( matActive ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );
				if ( ImGui::Button( sc->subcats[m].label, ImVec2( 85, 25 ) ) )
				{
					bridge.currentBuildMaterial = sc->subcats[m].dbKey;
					bridge.buildItems.clear();
					s_selectedMats.clear();
					bridge.cmdRequestBuildItems( bridge.currentBuildCategory, bridge.currentBuildMaterial );
				}
				if ( matActive ) ImGui::PopStyleColor();
			}

			ImGui::End();
			subcatPanelRight = 235;
		}

		// Build items list
		drawBuildItemList( bridge, subcatPanelRight );
	}
	else if ( bridge.currentToolbar == ButtonSelection::Mine )
	{
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - 200 ) );
		ImGui::SetNextWindowSize( ImVec2( 160, 190 ) );
		ImGui::Begin( "##mineactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		for ( int i = 0; i < 7; ++i )
		{
			if ( ImGui::Button( mineLabels[i], ImVec2( 140, 22 ) ) )
			{
				bridge.cmdSetSelectionAction( mineActions[i] );
			}
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Agriculture )
	{
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - 140 ) );
		ImGui::SetNextWindowSize( ImVec2( 140, 130 ) );
		ImGui::Begin( "##agriactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		for ( int i = 0; i < 5; ++i )
		{
			bool hasAction = strlen( agriActions[i] ) > 0;
			if ( !hasAction ) ImGui::BeginDisabled();
			if ( ImGui::Button( agriLabels[i], ImVec2( 120, 22 ) ) )
			{
				if ( hasAction )
					bridge.cmdSetSelectionAction( agriActions[i] );
			}
			if ( !hasAction ) ImGui::EndDisabled();
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Designation )
	{
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - 290 ) );
		ImGui::SetNextWindowSize( ImVec2( 140, 280 ) );
		ImGui::Begin( "##designactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		for ( int i = 0; i < 11; ++i )
		{
			if ( ImGui::Button( designLabels[i], ImVec2( 120, 22 ) ) )
			{
				bridge.cmdSetSelectionAction( designActions[i] );
			}
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Job )
	{
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - 140 ) );
		ImGui::SetNextWindowSize( ImVec2( 140, 130 ) );
		ImGui::Begin( "##jobactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		for ( int i = 0; i < 5; ++i )
		{
			if ( ImGui::Button( jobLabels[i], ImVec2( 120, 22 ) ) )
			{
				bridge.cmdSetSelectionAction( jobActions[i] );
			}
		}

		ImGui::End();
	}

	// =========================================================================
	// Event popups
	// =========================================================================
	if ( !bridge.pendingEvents.isEmpty() )
	{
		auto& evt = bridge.pendingEvents.first();
		ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
		ImGui::SetNextWindowSize( ImVec2( 400, 200 ) );
		ImGui::Begin( evt.title.toStdString().c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

		ImGui::TextWrapped( "%s", evt.msg.toStdString().c_str() );
		ImGui::Spacing();

		if ( evt.yesno )
		{
			if ( ImGui::Button( "Yes", ImVec2( 80, 30 ) ) )
			{
				bridge.cmdEventAnswer( evt.id, true );
				bridge.pendingEvents.removeFirst();
			}
			ImGui::SameLine();
			if ( ImGui::Button( "No", ImVec2( 80, 30 ) ) )
			{
				bridge.cmdEventAnswer( evt.id, false );
				bridge.pendingEvents.removeFirst();
			}
		}
		else
		{
			if ( ImGui::Button( "OK", ImVec2( 80, 30 ) ) )
			{
				bridge.cmdEventAnswer( evt.id, true );
				bridge.pendingEvents.removeFirst();
			}
		}

		ImGui::End();
	}
}
