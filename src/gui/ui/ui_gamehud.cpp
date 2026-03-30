#include "ui_gamehud.h"
#include "../imguibridge.h"
#include "../../base/global.h"
#include "../../base/gamestate.h"
#include "../../base/logger.h"
#include "../strings.h"
#include "../spritetexturecache.h"

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
};
const int toolbarButtonCount = 5;

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
const char* mineLabels[] = { "Mine", "Mine Vein", "Remove floor", "Stairs down", "Stairs up", "Ramp", "Hole" };

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
	float panelW = 450;
	float panelH = io.DisplaySize.y - panelY - 70;

	ImGui::SetNextWindowPos( ImVec2( panelX, panelY ) );
	ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
	ImGui::Begin( "##builditems", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

	// Lazy-init sprite texture cache
	if ( !bridge.spriteTexCache )
	{
		bridge.spriteTexCache = new SpriteTextureCache();
	}

	float iconSize = 64.0f;

	for ( const auto& item : bridge.buildItems )
	{
		ImGui::PushID( item.id.toStdString().c_str() );

		ImGui::Separator();

		// Get sprite texture from pre-generated buffer
		ImTextureID texID = (ImTextureID)0;
		if ( !item.buffer.empty() && item.iconWidth > 0 && item.iconHeight > 0 )
		{
			unsigned int cacheKey = qHash( item.id ) + 100000;
			texID = bridge.spriteTexCache->getTextureFromBuffer( cacheKey, item.buffer.data(), item.iconWidth, item.iconHeight );
		}

		// Layout: square icon on left, title + dropdowns + buttons on right
		ImVec2 startPos = ImGui::GetCursorPos();

		// Draw icon (square)
		if ( texID )
		{
			ImGui::Image( texID, ImVec2( iconSize, iconSize ) );
		}
		else
		{
			// Placeholder box if no icon
			ImGui::Dummy( ImVec2( iconSize, iconSize ) );
		}

		// Position right column next to icon
		float rightX = startPos.x + iconSize + 8;
		float rightW = ImGui::GetContentRegionAvail().x - iconSize - 8;

		// Title — aligned with top of icon
		ImGui::SetCursorPos( ImVec2( rightX, startPos.y ) );
		ImGui::Text( "%s", item.name.toStdString().c_str() );

		// Material dropdowns — below title, aligned with icon
		QStringList mats;
		bool canBuild = true;
		float dropdownW = qMax( 120.0f, rightW - 80 );

		for ( int r = 0; r < item.requiredItems.size(); ++r )
		{
			const auto& req = item.requiredItems[r];
			ImGui::SetCursorPosX( rightX );

			if ( req.availableMats.isEmpty() )
			{
				canBuild = false;
				QString unavailName = S::s( "$ItemName_" + req.itemID );
				ImGui::TextDisabled( "%d x %s (unavailable)", req.amount, unavailName.toStdString().c_str() );
				mats.append( "" );
				continue;
			}

			int& selIdx = s_selectedMats[item.id][r];
			if ( selIdx >= req.availableMats.size() ) selIdx = 0;

			// Compact: "N  item_name  [dropdown]"
			QString itemName = S::s( "$ItemName_" + req.itemID );
			ImGui::Text( "%d", req.amount );
			ImGui::SameLine();
			ImGui::SetNextItemWidth( dropdownW );

			QString comboLabel = "##mat" + QString::number( r );
			QString matName = S::s( "$MaterialName_" + req.availableMats[selIdx].first );
			QString preview = matName + " " + itemName + " (" + QString::number( req.availableMats[selIdx].second ) + ")";

			if ( ImGui::BeginCombo( comboLabel.toStdString().c_str(), preview.toStdString().c_str() ) )
			{
				for ( int m = 0; m < req.availableMats.size(); ++m )
				{
					QString mLabel = S::s( "$MaterialName_" + req.availableMats[m].first );
					QString label = mLabel + " " + itemName + " (" + QString::number( req.availableMats[m].second ) + ")";
					if ( ImGui::Selectable( label.toStdString().c_str(), m == selIdx ) )
					{
						selIdx = m;
					}
				}
				ImGui::EndCombo();
			}

			mats.append( req.availableMats[selIdx].first );
		}

		// Buttons — right-aligned below dropdowns
		ImGui::SetCursorPosX( rightX );
		if ( !canBuild ) ImGui::BeginDisabled();
		if ( ImGui::SmallButton( "Build" ) )
		{
			bridge.cmdBuild( item.biType, "", item.id, mats );
		}
		if ( !canBuild ) ImGui::EndDisabled();

		if ( item.biType == BuildItemType::Terrain )
		{
			bool isWall = item.id.endsWith( "Wall" ) || item.id.startsWith( "FancyWall" );
			bool isFloor = item.id.endsWith( "Floor" ) || item.id.startsWith( "FancyFloor" );

			if ( isWall )
			{
				ImGui::SameLine();
				if ( !canBuild ) ImGui::BeginDisabled();
				if ( ImGui::SmallButton( "Fill Hole" ) ) { bridge.cmdBuild( item.biType, "FillHole", item.id, mats ); }
				if ( !canBuild ) ImGui::EndDisabled();
			}
			if ( isWall || isFloor )
			{
				ImGui::SameLine();
				if ( !canBuild ) ImGui::BeginDisabled();
				if ( ImGui::SmallButton( "Replace" ) ) { bridge.cmdBuild( item.biType, "Replace", item.id, mats ); }
				if ( !canBuild ) ImGui::EndDisabled();
			}
		}

		// Ensure cursor is past the icon height
		float endY = startPos.y + iconSize + ImGui::GetStyle().ItemSpacing.y;
		if ( ImGui::GetCursorPosY() < endY )
		{
			ImGui::SetCursorPosY( endY );
		}
		ImGui::Dummy( ImVec2( 0, 0 ) ); // satisfy ImGui bounds check

		ImGui::PopID();
	}

	ImGui::End();
}

} // namespace

void drawGameHUD( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();

	// =========================================================================
	// Top-left: Kingdom info + resources (compact, 2 lines)
	// =========================================================================
	float topBarHeight = 48;
	ImGui::SetNextWindowPos( ImVec2( 0, 0 ) );
	ImGui::SetNextWindowSize( ImVec2( 420, topBarHeight ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10, 4 ) );
	ImGui::Begin( "##topleft", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

	// Line 1: Kingdom name
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.9f, 0.85f, 0.6f, 1.0f ) );
	ImGui::Text( "%s", bridge.kingdomName.toStdString().c_str() );
	ImGui::PopStyleColor();

	// Line 2: Resources
	auto colorForCount = []( int count ) -> ImVec4 {
		if ( count > 50 ) return ImVec4( 0.3f, 0.8f, 0.3f, 1.0f );
		if ( count > 10 ) return ImVec4( 0.9f, 0.8f, 0.2f, 1.0f );
		return ImVec4( 0.9f, 0.2f, 0.2f, 1.0f );
	};

	ImGui::PushStyleColor( ImGuiCol_Text, colorForCount( bridge.stockFood ) );
	ImGui::Text( "Food: %d", bridge.stockFood );
	ImGui::PopStyleColor();
	ImGui::SameLine( 0, 10 );

	ImGui::PushStyleColor( ImGuiCol_Text, colorForCount( bridge.stockDrink ) );
	ImGui::Text( "Drink: %d", bridge.stockDrink );
	ImGui::PopStyleColor();
	ImGui::SameLine( 0, 10 );

	ImGui::Text( "Gnomes: %d  Items: %d", bridge.stockGnomes, bridge.stockItems );

	ImGui::End();
	ImGui::PopStyleVar();

	// =========================================================================
	// Top-right: Time, Z-level, speed controls (compact, 2 lines)
	// =========================================================================
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 290, 0 ) );
	ImGui::SetNextWindowSize( ImVec2( 290, 0 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10, 4 ) );
	ImGui::Begin( "##topright", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar );

	// Line 1: Date + Z-level
	ImGui::Text( "Day %d %s  Y%d", bridge.day, bridge.season.toStdString().c_str(), bridge.year );
	ImGui::SameLine( 0, 15 );
	ImGui::TextDisabled( "Z:%d", bridge.viewLevel );

	// Line 2: Time + speed controls + lockdown
	ImGui::Text( "%02d:%02d", bridge.hour, bridge.minute );
	ImGui::SameLine( 0, 10 );

	if ( ImGui::SmallButton( bridge.paused ? ">" : "||" ) )
	{
		bridge.cmdSetPaused( !bridge.paused );
	}
	ImGui::SameLine();
	bool isNormal = ( bridge.gameSpeed == GameSpeed::Normal && !bridge.paused );
	bool isFast = ( bridge.gameSpeed == GameSpeed::Fast && !bridge.paused );
	if ( isNormal ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.6f, 0.2f, 1.0f ) );
	if ( ImGui::SmallButton( ">>" ) ) { bridge.cmdSetPaused( false ); bridge.cmdSetGameSpeed( GameSpeed::Normal ); }
	if ( isNormal ) ImGui::PopStyleColor();
	ImGui::SameLine();
	if ( isFast ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.6f, 0.2f, 1.0f ) );
	if ( ImGui::SmallButton( ">>>" ) ) { bridge.cmdSetPaused( false ); bridge.cmdSetGameSpeed( GameSpeed::Fast ); }
	if ( isFast ) ImGui::PopStyleColor();
	ImGui::SameLine( 0, 10 );

	// Lockdown button
	if ( GameState::lockdown )
	{
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.8f, 0.1f, 0.1f, 1.0f ) );
		if ( ImGui::SmallButton( "L" ) ) { GameState::lockdown = false; Global::logger().log( LogType::INFO, "Lockdown lifted.", 0 ); }
		ImGui::PopStyleColor();
		if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "LOCKDOWN ACTIVE\nClick to lift" );
	}
	else
	{
		if ( ImGui::SmallButton( "L" ) ) { GameState::lockdown = true; Global::logger().log( LogType::DANGER, "LOCKDOWN!", 0 ); }
		if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Lockdown\nConfine civilians to safe rooms" );
	}

	// Line 3: DJWA overlay toggles as small highlighted buttons
	ImVec4 onColor  = ImVec4( 0.2f, 0.5f, 0.7f, 1.0f );
	ImVec4 offColor = ImVec4( 0.3f, 0.3f, 0.3f, 1.0f );
	bool changed = false;

	bool d = bridge.renderDesignations;
	ImGui::PushStyleColor( ImGuiCol_Button, d ? onColor : offColor );
	if ( ImGui::SmallButton( "D" ) ) { d = !d; bridge.renderDesignations = d; changed = true; }
	ImGui::PopStyleColor();
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Designations" );
	ImGui::SameLine();

	bool j = bridge.renderJobs;
	ImGui::PushStyleColor( ImGuiCol_Button, j ? onColor : offColor );
	if ( ImGui::SmallButton( "J" ) ) { j = !j; bridge.renderJobs = j; changed = true; }
	ImGui::PopStyleColor();
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Jobs" );
	ImGui::SameLine();

	bool w = bridge.renderWalls;
	ImGui::PushStyleColor( ImGuiCol_Button, w ? onColor : offColor );
	if ( ImGui::SmallButton( "W" ) ) { w = !w; bridge.renderWalls = w; changed = true; }
	ImGui::PopStyleColor();
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Lower Walls" );
	ImGui::SameLine();

	bool a = bridge.renderAxles;
	ImGui::PushStyleColor( ImGuiCol_Button, a ? onColor : offColor );
	if ( ImGui::SmallButton( "A" ) ) { a = !a; bridge.renderAxles = a; changed = true; }
	ImGui::PopStyleColor();
	if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Axles" );

	if ( changed ) bridge.cmdSetRenderOptions( d, j, w, a );

	ImGui::End();
	ImGui::PopStyleVar();

	// =========================================================================
	// Bottom toolbar
	// =========================================================================
	float toolbarHeight = 55;
	ImGui::SetNextWindowPos( ImVec2( 0, io.DisplaySize.y - toolbarHeight ) );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x, toolbarHeight ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 10, 9 ) );
	ImGui::Begin( "##toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

	float gap = 40.0f; // gap between left and right button groups
	float padding = 10.0f;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float availableW = io.DisplaySize.x - padding * 2 - gap;
	float leftWidth = availableW * 0.5f;
	float rightWidth = availableW * 0.5f;
	// Account for (N-1) spacings between N buttons
	float buttonW = ( leftWidth - spacing * ( toolbarButtonCount - 1 ) ) / toolbarButtonCount;

	// Left side: action buttons
	ImGui::SetCursorPosX( padding );
	for ( int i = 0; i < toolbarButtonCount; ++i )
	{
		if ( i > 0 ) ImGui::SameLine();
		bool active = ( bridge.currentToolbar == toolbarButtons[i].selection );
		if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );
		if ( ImGui::Button( toolbarButtons[i].label, ImVec2( buttonW, 36 ) ) )
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
	}

	// Right side: management panels
	float rightStart = padding + leftWidth + gap;
	float rightButtonW = ( rightWidth - spacing * 5 ) / 6.0f;
	ImGui::SameLine( rightStart );
	if ( ImGui::Button( "Kingdom", ImVec2( rightButtonW, 36 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Kingdom ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Kingdom;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Kingdom )
			bridge.cmdRequestInventoryCategories();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Stockpiles", ImVec2( rightButtonW, 36 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Stockpile ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Stockpile;
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Military", ImVec2( rightButtonW, 36 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Military ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Military;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Military )
			bridge.cmdRequestMilitaryUpdate();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Population", ImVec2( rightButtonW, 36 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Population ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Population;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Population )
			bridge.cmdRequestPopulationUpdate();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Missions", ImVec2( rightButtonW, 36 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::Missions ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::Missions;
		if ( bridge.activeSidePanel == ImGuiBridge::SidePanel::Missions )
			bridge.cmdRequestNeighborsUpdate();
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Log", ImVec2( rightButtonW, 36 ) ) )
	{
		bridge.activeSidePanel = bridge.activeSidePanel == ImGuiBridge::SidePanel::EventLog ? ImGuiBridge::SidePanel::None : ImGuiBridge::SidePanel::EventLog;
	}

	ImGui::End();
	ImGui::PopStyleVar(); // WindowPadding

	// =========================================================================
	// Toolbar expansion panels (above the bottom toolbar)
	// =========================================================================
	if ( bridge.currentToolbar == ButtonSelection::Build )
	{
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - 400 ) );
		ImGui::SetNextWindowSize( ImVec2( 150, 390 ) );
		ImGui::Begin( "##buildcats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

		float catBtnW = ImGui::GetContentRegionAvail().x;

		for ( int i = 0; i < 9; ++i )
		{
			bool active = ( bridge.currentBuildCategory == buildCategories[i].selection );
			if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );
			if ( ImGui::Button( buildCategories[i].label, ImVec2( catBtnW, 30 ) ) )
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

		if ( ImGui::Button( "Deconstruct", ImVec2( catBtnW, 30 ) ) )
		{
			bridge.currentBuildCategory = BuildSelection::None;
			bridge.buildItems.clear();
			bridge.cmdSetSelectionAction( "Deconstruct" );
		}

		ImGui::End();

		// Subcategory buttons (materials, workshop tabs, furniture groups, etc.)
		float catPanelRight = 160; // build cat panel is 150px at x=5
		float subcatPanelRight = catPanelRight; // default if no subcats
		const BuildCategorySubcats* sc = getSubcatsFor( bridge.currentBuildCategory );
		if ( sc && sc->count > 0 )
		{
			// Lazy-init sprite cache
			if ( !bridge.spriteTexCache ) bridge.spriteTexCache = new SpriteTextureCache();

			float subcatW = 115.0f;
			float subcatBtnH = 70.0f;
			float subcatH = sc->count * ( subcatBtnH + 5 ) + 10.0f;
			ImGui::SetNextWindowPos( ImVec2( catPanelRight + 5, io.DisplaySize.y - toolbarHeight - subcatH - 10 ) );
			ImGui::SetNextWindowSize( ImVec2( subcatW, subcatH ) );
			ImGui::Begin( "##buildsubcats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

			// Map subcategory labels to representative sprites for icons
			struct SubcatSpriteInfo { QString itemSID; QString material; };
			static QMap<QString, SubcatSpriteInfo> subcatSprites = {
				{ "Wood",       { "Log", "Oak" } },
				{ "Soil",       { "RawSoil", "Dirt" } },
				{ "Stone",      { "RawStone", "Granite" } },
				{ "Metal",      { "Bar", "Iron" } },
				{ "Other",      { "RawStone", "Ite" } },
				{ "Food",       { "Grain", "Wheat" } },
				{ "Crafts",     { "Plank", "Oak" } },
				{ "Mechanics",  { "Axle", "Oak" } },
				{ "Misc",       { "Torch", "Oak" } },
				{ "Chairs",     { "Chair", "Oak" } },
				{ "Tables",     { "Table", "Oak" } },
				{ "Beds",       { "Bed", "Oak" } },
				{ "Cabinets",   { "Cabinet", "Oak" } },
				{ "Doors",      { "Door", "Oak" } },
				{ "Lights",     { "Torch", "Oak" } },
				{ "Farm",       { "Trough", "Oak" } },
				{ "Mechanism",  { "Lever", "Oak" } },
				{ "Hydraulics", { "Pump", "Oak" } }
			};

			for ( int m = 0; m < sc->count; ++m )
			{
				bool matActive = ( bridge.currentBuildMaterial == sc->subcats[m].dbKey );
				if ( matActive ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );

				ImGui::PushID( m );

				// Show sprite icon + text label as a styled ImageButton or fallback to text button
				auto spriteInfo = subcatSprites.value( sc->subcats[m].label );
				ImTextureID icon = spriteInfo.itemSID.isEmpty() ? (ImTextureID)0 : bridge.spriteTexCache->getTextureForItem( spriteInfo.itemSID, { spriteInfo.material } );

				if ( icon )
				{
					// Icon button with label below
					if ( ImGui::ImageButton( sc->subcats[m].label, icon, ImVec2( 32, 40 ) ) )
					{
						bridge.currentBuildMaterial = sc->subcats[m].dbKey;
						bridge.buildItems.clear();
						s_selectedMats.clear();
						bridge.cmdRequestBuildItems( bridge.currentBuildCategory, bridge.currentBuildMaterial );
					}
					ImGui::TextUnformatted( sc->subcats[m].label );
				}
				else
				{
					// Fallback: text button
					if ( ImGui::Button( sc->subcats[m].label, ImVec2( 85, subcatBtnH ) ) )
					{
						bridge.currentBuildMaterial = sc->subcats[m].dbKey;
						bridge.buildItems.clear();
						s_selectedMats.clear();
						bridge.cmdRequestBuildItems( bridge.currentBuildCategory, bridge.currentBuildMaterial );
					}
				}

				ImGui::PopID();
				if ( matActive ) ImGui::PopStyleColor();
			}

			ImGui::End();
			subcatPanelRight = catPanelRight + 5 + subcatW + 5;
		}

		// Build items list
		drawBuildItemList( bridge, subcatPanelRight );
	}
	else if ( bridge.currentToolbar == ButtonSelection::Mine )
	{
		float panelW = 170.0f;
		float btnH = 30.0f;
		float panelH = 7 * ( btnH + ImGui::GetStyle().ItemSpacing.y ) + ImGui::GetStyle().WindowPadding.y * 2;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##mineactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

		float btnW = ImGui::GetContentRegionAvail().x;
		for ( int i = 0; i < 7; ++i )
		{
			if ( ImGui::Button( mineLabels[i], ImVec2( btnW, btnH ) ) )
			{
				bridge.cmdSetSelectionAction( mineActions[i] );
			}
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Agriculture )
	{
		float panelW = 170.0f;
		float btnH = 30.0f;
		float panelH = 5 * ( btnH + ImGui::GetStyle().ItemSpacing.y ) + ImGui::GetStyle().WindowPadding.y * 2;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##agriactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

		float btnW = ImGui::GetContentRegionAvail().x;
		for ( int i = 0; i < 5; ++i )
		{
			bool hasAction = strlen( agriActions[i] ) > 0;
			if ( !hasAction ) ImGui::BeginDisabled();
			if ( ImGui::Button( agriLabels[i], ImVec2( btnW, btnH ) ) )
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
		float panelW = 180.0f;
		float btnH = 30.0f;
		float panelH = 11 * ( btnH + ImGui::GetStyle().ItemSpacing.y ) + ImGui::GetStyle().WindowPadding.y * 2;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##designactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

		float btnW = ImGui::GetContentRegionAvail().x;
		for ( int i = 0; i < 11; ++i )
		{
			if ( ImGui::Button( designLabels[i], ImVec2( btnW, btnH ) ) )
			{
				bridge.cmdSetSelectionAction( designActions[i] );
			}
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Job )
	{
		float panelW = 180.0f;
		float btnH = 30.0f;
		float panelH = 5 * ( btnH + ImGui::GetStyle().ItemSpacing.y ) + ImGui::GetStyle().WindowPadding.y * 2;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##jobactions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar );

		float btnW = ImGui::GetContentRegionAvail().x;
		for ( int i = 0; i < 5; ++i )
		{
			if ( ImGui::Button( jobLabels[i], ImVec2( btnW, btnH ) ) )
			{
				bridge.cmdSetSelectionAction( jobActions[i] );
			}
		}

		ImGui::End();
	}

	// =========================================================================
	// Toast notifications (single stacked panel, interactive)
	// =========================================================================
	{
		// Generate toasts from new log entries
		auto& logMessages = Global::logger().messages();
		if ( logMessages.size() > bridge.lastLogCount )
		{
			for ( size_t i = bridge.lastLogCount; i < logMessages.size(); ++i )
			{
				auto& lm = logMessages[i];
				if ( lm.type == LogType::WARNING || lm.type == LogType::DANGER ||
					 lm.type == LogType::COMBAT || lm.type == LogType::DEATH ||
					 lm.type == LogType::MIGRATION )
				{
					ImGuiBridge::ToastNotification toast;
					toast.text = lm.message;
					toast.severity = lm.type;
					toast.createdTick = GameState::tick;
					toast.alpha = 1.0f;
					toast.entityID = lm.source;
					toast.eventPosX = lm.posX;
					toast.eventPosY = lm.posY;
					toast.eventPosZ = lm.posZ;
					toast.dismissed = false;
					bridge.activeToasts.append( toast );

					// Cap visible toasts
					while ( bridge.activeToasts.size() > bridge.MAX_TOASTS )
						bridge.activeToasts.removeFirst();
				}
			}
			bridge.lastLogCount = logMessages.size();
		}

		// Remove dismissed and faded toasts
		for ( int i = bridge.activeToasts.size() - 1; i >= 0; --i )
		{
			auto& toast = bridge.activeToasts[i];
			if ( toast.dismissed )
			{
				bridge.activeToasts.removeAt( i );
				continue;
			}
			quint64 age = GameState::tick - toast.createdTick;
			if ( age > 4500 )
			{
				toast.alpha = qMax( 0.0f, 1.0f - (float)( age - 4500 ) / 1500.0f );
				if ( toast.alpha <= 0.0f )
				{
					bridge.activeToasts.removeAt( i );
				}
			}
		}

		// Render all toasts in a single stacked window
		if ( !bridge.activeToasts.isEmpty() )
		{
			float toastWidth = 350.0f;
			ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - toastWidth - 10, 80 ), ImGuiCond_Always );
			ImGui::SetNextWindowSize( ImVec2( toastWidth, 0 ), ImGuiCond_Always );
			ImGui::SetNextWindowBgAlpha( 0.88f );
			ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 12, 10 ) );
			ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8, 6 ) );

			ImGui::Begin( "##toasts", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize );

			for ( int i = 0; i < bridge.activeToasts.size(); ++i )
			{
				auto& toast = bridge.activeToasts[i];
				ImGui::PushID( i );

				// Severity color
				ImVec4 color;
				switch ( toast.severity )
				{
					case LogType::DEATH:     color = ImVec4( 1.0f, 0.2f, 0.2f, toast.alpha ); break;
					case LogType::DANGER:    color = ImVec4( 1.0f, 0.5f, 0.1f, toast.alpha ); break;
					case LogType::WARNING:   color = ImVec4( 1.0f, 0.8f, 0.2f, toast.alpha ); break;
					case LogType::COMBAT:    color = ImVec4( 0.9f, 0.3f, 0.3f, toast.alpha ); break;
					case LogType::MIGRATION: color = ImVec4( 0.3f, 0.8f, 0.3f, toast.alpha ); break;
					case LogType::WILDLIFE:  color = ImVec4( 0.8f, 0.6f, 0.2f, toast.alpha ); break;
					default:                 color = ImVec4( 0.7f, 0.7f, 0.7f, toast.alpha ); break;
				}

				// Message text
				float buttonAreaWidth = ( toast.entityID != 0 ) ? 90 : 40;
				ImGui::PushStyleColor( ImGuiCol_Text, color );
				ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + toastWidth - 24 - buttonAreaWidth );
				ImGui::TextWrapped( "%s", toast.text.toStdString().c_str() );
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();

				// Buttons on the same line as the last line of text
				ImGui::SameLine( toastWidth - 24 - buttonAreaWidth );

				// "Go To" button — navigate camera to entity or stored position
				bool hasGoTo = ( toast.entityID != 0 ) ||
					( toast.eventPosX > 0 || toast.eventPosY > 0 || toast.eventPosZ > 0 );
				if ( hasGoTo )
				{
					ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.4f, 0.6f, 0.7f ) );
					if ( ImGui::SmallButton( "Go To" ) )
					{
						// Try live entity first, fall back to stored position
						if ( toast.entityID != 0 )
							bridge.cmdNavigateToEntity( toast.entityID );
						// If entity not found (dead), use stored position
						if ( !bridge.pendingCameraNav )
							bridge.cmdNavigateToPosition( Position( toast.eventPosX, toast.eventPosY, toast.eventPosZ ) );
					}
					ImGui::PopStyleColor();
					ImGui::SameLine();
				}

				// Close button
				ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.5f, 0.5f, 0.5f, toast.alpha ) );
				if ( ImGui::SmallButton( "X" ) )
				{
					toast.dismissed = true;
				}
				ImGui::PopStyleColor();

				ImGui::PopID();

				if ( i < bridge.activeToasts.size() - 1 )
				{
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
				}
			}

			ImGui::End();
			ImGui::PopStyleVar( 2 );
		}
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
