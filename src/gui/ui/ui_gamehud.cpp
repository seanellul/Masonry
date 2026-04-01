#include "ui_gamehud.h"
#include "../imguibridge.h"
#include "../IconsFontAwesome6.h"
#include "../IconsRpgAwesome.h"
#include "../../base/global.h"
#include "../../base/gamestate.h"
#include "../../base/logger.h"
#include "../strings.h"
#include "../spritetexturecache.h"

#include <imgui.h>

namespace
{

// Dig actions (was "Mine")
struct ActionDef { const char* icon; const char* label; const char* action; };

// Build subcategories
struct BuildCategoryButton
{
	const char* icon;
	const char* label;
	BuildSelection selection;
};

const BuildCategoryButton buildCategories[] = {
	{ ICON_FA_GEAR, "Workshops", BuildSelection::Workshop },
	{ ICON_RA_CASTLE_EMBLEM, "Structures", BuildSelection::Wall },
	{ ICON_FA_COUCH, "Furniture", BuildSelection::Furniture },
	{ ICON_FA_WRENCH, "Utility", BuildSelection::Utility },
	{ ICON_FA_BOX, "Containers", BuildSelection::Containers },
};

// Structure sub-items (wall, floor, stairs, ramp, fence) — displayed as a grid within Structures
struct StructureType { const char* icon; const char* label; BuildSelection selection; };
const StructureType structureTypes[] = {
	{ ICON_FA_BORDER_ALL, "Wall", BuildSelection::Wall },
	{ ICON_FA_SQUARE, "Floor", BuildSelection::Floor },
	{ ICON_FA_STAIRS, "Stairs", BuildSelection::Stairs },
	{ ICON_RA_TRAIL, "Ramp", BuildSelection::Ramps },
	{ ICON_RA_WOODEN_SIGN, "Fence", BuildSelection::Fence },
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

// Dig actions (Shape → Dig)
const ActionDef digActions[] = {
	{ ICON_FA_HAMMER, "Mine", "Mine" },
	{ ICON_FA_GEM, "Mine Vein", "ExplorativeMine" },
	{ ICON_FA_TRASH, "Remove Floor", "RemoveFloor" },
	{ ICON_FA_ARROW_DOWN, "Stairs Down", "DigStairsDown" },
	{ ICON_FA_ARROW_UP, "Stairs Up", "MineStairsUp" },
	{ ICON_FA_ANGLE_DOWN, "Ramp", "DigRampDown" },
	{ ICON_FA_CIRCLE, "Hole", "DigHole" },
};

// Nature actions (Shape → Nature)
const ActionDef natureActions[] = {
	{ ICON_RA_AXE, "Cut Tree", "FellTree" },
	{ ICON_FA_SEEDLING, "Plant Tree", "" },
	{ ICON_FA_LEAF, "Harvest", "HarvestTree" },
	{ ICON_FA_HAND, "Forage", "Forage" },
	{ ICON_FA_XMARK, "Remove Plant", "RemovePlant" },
};

// Zone actions — organized by category
struct ZoneDef { const char* icon; const char* label; const char* action; ZoneCategory category; };
const ZoneDef zoneActions[] = {
	{ ICON_FA_BOX_ARCHIVE, "Stockpile", "CreateStockpile", ZoneCategory::Storage },
	{ ICON_FA_SEEDLING, "Farm", "CreateFarm", ZoneCategory::Production },
	{ ICON_FA_TREE, "Grove", "CreateGrove", ZoneCategory::Production },
	{ ICON_RA_SHEEP, "Pasture", "CreatePasture", ZoneCategory::Production },
	{ ICON_FA_HOUSE, "Personal Room", "CreateRoom", ZoneCategory::Rooms },
	{ ICON_FA_BED, "Dormitory", "CreateDorm", ZoneCategory::Rooms },
	{ ICON_FA_UTENSILS, "Dining Hall", "CreateDining", ZoneCategory::Rooms },
	{ ICON_RA_HEALTH, "Hospital", "CreateHospital", ZoneCategory::Rooms },
	{ ICON_FA_BAN, "Forbidden", "CreateNoPass", ZoneCategory::Control },
	{ ICON_FA_SHIELD, "Guard", "CreateGuardArea", ZoneCategory::Control },
	{ ICON_FA_XMARK, "Remove Zone", "RemoveDesignation", ZoneCategory::Control },
};

// Manage → Job actions
const ActionDef jobActions[] = {
	{ ICON_FA_PAUSE, "Suspend", "SuspendJob" },
	{ ICON_FA_PLAY, "Resume", "ResumeJob" },
	{ ICON_FA_XMARK, "Cancel", "CancelJob" },
	{ ICON_FA_ARROW_DOWN, "Lower Priority", "LowerPrio" },
	{ ICON_FA_ARROW_UP, "Raise Priority", "RaisePrio" },
};

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

	// Helper to draw a toolbar button with icon
	auto toolbarBtn = [&]( const char* icon, const char* label, ButtonSelection sel ) -> bool {
		bool active = ( bridge.currentToolbar == sel );
		if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );
		char fullLabel[128];
		snprintf( fullLabel, sizeof(fullLabel), "%s %s", icon, label );
		bool clicked = ImGui::Button( fullLabel, ImVec2( 0, 36 ) );
		if ( active ) ImGui::PopStyleColor();
		return clicked;
	};

	float padding = 10.0f;
	float spacing = ImGui::GetStyle().ItemSpacing.x;

	// Left side: 3 primary action buttons
	ImGui::SetCursorPosX( padding );

	auto toggleToolbar = [&]( ButtonSelection sel ) {
		if ( bridge.currentToolbar == sel )
		{
			bridge.currentToolbar = ButtonSelection::None;
			bridge.currentBuildCategory = BuildSelection::None;
			bridge.buildItems.clear();
		}
		else
		{
			bridge.currentToolbar = sel;
			bridge.currentBuildCategory = BuildSelection::None;
			bridge.buildItems.clear();
		}
	};

	if ( toolbarBtn( ICON_FA_HAMMER, "Shape", ButtonSelection::Shape ) )
		toggleToolbar( ButtonSelection::Shape );

	ImGui::SameLine();
	if ( toolbarBtn( ICON_FA_VECTOR_SQUARE, "Zone", ButtonSelection::Zone ) )
		toggleToolbar( ButtonSelection::Zone );

	ImGui::SameLine();
	if ( toolbarBtn( ICON_FA_LIST_CHECK, "Manage", ButtonSelection::Manage ) )
		toggleToolbar( ButtonSelection::Manage );

	// Right side: management panels
	float rightWidth = io.DisplaySize.x * 0.5f;
	float rightStart = io.DisplaySize.x - rightWidth - padding;
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
	// Unified action panel (above toolbar)
	// =========================================================================
	if ( bridge.currentToolbar == ButtonSelection::Shape )
	{
		float panelW = io.DisplaySize.x * 0.45f;
		if ( panelW < 500 ) panelW = 500;
		float panelH = 350;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##shapepanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		// Sub-tabs: Build | Dig | Nature
		if ( ImGui::BeginTabBar( "ShapeTabs" ) )
		{
			// ---- BUILD TAB ----
			if ( ImGui::BeginTabItem( ICON_FA_HAMMER " Build" ) )
			{
				bridge.currentShapeTab = ShapeTab::Build;

				// Category row (horizontal icon buttons)
				for ( int i = 0; i < 5; ++i )
				{
					if ( i > 0 ) ImGui::SameLine();
					bool active = ( bridge.currentBuildCategory == buildCategories[i].selection );
					if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.3f, 0.5f, 0.7f, 1.0f ) );

					char catLabel[128];
					snprintf( catLabel, sizeof(catLabel), "%s %s", buildCategories[i].icon, buildCategories[i].label );
					if ( ImGui::Button( catLabel, ImVec2( 0, 30 ) ) )
					{
						bridge.currentBuildCategory = buildCategories[i].selection;
						bridge.currentBuildMaterial.clear();
						bridge.buildItems.clear();
						s_selectedMats.clear();

						// If Structures, don't auto-request yet (user picks wall/floor/stairs)
						if ( buildCategories[i].selection != BuildSelection::Wall )
						{
							const BuildCategorySubcats* sc = getSubcatsFor( buildCategories[i].selection );
							if ( sc && sc->count > 0 )
							{
								bridge.currentBuildMaterial = sc->subcats[0].dbKey;
								bridge.cmdRequestBuildItems( buildCategories[i].selection, bridge.currentBuildMaterial );
							}
							else
							{
								bridge.cmdRequestBuildItems( buildCategories[i].selection, "" );
							}
						}
					}
					if ( active ) ImGui::PopStyleColor();
				}

				// If Structures selected, show structure type buttons
				if ( bridge.currentBuildCategory == BuildSelection::Wall ||
					 bridge.currentBuildCategory == BuildSelection::Floor ||
					 bridge.currentBuildCategory == BuildSelection::Stairs ||
					 bridge.currentBuildCategory == BuildSelection::Ramps ||
					 bridge.currentBuildCategory == BuildSelection::Fence )
				{
					ImGui::Spacing();
					for ( int i = 0; i < 5; ++i )
					{
						if ( i > 0 ) ImGui::SameLine();
						bool active = ( bridge.currentBuildCategory == structureTypes[i].selection );
						if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.45f, 0.65f, 1.0f ) );
						char stLabel[64];
						snprintf( stLabel, sizeof(stLabel), "%s %s", structureTypes[i].icon, structureTypes[i].label );
						if ( ImGui::Button( stLabel ) )
						{
							bridge.currentBuildCategory = structureTypes[i].selection;
							bridge.currentBuildMaterial.clear();
							bridge.buildItems.clear();
							s_selectedMats.clear();
							const BuildCategorySubcats* sc = getSubcatsFor( structureTypes[i].selection );
							if ( sc && sc->count > 0 )
							{
								bridge.currentBuildMaterial = sc->subcats[0].dbKey;
								bridge.cmdRequestBuildItems( structureTypes[i].selection, bridge.currentBuildMaterial );
							}
						}
						if ( active ) ImGui::PopStyleColor();
					}
				}

				// Material tabs (if current category has subcategories)
				if ( bridge.currentBuildCategory != BuildSelection::None )
				{
					const BuildCategorySubcats* sc = getSubcatsFor( bridge.currentBuildCategory );
					if ( sc && sc->count > 0 )
					{
						ImGui::Spacing();
						if ( ImGui::BeginTabBar( "MatTabs" ) )
						{
							for ( int m = 0; m < sc->count; ++m )
							{
								if ( ImGui::BeginTabItem( sc->subcats[m].label ) )
								{
									if ( bridge.currentBuildMaterial != sc->subcats[m].dbKey )
									{
										bridge.currentBuildMaterial = sc->subcats[m].dbKey;
										bridge.buildItems.clear();
										s_selectedMats.clear();
										bridge.cmdRequestBuildItems( bridge.currentBuildCategory, bridge.currentBuildMaterial );
									}
									ImGui::EndTabItem();
								}
							}
							ImGui::EndTabBar();
						}
					}

					// Build items list (scrollable)
					ImGui::BeginChild( "BuildItems", ImVec2( 0, 0 ), false );
					if ( !bridge.spriteTexCache )
						bridge.spriteTexCache = new SpriteTextureCache();

					float iconSize = 48.0f;
					for ( const auto& item : bridge.buildItems )
					{
						ImGui::PushID( item.id.toStdString().c_str() );
						ImGui::Separator();

						ImVec2 startPos = ImGui::GetCursorPos();

						// Icon
						ImTextureID texID = (ImTextureID)0;
						if ( !item.buffer.empty() && item.iconWidth > 0 && item.iconHeight > 0 )
						{
							unsigned int cacheKey = qHash( item.id ) + 100000;
							texID = bridge.spriteTexCache->getTextureFromBuffer( cacheKey, item.buffer.data(), item.iconWidth, item.iconHeight );
						}
						if ( texID )
							ImGui::Image( texID, ImVec2( iconSize, iconSize ) );
						else
							ImGui::Dummy( ImVec2( iconSize, iconSize ) );

						// Right of icon: name + material dropdowns + build button
						float rightX = startPos.x + iconSize + 8;
						ImGui::SetCursorPos( ImVec2( rightX, startPos.y ) );
						ImGui::Text( "%s", item.name.toStdString().c_str() );

						QStringList mats;
						bool canBuild = true;
						float dropdownW = qMax( 120.0f, ImGui::GetContentRegionAvail().x - iconSize - 80 );

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

							QString itemName = S::s( "$ItemName_" + req.itemID );
							ImGui::Text( "%d", req.amount );
							ImGui::SameLine();
							ImGui::SetNextItemWidth( dropdownW );
							QString comboLabel = "##mat" + QString::number( r );
							QString matName = S::s( "$MaterialName_" + req.availableMats[selIdx].first );
							QString preview = matName + " " + itemName + " (" + QString::number( req.availableMats[selIdx].second ) + ")";

							if ( ImGui::BeginCombo( comboLabel.toStdString().c_str(), preview.toStdString().c_str() ) )
							{
								for ( int mi = 0; mi < req.availableMats.size(); ++mi )
								{
									QString mLabel = S::s( "$MaterialName_" + req.availableMats[mi].first );
									QString label = mLabel + " " + itemName + " (" + QString::number( req.availableMats[mi].second ) + ")";
									if ( ImGui::Selectable( label.toStdString().c_str(), mi == selIdx ) )
										selIdx = mi;
								}
								ImGui::EndCombo();
							}
							mats.append( req.availableMats[selIdx].first );
						}

						ImGui::SetCursorPosX( rightX );
						if ( !canBuild ) ImGui::BeginDisabled();
						if ( ImGui::SmallButton( ICON_FA_HAMMER " Build" ) )
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
								if ( ImGui::SmallButton( "Fill Hole" ) ) bridge.cmdBuild( item.biType, "FillHole", item.id, mats );
								if ( !canBuild ) ImGui::EndDisabled();
							}
							if ( isWall || isFloor )
							{
								ImGui::SameLine();
								if ( !canBuild ) ImGui::BeginDisabled();
								if ( ImGui::SmallButton( ICON_FA_RIGHT_LEFT " Replace" ) ) bridge.cmdBuild( item.biType, "Replace", item.id, mats );
								if ( !canBuild ) ImGui::EndDisabled();
							}
						}

						float endY = startPos.y + iconSize + ImGui::GetStyle().ItemSpacing.y;
						if ( ImGui::GetCursorPosY() < endY )
							ImGui::SetCursorPosY( endY );

						ImGui::PopID();
					}
					ImGui::EndChild();
				}

				// Deconstruct button at bottom
				ImGui::Separator();
				if ( ImGui::Button( ICON_FA_TRASH " Deconstruct" ) )
				{
					bridge.currentBuildCategory = BuildSelection::None;
					bridge.buildItems.clear();
					bridge.cmdSetSelectionAction( "Deconstruct" );
				}

				ImGui::EndTabItem();
			}

			// ---- DIG TAB ----
			if ( ImGui::BeginTabItem( ICON_FA_MOUNTAIN " Dig" ) )
			{
				bridge.currentShapeTab = ShapeTab::Dig;
				ImGui::Spacing();

				// Grid of dig action buttons
				float btnW = 150.0f;
				for ( int i = 0; i < 7; ++i )
				{
					if ( i % 3 != 0 ) ImGui::SameLine();
					char label[128];
					snprintf( label, sizeof(label), "%s %s", digActions[i].icon, digActions[i].label );
					if ( ImGui::Button( label, ImVec2( btnW, 36 ) ) )
					{
						bridge.cmdSetSelectionAction( digActions[i].action );
					}
				}

				ImGui::EndTabItem();
			}

			// ---- NATURE TAB ----
			if ( ImGui::BeginTabItem( ICON_FA_TREE " Nature" ) )
			{
				bridge.currentShapeTab = ShapeTab::Nature;
				ImGui::Spacing();

				float btnW = 150.0f;
				for ( int i = 0; i < 5; ++i )
				{
					if ( i % 3 != 0 ) ImGui::SameLine();
					bool hasAction = strlen( natureActions[i].action ) > 0;
					if ( !hasAction ) ImGui::BeginDisabled();
					char label[128];
					snprintf( label, sizeof(label), "%s %s", natureActions[i].icon, natureActions[i].label );
					if ( ImGui::Button( label, ImVec2( btnW, 36 ) ) )
					{
						if ( hasAction )
							bridge.cmdSetSelectionAction( natureActions[i].action );
					}
					if ( !hasAction ) ImGui::EndDisabled();
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Zone )
	{
		float panelW = 450;
		float panelH = 250;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##zonepanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		// Category tabs
		if ( ImGui::BeginTabBar( "ZoneTabs" ) )
		{
			struct ZoneCatTab { const char* label; ZoneCategory cat; };
			ZoneCatTab zoneTabs[] = {
				{ ICON_FA_BOX " Storage", ZoneCategory::Storage },
				{ ICON_FA_SEEDLING " Production", ZoneCategory::Production },
				{ ICON_FA_HOUSE " Rooms", ZoneCategory::Rooms },
				{ ICON_FA_SHIELD " Control", ZoneCategory::Control },
			};

			for ( auto& zt : zoneTabs )
			{
				if ( ImGui::BeginTabItem( zt.label ) )
				{
					bridge.currentZoneCategory = zt.cat;
					ImGui::Spacing();

					// Show zone buttons for this category
					float btnW = 180.0f;
					int col = 0;
					for ( int i = 0; i < 11; ++i )
					{
						if ( zoneActions[i].category != zt.cat ) continue;
						if ( col > 0 ) ImGui::SameLine();
						char label[128];
						snprintf( label, sizeof(label), "%s %s", zoneActions[i].icon, zoneActions[i].label );
						if ( ImGui::Button( label, ImVec2( btnW, 36 ) ) )
						{
							bridge.cmdSetSelectionAction( zoneActions[i].action );
						}
						col++;
						if ( col >= 2 ) col = 0;
					}

					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}
	else if ( bridge.currentToolbar == ButtonSelection::Manage )
	{
		float panelW = 400;
		float panelH = 220;
		ImGui::SetNextWindowPos( ImVec2( 5, io.DisplaySize.y - toolbarHeight - panelH - 5 ) );
		ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );
		ImGui::Begin( "##managepanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );

		// Two columns: Jobs and Demolition
		ImGui::BeginChild( "JobCol", ImVec2( ImGui::GetContentRegionAvail().x * 0.6f, 0 ), true );
		ImGui::Text( ICON_FA_LIST_CHECK " Job Control" );
		ImGui::Separator();
		ImGui::Spacing();

		float btnW = ImGui::GetContentRegionAvail().x;
		for ( int i = 0; i < 5; ++i )
		{
			char label[128];
			snprintf( label, sizeof(label), "%s %s", jobActions[i].icon, jobActions[i].label );
			if ( ImGui::Button( label, ImVec2( btnW, 32 ) ) )
			{
				bridge.cmdSetSelectionAction( jobActions[i].action );
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild( "DemoCol", ImVec2( 0, 0 ), true );
		ImGui::Text( ICON_FA_HAMMER " Demolition" );
		ImGui::Separator();
		ImGui::Spacing();

		btnW = ImGui::GetContentRegionAvail().x;
		if ( ImGui::Button( ICON_FA_TRASH " Deconstruct", ImVec2( btnW, 32 ) ) )
		{
			bridge.cmdSetSelectionAction( "Deconstruct" );
		}
		ImGui::EndChild();

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

				// Combat logs: group into a single updating combat toast
				if ( lm.type == LogType::COMBAT )
				{
					bridge.combatToast.active = true;
					bridge.combatToast.latestEvent = lm.message;
					bridge.combatToast.lastUpdateTick = GameState::tick;
					bridge.combatToast.eventCount++;
					bridge.combatToast.entityID = lm.source;
					bridge.combatToast.posX = lm.posX;
					bridge.combatToast.posY = lm.posY;
					bridge.combatToast.posZ = lm.posZ;
					continue; // don't create individual toast
				}

				if ( lm.type == LogType::WARNING || lm.type == LogType::DANGER ||
					 lm.type == LogType::DEATH ||
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

		// Deactivate combat toast if no updates for 300 ticks (~30 seconds)
		if ( bridge.combatToast.active )
		{
			quint64 combatAge = GameState::tick - bridge.combatToast.lastUpdateTick;
			if ( combatAge > 300 )
			{
				bridge.combatToast.active = false;
				bridge.combatToast.eventCount = 0;
			}
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
		if ( !bridge.activeToasts.isEmpty() || bridge.combatToast.active )
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

			// Grouped combat toast — single live-updating entry
			if ( bridge.combatToast.active )
			{
				ImGui::PushID( "combatToast" );

				// Header: "Combat (N events)"
				ImVec4 headerColor( 0.9f, 0.25f, 0.25f, 1.0f );
				ImGui::PushStyleColor( ImGuiCol_Text, headerColor );
				ImGui::Text( "Combat (%d events)", bridge.combatToast.eventCount );
				ImGui::PopStyleColor();

				// Latest event
				float buttonAreaWidth = 90;
				ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.85f, 0.5f, 0.5f, 1.0f ) );
				ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + toastWidth - 24 - buttonAreaWidth );
				ImGui::TextWrapped( "%s", bridge.combatToast.latestEvent.toStdString().c_str() );
				ImGui::PopTextWrapPos();
				ImGui::PopStyleColor();

				// Buttons
				ImGui::SameLine( toastWidth - 24 - buttonAreaWidth );
				ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.2f, 0.4f, 0.6f, 0.7f ) );
				if ( ImGui::SmallButton( "Go To" ) )
				{
					if ( bridge.combatToast.entityID != 0 )
						bridge.cmdNavigateToEntity( bridge.combatToast.entityID );
					if ( !bridge.pendingCameraNav )
						bridge.cmdNavigateToPosition( Position( bridge.combatToast.posX, bridge.combatToast.posY, bridge.combatToast.posZ ) );
				}
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if ( ImGui::SmallButton( "X" ) )
				{
					bridge.combatToast.active = false;
					bridge.combatToast.eventCount = 0;
				}

				ImGui::Separator();
				ImGui::PopID();
			}

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
