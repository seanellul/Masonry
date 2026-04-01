#include "ui_sidepanels.h"
#include "ui_helpers.h"
#include "../imguibridge.h"
#include "../eventconnector.h"
#include "../aggregatorstockpile.h"
#include "../strings.h"
#include "../../base/global.h"
#include "../../base/gamestate.h"
#include "../../base/logger.h"
#include "../../base/db.h"
#include "../../base/config.h"
#include <imgui.h>

// =============================================================================
// Kingdom / Inventory panel
// =============================================================================
void drawKingdomPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Kingdom", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	sectionHeader( "Inventory" );

	if ( ImGui::BeginTable( "InvTable", 3,
		ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp ) )
	{
		ImGui::TableSetupColumn( "Category", 0, 3.0f );
		ImGui::TableSetupColumn( "In Stock", 0, 1.0f );
		ImGui::TableSetupColumn( "Total", 0, 1.0f );
		ImGui::TableHeadersRow();

		int totalStock = 0, totalAll = 0;
		for ( const auto& cat : bridge.inventoryCategories )
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text( "%s", cat.name.toStdString().c_str() );
			ImGui::TableNextColumn();
			ImGui::Text( "%d", cat.countInStockpiles );
			ImGui::TableNextColumn();
			ImGui::Text( "%d", cat.countTotal );
			totalStock += cat.countInStockpiles;
			totalAll += cat.countTotal;
		}

		// Totals row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextColored( ImVec4( 0.7f, 0.85f, 1.0f, 1.0f ), "Total" );
		ImGui::TableNextColumn();
		ImGui::TextColored( ImVec4( 0.7f, 0.85f, 1.0f, 1.0f ), "%d", totalStock );
		ImGui::TableNextColumn();
		ImGui::TextColored( ImVec4( 0.7f, 0.85f, 1.0f, 1.0f ), "%d", totalAll );

		ImGui::EndTable();
	}

	ImGui::End();
}

// =============================================================================
// Stockpile management
// =============================================================================
void drawStockpilePanel( ImGuiBridge& bridge )
{

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 130 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 400, io.DisplaySize.y - 200 ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Stockpile", &open, 0 );

	if ( !open )
	{
		bridge.cmdCloseStockpileWindow();
	}

	auto& sp = bridge.stockpileInfo;

	sectionHeader( "Settings" );

	// Name
	static char name[128];
	snprintf( name, sizeof( name ), "%s", sp.name.toStdString().c_str() );
	if ( ImGui::InputText( "Name", name, sizeof( name ), ImGuiInputTextFlags_EnterReturnsTrue ) )
	{
		bridge.cmdStockpileSetOptions( bridge.activeStockpileID, name, sp.priority, sp.suspended, sp.pullFromOthers, sp.allowPullFromHere );
	}

	// Priority
	int prio = sp.priority;
	if ( ImGui::SliderInt( "Priority", &prio, 0, sp.maxPriority ) )
	{
		bridge.cmdStockpileSetOptions( bridge.activeStockpileID, sp.name, prio, sp.suspended, sp.pullFromOthers, sp.allowPullFromHere );
	}

	// Toggles
	bool suspended = sp.suspended;
	if ( ImGui::Checkbox( "Suspended", &suspended ) )
	{
		bridge.cmdStockpileSetOptions( bridge.activeStockpileID, sp.name, sp.priority, suspended, sp.pullFromOthers, sp.allowPullFromHere );
	}

	bool pullOthers = sp.pullFromOthers;
	ImGui::SameLine();
	if ( ImGui::Checkbox( "Pull from others", &pullOthers ) )
	{
		bridge.cmdStockpileSetOptions( bridge.activeStockpileID, sp.name, sp.priority, sp.suspended, pullOthers, sp.allowPullFromHere );
	}

	bool allowPull = sp.allowPullFromHere;
	ImGui::SameLine();
	if ( ImGui::Checkbox( "Allow pull", &allowPull ) )
	{
		bridge.cmdStockpileSetOptions( bridge.activeStockpileID, sp.name, sp.priority, sp.suspended, sp.pullFromOthers, allowPull );
	}

	// Limit with material
	bool limitWithMat = sp.limitWithMaterial;
	if ( ImGui::Checkbox( "Count by material", &limitWithMat ) )
	{
		bridge.cmdStockpileSetLimitWithMaterial( bridge.activeStockpileID, limitWithMat );
	}
	if ( ImGui::IsItemHovered() )
	{
		ImGui::SetTooltip( "When enabled, item limits count each material separately" );
	}

	// Copy / Paste settings
	if ( ImGui::Button( "Copy Settings" ) )
	{
		bridge.cmdStockpileCopySettings( bridge.activeStockpileID );
	}
	ImGui::SameLine();
	bool hasClip = Global::eventConnector->aggregatorStockpile()->hasClipboard();
	if ( !hasClip ) ImGui::BeginDisabled();
	if ( ImGui::Button( "Paste Settings" ) )
	{
		bridge.cmdStockpilePasteSettings( bridge.activeStockpileID );
	}
	if ( !hasClip ) ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::Text( "Capacity: %d / %d items (%d reserved)", sp.itemCount, sp.capacity, sp.reserved );

	// Progress bar for capacity
	float capRatio = sp.capacity > 0 ? (float)sp.itemCount / sp.capacity : 0.0f;
	ImVec4 capColor = capRatio > 0.9f ? ImVec4( 0.9f, 0.2f, 0.2f, 1.0f ) :
	                  capRatio > 0.7f ? ImVec4( 0.9f, 0.7f, 0.2f, 1.0f ) :
	                                    ImVec4( 0.2f, 0.7f, 0.3f, 1.0f );
	ImGui::PushStyleColor( ImGuiCol_PlotHistogram, capColor );
	ImGui::ProgressBar( qBound( 0.0f, capRatio, 1.0f ), ImVec2( -1, 0 ) );
	ImGui::PopStyleColor();

	ImGui::Separator();

	if ( ImGui::BeginTabBar( "StockpileTabs" ) )
	{
	// =========================================================================
	// TAB 1: Ledger — what's stored in this stockpile
	// =========================================================================
	if ( ImGui::BeginTabItem( "Ledger" ) )
	{
		if ( sp.summary.isEmpty() )
		{
			ImGui::TextDisabled( "No items stored" );
		}
		else
		{
			ImGui::BeginChild( "LedgerScroll", ImVec2( 0, 0 ), false );
			ImGui::Columns( 3, "ledgerCols" );
			ImGui::SetColumnWidth( 0, 40 );
			ImGui::Text( "Qty" ); ImGui::NextColumn();
			ImGui::Text( "Item" ); ImGui::NextColumn();
			ImGui::Text( "Material" ); ImGui::NextColumn();
			ImGui::Separator();

			for ( const auto& item : sp.summary )
			{
				ImGui::Text( "%d", item.count ); ImGui::NextColumn();
				ImGui::Text( "%s", item.itemName.toStdString().c_str() ); ImGui::NextColumn();
				ImGui::Text( "%s", item.materialName.toStdString().c_str() ); ImGui::NextColumn();
			}

			ImGui::Columns( 1 );
			ImGui::EndChild();
		}
		ImGui::EndTabItem();
	}

	// =========================================================================
	// TAB 2: Filters — what items this stockpile accepts
	// =========================================================================
	if ( ImGui::BeginTabItem( "Filters" ) )
	{

	// Search bar
	static char searchBuf[128] = "";
	ImGui::Text( "Search:" );
	ImGui::SameLine();
	ImGui::SetNextItemWidth( -1 );
	ImGui::InputText( "##filterSearch", searchBuf, sizeof( searchBuf ) );
	QString searchStr = QString( searchBuf ).toLower();

	ImGui::Text( "Filters:" );
	ImGui::SameLine();
	if ( ImGui::SmallButton( "Select All" ) )
	{
		auto& filter = bridge.stockpileInfo.filter;
		for ( const auto& cat : filter.categories() )
			bridge.cmdStockpileSetActive( bridge.activeStockpileID, true, cat, "", "", "" );
	}
	ImGui::SameLine();
	if ( ImGui::SmallButton( "Unselect All" ) )
	{
		auto& filter = bridge.stockpileInfo.filter;
		for ( const auto& cat : filter.categories() )
			bridge.cmdStockpileSetActive( bridge.activeStockpileID, false, cat, "", "", "" );
	}

	ImGui::BeginChild( "FilterTree", ImVec2( 0, 0 ), false );

	// Match tree node row height to checkbox height
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( ImGui::GetStyle().FramePadding.x, 6 ) );

	// Render filter tree: Category (checkbox) → Group (checkbox) → Item (checkbox) → Material (checkbox)
	auto& filter = bridge.stockpileInfo.filter;
	for ( const auto& cat : filter.categories() )
	{
		// Search filter: skip categories with no matching content
		bool catMatchesSearch = searchStr.isEmpty() || cat.toLower().contains( searchStr );
		bool anyChildMatches = false;
		if ( !catMatchesSearch )
		{
			for ( const auto& group : filter.groups( cat ) )
			{
				if ( group.toLower().contains( searchStr ) ) { anyChildMatches = true; break; }
				for ( const auto& item : filter.items( cat, group ) )
				{
					if ( item.toLower().contains( searchStr ) ) { anyChildMatches = true; break; }
				}
				if ( anyChildMatches ) break;
			}
		}
		if ( !catMatchesSearch && !anyChildMatches ) continue;

		ImGui::PushID( cat.toStdString().c_str() );

		// Category checkbox + tree node
		// Check if all items in category are checked
		bool allCatChecked = true;
		bool anyCatChecked = false;
		for ( const auto& group : filter.groups( cat ) )
		{
			for ( const auto& item : filter.items( cat, group ) )
			{
				for ( const auto& mat : filter.materials( cat, group, item ) )
				{
					if ( filter.getCheckState( cat, group, item, mat ) )
						anyCatChecked = true;
					else
						allCatChecked = false;
				}
			}
		}
		if ( !anyCatChecked ) allCatChecked = false;

		bool catChecked = allCatChecked;
		bool catPartial = anyCatChecked && !allCatChecked;
		bool catOpen = ImGui::TreeNodeEx( "##catTree", ( !searchStr.isEmpty() ? ImGuiTreeNodeFlags_DefaultOpen : 0 ) | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth );
		ImGui::SameLine();
		if ( catPartial ) { ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( 0.7f, 0.7f, 0.3f, 1.0f ) ); catChecked = true; }
		if ( ImGui::Checkbox( cat.toStdString().c_str(), &catChecked ) )
		{
			bridge.cmdStockpileSetActive( bridge.activeStockpileID, catPartial ? true : catChecked, cat, "", "", "" );
		}
		if ( catPartial ) ImGui::PopStyleColor();

		if ( catOpen )
		{
			for ( const auto& group : filter.groups( cat ) )
			{
				// Search filter at group level
				bool groupMatches = searchStr.isEmpty() || group.toLower().contains( searchStr ) || catMatchesSearch;
				bool anyItemMatches = false;
				if ( !groupMatches )
				{
					for ( const auto& item : filter.items( cat, group ) )
					{
						if ( item.toLower().contains( searchStr ) ) { anyItemMatches = true; break; }
					}
				}
				if ( !groupMatches && !anyItemMatches ) continue;

				ImGui::PushID( group.toStdString().c_str() );

				// Group checkbox + tree node
				bool allGroupChecked = true;
				bool anyGroupChecked = false;
				for ( const auto& item : filter.items( cat, group ) )
				{
					for ( const auto& mat : filter.materials( cat, group, item ) )
					{
						if ( filter.getCheckState( cat, group, item, mat ) )
							anyGroupChecked = true;
						else
							allGroupChecked = false;
					}
				}
				if ( !anyGroupChecked ) allGroupChecked = false;

				bool groupChecked = allGroupChecked;
				bool groupPartial = anyGroupChecked && !allGroupChecked;
				bool groupOpen = ImGui::TreeNodeEx( "##grpTree", ( !searchStr.isEmpty() ? ImGuiTreeNodeFlags_DefaultOpen : 0 ) | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth );
				ImGui::SameLine();
				if ( groupPartial ) { ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( 0.7f, 0.7f, 0.3f, 1.0f ) ); groupChecked = true; }
				if ( ImGui::Checkbox( group.toStdString().c_str(), &groupChecked ) )
				{
					bridge.cmdStockpileSetActive( bridge.activeStockpileID, groupPartial ? true : groupChecked, cat, group, "", "" );
				}
				if ( groupPartial ) ImGui::PopStyleColor();

				if ( groupOpen )
				{
					for ( const auto& item : filter.items( cat, group ) )
					{
						// Search filter at item level
						if ( !searchStr.isEmpty() && !item.toLower().contains( searchStr ) && !groupMatches ) continue;

						ImGui::PushID( item.toStdString().c_str() );

						auto mats = filter.materials( cat, group, item );
						if ( mats.size() <= 1 )
						{
							// Single material — just show checkbox
							QString mat = mats.isEmpty() ? "" : mats.first();
							bool checked = !mat.isEmpty() && filter.getCheckState( cat, group, item, mat );
							QString label = mat.isEmpty() ? item : item;
							if ( ImGui::Checkbox( label.toStdString().c_str(), &checked ) )
							{
								bridge.cmdStockpileSetActive( bridge.activeStockpileID, checked, cat, group, item, mat );
							}
						}
						else
						{
							// Multiple materials — item as tree node with checkbox
							bool allItemChecked = true;
							bool anyItemChecked = false;
							for ( const auto& mat : mats )
							{
								if ( filter.getCheckState( cat, group, item, mat ) )
									anyItemChecked = true;
								else
									allItemChecked = false;
							}
							if ( !anyItemChecked ) allItemChecked = false;

							bool itemChecked = allItemChecked;
							bool itemPartial = anyItemChecked && !allItemChecked;
							bool itemOpen = ImGui::TreeNodeEx( "##itemTree", ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth );
							ImGui::SameLine();
							if ( itemPartial ) { ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( 0.7f, 0.7f, 0.3f, 1.0f ) ); itemChecked = true; }
							if ( ImGui::Checkbox( item.toStdString().c_str(), &itemChecked ) )
							{
								// Toggle all materials for this item
								for ( const auto& mat : mats )
								{
									bridge.cmdStockpileSetActive( bridge.activeStockpileID, itemPartial ? true : itemChecked, cat, group, item, mat );
								}
							}
							if ( itemPartial ) ImGui::PopStyleColor();

							if ( itemOpen )
							{
								for ( const auto& mat : mats )
								{
									bool matChecked = filter.getCheckState( cat, group, item, mat );
									if ( ImGui::Checkbox( mat.toStdString().c_str(), &matChecked ) )
									{
										bridge.cmdStockpileSetActive( bridge.activeStockpileID, matChecked, cat, group, item, mat );
									}
								}
								ImGui::TreePop();
							}
						}

						ImGui::PopID();
					}
					ImGui::TreePop();
				}

				ImGui::PopID();
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::PopStyleVar(); // FramePadding
	ImGui::EndChild();

	ImGui::EndTabItem();
	} // end Filters tab

	// =========================================================================
	// TAB 3: All Stockpiles — priority ordering
	// =========================================================================
	if ( ImGui::BeginTabItem( "All Stockpiles" ) )
	{
		auto briefs = Global::eventConnector->aggregatorStockpile()->allStockpileBriefs();

		if ( briefs.isEmpty() )
		{
			ImGui::TextDisabled( "No stockpiles placed" );
		}
		else
		{
			ImGui::Text( "Reorder stockpile priorities:" );
			ImGui::BeginChild( "SPList", ImVec2( 0, 0 ), false );

			for ( int i = 0; i < briefs.size(); ++i )
			{
				auto& b = briefs[i];
				ImGui::PushID( (int)b.id );

				// Up/down buttons
				if ( i > 0 )
				{
					if ( ImGui::SmallButton( "^" ) )
					{
						bridge.cmdStockpileMovePriorityUp( b.id );
					}
				}
				else
				{
					ImGui::Dummy( ImVec2( ImGui::CalcTextSize( "^" ).x + ImGui::GetStyle().FramePadding.x * 2, 0 ) );
				}
				ImGui::SameLine();
				if ( i < briefs.size() - 1 )
				{
					if ( ImGui::SmallButton( "v" ) )
					{
						bridge.cmdStockpileMovePriorityDown( b.id );
					}
				}
				else
				{
					ImGui::Dummy( ImVec2( ImGui::CalcTextSize( "v" ).x + ImGui::GetStyle().FramePadding.x * 2, 0 ) );
				}

				ImGui::SameLine();
				bool isActive = ( b.id == bridge.activeStockpileID );
				if ( isActive ) ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 0.9f, 0.3f, 1.0f ) );
				ImGui::Text( "[%d] %s%s", b.priority, b.name.toStdString().c_str(), b.suspended ? " (suspended)" : "" );
				if ( isActive ) ImGui::PopStyleColor();

				ImGui::PopID();
			}

			ImGui::EndChild();
		}

		ImGui::EndTabItem();
	} // end All Stockpiles tab

	ImGui::EndTabBar();
	} // end TabBar

	ImGui::End();
}

// =============================================================================
// Population panel — helper: parse "R G B A" color string to ImVec4
// =============================================================================
static ImVec4 parseGroupColor( const QString& colorStr )
{
	auto parts = colorStr.split( ' ' );
	if ( parts.size() >= 3 )
	{
		float r = parts[0].toFloat() / 255.0f;
		float g = parts[1].toFloat() / 255.0f;
		float b = parts[2].toFloat() / 255.0f;
		float a = parts.size() >= 4 ? parts[3].toFloat() / 255.0f : 1.0f;
		return ImVec4( r, g, b, a );
	}
	return ImVec4( 0.6f, 0.6f, 0.6f, 1.0f );
}

// Cached group index built from skill data
struct SkillGroupIndex
{
	struct Group
	{
		QString id;
		QString name;
		ImVec4 color;
		QList<int> skillIndices; // indices into gnome.skills[]
	};
	QList<Group> groups;
	bool built = false;
};

static SkillGroupIndex s_groupIndex;

static void buildGroupIndex( const QList<GuiSkillInfo>& skills )
{
	s_groupIndex.groups.clear();
	QMap<QString, int> groupMap; // groupID -> index in groups list

	for ( int i = 0; i < skills.size(); ++i )
	{
		const auto& skill = skills[i];
		if ( !groupMap.contains( skill.group ) )
		{
			SkillGroupIndex::Group g;
			g.id = skill.group;
			g.name = skill.group; // use group ID as display name
			g.color = parseGroupColor( skill.color );
			groupMap[skill.group] = s_groupIndex.groups.size();
			s_groupIndex.groups.append( g );
		}
		s_groupIndex.groups[groupMap[skill.group]].skillIndices.append( i );
	}
	s_groupIndex.built = true;
}

// =============================================================================
// Population panel
// =============================================================================
void drawPopulationPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.82f, io.DisplaySize.y * 0.78f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Population", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	if ( ImGui::BeginTabBar( "PopTabs" ) )
	{
		// =================================================================
		// SKILLS TAB
		// =================================================================
		if ( ImGui::BeginTabItem( "Skills" ) )
		{
			if ( bridge.populationInfo.gnomes.isEmpty() || bridge.populationInfo.gnomes[0].skills.isEmpty() )
			{
				ImGui::TextDisabled( "No gnomes" );
				ImGui::EndTabItem();
			}
			else
			{
				// Build group index from first gnome's skills
				if ( !s_groupIndex.built )
				{
					buildGroupIndex( bridge.populationInfo.gnomes[0].skills );
				}

				// Toolbar
				if ( ImGui::Button( bridge.skillsShowIndividual ? "Group View" : "Individual View" ) )
				{
					bridge.skillsShowIndividual = !bridge.skillsShowIndividual;
				}

				ImGui::Separator();

				if ( bridge.skillsShowIndividual )
				{
					// ---- INDIVIDUAL SKILLS VIEW ----
					int numSkills = bridge.populationInfo.gnomes[0].skills.size();
					int numCols = numSkills + 2;

					if ( ImGui::BeginTable( "SkillsIndiv", numCols,
						ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
						ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit |
						ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable ) )
					{
						ImGui::TableSetupScrollFreeze( 2, 1 );
						ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide, 145.0f );
						ImGui::TableSetupColumn( "Prof", 0, 110.0f );

						for ( const auto& skill : bridge.populationInfo.gnomes[0].skills )
						{
							ImGui::TableSetupColumn( skill.name.left( 5 ).toStdString().c_str(), 0, 50.0f );
						}
						ImGui::TableHeadersRow();

						for ( const auto& gnome : bridge.populationInfo.gnomes )
						{
							ImGui::TableNextRow();
							ImGui::PushID( gnome.id );

							// Name
							ImGui::TableNextColumn();
							bool isSelected = ( bridge.selectedGnomeID == gnome.id );
							if ( ImGui::Selectable( gnome.name.toStdString().c_str(), isSelected ) )
							{
								bridge.selectedGnomeID = gnome.id;
							}

							// Profession combo
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth( -1 );
							if ( ImGui::BeginCombo( "##prof", gnome.profession.toStdString().c_str(), ImGuiComboFlags_NoArrowButton ) )
							{
								for ( const auto& prof : bridge.professionList )
								{
									bool sel = ( gnome.profession == prof );
									if ( ImGui::Selectable( prof.toStdString().c_str(), sel ) )
									{
										bridge.cmdSetProfession( gnome.id, prof );
									}
								}
								ImGui::EndCombo();
							}

							// Individual skill cells
							for ( int si = 0; si < gnome.skills.size(); ++si )
							{
								ImGui::TableNextColumn();
								const auto& skill = gnome.skills[si];
								ImGui::PushID( si );

								bool active = skill.active;
								if ( ImGui::Checkbox( "", &active ) )
								{
									bridge.cmdSetSkillActive( gnome.id, skill.sid, active );
								}
								if ( skill.level > 0 )
								{
									ImGui::SameLine();
									ImGui::Text( "%d", skill.level );
								}
								if ( ImGui::IsItemHovered() )
								{
									ImGui::SetTooltip( "%s: Level %d (XP: %.0f)", skill.name.toStdString().c_str(), skill.level, skill.xpValue );
								}
								ImGui::PopID();
							}

							ImGui::PopID();
						}
						ImGui::EndTable();
					}
				}
				else
				{
					// ---- GROUP VIEW (default, RimWorld-style) ----
					int numGroups = s_groupIndex.groups.size();
					int numCols = numGroups + 2;

					if ( ImGui::BeginTable( "SkillsGroup", numCols,
						ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
						ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit |
						ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable ) )
					{
						ImGui::TableSetupScrollFreeze( 2, 1 );
						ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide, 145.0f );
						ImGui::TableSetupColumn( "Prof", 0, 110.0f );

						for ( const auto& grp : s_groupIndex.groups )
						{
							ImGui::TableSetupColumn( grp.name.toStdString().c_str(), 0, 82.0f );
						}

						// Custom colored headers with click-to-sort
						ImGui::TableNextRow( ImGuiTableRowFlags_Headers );

						ImGui::TableNextColumn();
						if ( ImGui::Selectable( "Name", false, ImGuiSelectableFlags_None ) )
						{
							bridge.cmdSortGnomes( "Name" );
						}

						ImGui::TableNextColumn();
						if ( ImGui::Selectable( "Prof", false, ImGuiSelectableFlags_None ) )
						{
							bridge.cmdSortGnomes( "Prof" );
						}

						for ( int gi = 0; gi < numGroups; ++gi )
						{
							ImGui::TableNextColumn();
							const auto& grp = s_groupIndex.groups[gi];
							ImGui::PushStyleColor( ImGuiCol_Text, grp.color );
							if ( ImGui::Selectable( grp.name.toStdString().c_str(), false, ImGuiSelectableFlags_None ) )
							{
								bridge.cmdSortGnomes( grp.id );
							}
							ImGui::PopStyleColor();

							// Tooltip listing individual skills in group
							if ( ImGui::IsItemHovered() && !bridge.populationInfo.gnomes.isEmpty() )
							{
								ImGui::BeginTooltip();
								const auto& skills = bridge.populationInfo.gnomes[0].skills;
								for ( int idx : grp.skillIndices )
								{
									if ( idx < skills.size() )
										ImGui::Text( "%s", skills[idx].name.toStdString().c_str() );
								}
								ImGui::EndTooltip();
							}
						}

						// Gnome rows
						for ( const auto& gnome : bridge.populationInfo.gnomes )
						{
							ImGui::TableNextRow();
							ImGui::PushID( gnome.id );

							// Name column
							ImGui::TableNextColumn();
							bool isSelected = ( bridge.selectedGnomeID == gnome.id );
							if ( ImGui::Selectable( gnome.name.toStdString().c_str(), isSelected ) )
							{
								bridge.selectedGnomeID = gnome.id;
							}

							// Profession combo
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth( -1 );
							if ( ImGui::BeginCombo( "##prof", gnome.profession.toStdString().c_str(), ImGuiComboFlags_NoArrowButton ) )
							{
								for ( const auto& prof : bridge.professionList )
								{
									bool sel = ( gnome.profession == prof );
									if ( ImGui::Selectable( prof.toStdString().c_str(), sel ) )
									{
										bridge.cmdSetProfession( gnome.id, prof );
									}
								}
								ImGui::EndCombo();
							}

							// Group columns
							for ( int gi = 0; gi < numGroups; ++gi )
							{
								ImGui::TableNextColumn();
								const auto& grp = s_groupIndex.groups[gi];
								ImGui::PushID( gi );

								// Determine group state: any active? max level?
								bool anyActive = false;
								int maxLevel = 0;
								for ( int idx : grp.skillIndices )
								{
									if ( idx < gnome.skills.size() )
									{
										if ( gnome.skills[idx].active ) anyActive = true;
										if ( gnome.skills[idx].level > maxLevel ) maxLevel = gnome.skills[idx].level;
									}
								}

								// Checkbox for group toggle
								bool groupActive = anyActive;
								if ( ImGui::Checkbox( "", &groupActive ) )
								{
									bridge.cmdSetGroupActive( gnome.id, grp.id, groupActive );
								}

								// Level badge next to checkbox
								if ( maxLevel > 0 )
								{
									ImGui::SameLine();
									ImGui::Text( "%d", maxLevel );
								}

								// Tooltip with individual skills
								if ( ImGui::IsItemHovered() )
								{
									ImGui::BeginTooltip();
									ImGui::TextColored( grp.color, "%s", grp.name.toStdString().c_str() );
									ImGui::Separator();
									for ( int idx : grp.skillIndices )
									{
										if ( idx < gnome.skills.size() )
										{
											const auto& sk = gnome.skills[idx];
											ImGui::Text( "%s %s Lv %d (XP: %.0f)",
												sk.active ? "[x]" : "[ ]",
												sk.name.toStdString().c_str(),
												sk.level, sk.xpValue );
										}
									}
									ImGui::EndTooltip();
								}

								ImGui::PopID();
							}

							ImGui::PopID();
						}
						ImGui::EndTable();
					}
				}

				ImGui::EndTabItem();
			}
		}

		// =================================================================
		// SCHEDULE TAB
		// =================================================================
		if ( ImGui::BeginTabItem( "Schedule" ) )
		{
			if ( bridge.scheduleInfo.schedules.isEmpty() )
			{
				bridge.cmdRequestSchedules();
				ImGui::TextDisabled( "Loading schedules..." );
			}
			else
			{
				// Activity color/label helpers
				auto activityColor = []( ScheduleActivity a ) -> ImVec4 {
					switch ( a )
					{
						case ScheduleActivity::Anything: return ImVec4( 0.35f, 0.35f, 0.45f, 1.0f );
						case ScheduleActivity::None:     return ImVec4( 0.2f, 0.5f, 0.2f, 1.0f );
						case ScheduleActivity::Eat:      return ImVec4( 0.7f, 0.4f, 0.0f, 1.0f );
						case ScheduleActivity::Sleep:    return ImVec4( 0.0f, 0.3f, 0.7f, 1.0f );
						case ScheduleActivity::Training: return ImVec4( 0.7f, 0.0f, 0.0f, 1.0f );
						default: return ImVec4( 0.3f, 0.3f, 0.3f, 1.0f );
					}
				};
				auto activityLabel = []( ScheduleActivity a ) -> const char* {
					switch ( a )
					{
						case ScheduleActivity::Anything: return "A";
						case ScheduleActivity::None:     return "W";
						case ScheduleActivity::Eat:      return "E";
						case ScheduleActivity::Sleep:    return "S";
						case ScheduleActivity::Training: return "T";
						default: return "?";
					}
				};

				// Paint brush selector toolbar
				ImGui::Text( "Paint:" );
				ImGui::SameLine();

				struct BrushOption { ScheduleActivity act; const char* label; const char* tooltip; };
				BrushOption brushes[] = {
					{ ScheduleActivity::Anything, "Anything",  "Self-manage: eat/sleep when needed, work otherwise" },
					{ ScheduleActivity::None,     "Work",      "Work only — ignore needs until critical" },
					{ ScheduleActivity::Eat,      "Eat",       "Proactively eat and drink" },
					{ ScheduleActivity::Sleep,    "Sleep",     "Go to bed" },
					{ ScheduleActivity::Training, "Train",     "Train at training ground" },
				};

				for ( const auto& b : brushes )
				{
					bool selected = ( bridge.schedulePaintBrush == b.act );
					ImVec4 col = activityColor( b.act );
					if ( selected )
					{
						col.x = qMin( col.x + 0.3f, 1.0f );
						col.y = qMin( col.y + 0.3f, 1.0f );
						col.z = qMin( col.z + 0.3f, 1.0f );
					}
					ImGui::PushStyleColor( ImGuiCol_Button, selected ? col : activityColor( b.act ) );
					ImGui::PushStyleColor( ImGuiCol_ButtonHovered, col );
					if ( ImGui::Button( b.label ) )
					{
						bridge.schedulePaintBrush = b.act;
					}
					ImGui::PopStyleColor( 2 );
					if ( ImGui::IsItemHovered() )
					{
						ImGui::SetTooltip( "%s", b.tooltip );
					}
					ImGui::SameLine();
				}
				ImGui::NewLine();
				ImGui::Spacing();

				if ( ImGui::BeginTable( "ScheduleTable", 26,
					ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
					ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit |
					ImGuiTableFlags_RowBg ) )
				{
					ImGui::TableSetupScrollFreeze( 1, 1 );
					ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide, 145.0f );
					for ( int h = 0; h < 24; ++h )
					{
						char hdr[4];
						snprintf( hdr, sizeof( hdr ), "%02d", h );
						ImGui::TableSetupColumn( hdr, 0, 36.0f );
					}
					ImGui::TableSetupColumn( "All", 0, 40.0f );
					ImGui::TableHeadersRow();

					for ( const auto& gnome : bridge.scheduleInfo.schedules )
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text( "%s", gnome.name.toStdString().c_str() );

						for ( int h = 0; h < 24; ++h )
						{
							ImGui::TableNextColumn();
							ImGui::PushID( gnome.id * 100 + h );

							ScheduleActivity act = ( h < gnome.schedule.size() ) ? gnome.schedule[h] : ScheduleActivity::Anything;
							ImVec4 col = activityColor( act );
							const char* lbl = activityLabel( act );

							// Draw colored cell button
							ImGui::PushStyleColor( ImGuiCol_Button, col );
							ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( col.x + 0.15f, col.y + 0.15f, col.z + 0.15f, 1.0f ) );
							ImGui::SmallButton( lbl );
							ImGui::PopStyleColor( 2 );

							// Click to paint single cell
							if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) )
							{
								bridge.cmdSetSchedule( gnome.id, h, bridge.schedulePaintBrush );
							}

							// Drag painting: if mouse is held and hovering this cell, paint it
							if ( ImGui::IsItemHovered() && ImGui::IsMouseDown( ImGuiMouseButton_Left ) && act != bridge.schedulePaintBrush )
							{
								bridge.cmdSetSchedule( gnome.id, h, bridge.schedulePaintBrush );
							}

							ImGui::PopID();
						}

						// "All" column: set all hours for this gnome to paint brush
						ImGui::TableNextColumn();
						ImGui::PushID( gnome.id * 100 + 99 );
						ImGui::PushStyleColor( ImGuiCol_Button, activityColor( bridge.schedulePaintBrush ) );
						if ( ImGui::SmallButton( activityLabel( bridge.schedulePaintBrush ) ) )
						{
							bridge.cmdSetAllHours( gnome.id, bridge.schedulePaintBrush );
						}
						ImGui::PopStyleColor();
						if ( ImGui::IsItemHovered() )
						{
							ImGui::SetTooltip( "Set all hours to selected activity" );
						}
						ImGui::PopID();
					}

					// "All gnomes" row at bottom
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextDisabled( "All" );
					for ( int h = 0; h < 24; ++h )
					{
						ImGui::TableNextColumn();
						ImGui::PushID( 999900 + h );
						ImGui::PushStyleColor( ImGuiCol_Button, activityColor( bridge.schedulePaintBrush ) );
						if ( ImGui::SmallButton( activityLabel( bridge.schedulePaintBrush ) ) )
						{
							bridge.cmdSetHourForAll( h, bridge.schedulePaintBrush );
						}
						ImGui::PopStyleColor();
						ImGui::PopID();
					}
					// "All" column in "All" row
					ImGui::TableNextColumn();

					ImGui::EndTable();
				}
			}

			ImGui::EndTabItem();
		}

		// =================================================================
		// PERSONALITY TAB (unchanged)
		// =================================================================
		if ( ImGui::BeginTabItem( "Personality" ) )
		{
			for ( const auto& gnome : bridge.populationInfo.gnomes )
			{
				if ( ImGui::TreeNode( gnome.name.toStdString().c_str() ) )
				{
					// Mood bar at top
					ImVec4 moodColor;
					if ( gnome.mood > 65 )
						moodColor = ImVec4( 0.2f, 0.7f, 0.3f, 1.0f );
					else if ( gnome.mood > 35 )
						moodColor = ImVec4( 0.7f, 0.7f, 0.2f, 1.0f );
					else if ( gnome.mood > 15 )
						moodColor = ImVec4( 0.8f, 0.4f, 0.1f, 1.0f );
					else
						moodColor = ImVec4( 0.8f, 0.1f, 0.1f, 1.0f );

					ImGui::PushStyleColor( ImGuiCol_PlotHistogram, moodColor );
					ImGui::ProgressBar( gnome.mood / 100.0f, ImVec2( 200, 16 ), "" );
					ImGui::PopStyleColor();
					ImGui::SameLine();
					if ( gnome.mentalBreak )
						ImGui::TextColored( ImVec4( 1.0f, 0.2f, 0.2f, 1.0f ), "MENTAL BREAK! Mood: %d", gnome.mood );
					else
						ImGui::Text( "Mood: %d/100", gnome.mood );

					// Active thoughts
					if ( !gnome.thoughts.isEmpty() )
					{
						ImGui::Indent( 10.0f );
						for ( const auto& thought : gnome.thoughts )
						{
							ImVec4 tColor = thought.moodValue > 0 ?
								ImVec4( 0.3f, 0.7f, 0.3f, 1.0f ) :
								ImVec4( 0.7f, 0.3f, 0.3f, 1.0f );
							ImGui::TextColored( tColor, "%s (%+d)", thought.text.toStdString().c_str(), thought.moodValue );
						}
						ImGui::Unindent( 10.0f );
					}
					ImGui::Spacing();

					// Backstory section
					if ( !gnome.childhood.title.isEmpty() || !gnome.adulthood.title.isEmpty() )
					{
						ImGui::TextColored( ImVec4( 0.7f, 0.85f, 1.0f, 1.0f ), "Backstory" );
						ImGui::Indent( 10.0f );
						if ( !gnome.childhood.title.isEmpty() )
						{
							ImGui::Text( "Youth: %s", gnome.childhood.title.toStdString().c_str() );
							if ( ImGui::IsItemHovered() && !gnome.childhood.description.isEmpty() )
							{
								ImGui::SetTooltip( "%s", gnome.childhood.description.toStdString().c_str() );
							}
						}
						if ( !gnome.adulthood.title.isEmpty() )
						{
							ImGui::Text( "Before: %s", gnome.adulthood.title.toStdString().c_str() );
							if ( ImGui::IsItemHovered() && !gnome.adulthood.description.isEmpty() )
							{
								ImGui::SetTooltip( "%s", gnome.adulthood.description.toStdString().c_str() );
							}
						}
						ImGui::Unindent( 10.0f );
						ImGui::Spacing();
					}

					// Traits section
					ImGui::TextColored( ImVec4( 0.7f, 0.85f, 1.0f, 1.0f ), "Personality Traits" );
					ImGui::Indent( 10.0f );
					bool hasNotable = false;
					for ( const auto& trait : gnome.traits )
					{
						if ( trait.label.isEmpty() )
							continue;

						hasNotable = true;
						float barFrac = ( trait.value + 50.0f ) / 100.0f;
						ImVec4 barColor;
						if ( trait.value > 0 )
							barColor = ImVec4( 0.2f, 0.6f, 0.3f, 1.0f );
						else
							barColor = ImVec4( 0.7f, 0.3f, 0.2f, 1.0f );

						ImGui::PushStyleColor( ImGuiCol_PlotHistogram, barColor );
						ImGui::ProgressBar( barFrac, ImVec2( 120, 14 ), "" );
						ImGui::PopStyleColor();
						ImGui::SameLine();
						ImGui::Text( "%s", trait.label.toStdString().c_str() );
						if ( ImGui::IsItemHovered() && !trait.description.isEmpty() )
						{
							ImGui::SetTooltip( "[%s] %s (%+d)", trait.id.toStdString().c_str(),
								trait.description.toStdString().c_str(), trait.value );
						}
					}
					if ( !hasNotable )
					{
						ImGui::TextDisabled( "Unremarkable personality" );
					}
					ImGui::Unindent( 10.0f );

					ImGui::TreePop();
					ImGui::Separator();
				}
			}
			if ( bridge.populationInfo.gnomes.isEmpty() )
			{
				ImGui::TextDisabled( "No gnomes" );
			}
			ImGui::EndTabItem();
		}

		// =================================================================
		// SOCIAL TAB
		// =================================================================
		if ( ImGui::BeginTabItem( "Social" ) )
		{
			// Summary
			int friendships = 0, rivalries = 0, withRels = 0;
			for ( const auto& gnome : bridge.populationInfo.gnomes )
			{
				if ( !gnome.relationships.isEmpty() ) ++withRels;
				for ( const auto& rel : gnome.relationships )
				{
					if ( rel.opinion > 20 ) ++friendships;
					else if ( rel.opinion < -20 ) ++rivalries;
				}
			}
			if ( withRels > 0 )
			{
				ImGui::Text( "%d gnomes with relationships | %d friendships | %d rivalries", withRels, friendships, rivalries );
				ImGui::Separator();
			}

			// Show all gnomes, even those without relationships
			for ( const auto& gnome : bridge.populationInfo.gnomes )
			{
				if ( ImGui::TreeNode( gnome.name.toStdString().c_str() ) )
				{
					if ( gnome.relationships.isEmpty() )
					{
						ImGui::TextDisabled( "No notable relationships" );
					}
					else
					{
						for ( const auto& rel : gnome.relationships )
						{
							ImVec4 color;
							if ( rel.opinion > 20 )
								color = ImVec4( 0.2f, 0.7f, 0.3f, 1.0f );
							else if ( rel.opinion < -20 )
								color = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f );
							else
								color = ImVec4( 0.7f, 0.7f, 0.3f, 1.0f );

							ImGui::TextColored( color, "%s", rel.label.toStdString().c_str() );
							ImGui::SameLine();
							ImGui::Text( " %s (%+d)", rel.name.toStdString().c_str(), rel.opinion );
						}
					}
					ImGui::TreePop();
					ImGui::Separator();
				}
			}

			if ( bridge.populationInfo.gnomes.isEmpty() )
			{
				ImGui::TextDisabled( "No gnomes" );
			}
			else if ( withRels == 0 )
			{
				ImGui::Spacing();
				ImGui::TextDisabled( "No relationships yet -- gnomes develop opinions over time" );
			}
			ImGui::EndTabItem();
		}

		// =================================================================
		// PROFESSIONS TAB
		// =================================================================
		if ( ImGui::BeginTabItem( "Professions" ) )
		{
			if ( bridge.professionList.isEmpty() )
			{
				bridge.cmdRequestProfessions();
			}

			// Left panel: profession list
			ImGui::BeginChild( "ProfList", ImVec2( 200, 0 ), true );
			{
				if ( ImGui::Button( "New", ImVec2( 90, 0 ) ) )
				{
					bridge.cmdNewProfession();
					bridge.cmdRequestProfessions();
				}
				ImGui::SameLine();
				if ( ImGui::Button( "Delete", ImVec2( 90, 0 ) ) )
				{
					if ( !bridge.editingProfession.isEmpty() )
					{
						bridge.cmdDeleteProfession( bridge.editingProfession );
						bridge.editingProfession.clear();
						bridge.professionSkills.clear();
					}
				}
				ImGui::Separator();

				for ( const auto& prof : bridge.professionList )
				{
					if ( ImGui::Selectable( prof.toStdString().c_str(), bridge.editingProfession == prof ) )
					{
						bridge.cmdRequestSkills( prof );
					}
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			// Right panel: skill editor
			ImGui::BeginChild( "ProfEdit", ImVec2( 0, 0 ), true );
			{
				if ( !bridge.editingProfession.isEmpty() && !bridge.professionSkills.isEmpty() )
				{
					// Rename field
					static char renameBuf[128] = {};
					static QString lastProf;
					if ( lastProf != bridge.editingProfession )
					{
						lastProf = bridge.editingProfession;
						strncpy( renameBuf, bridge.editingProfession.toStdString().c_str(), sizeof( renameBuf ) - 1 );
						renameBuf[sizeof( renameBuf ) - 1] = '\0';
					}
					ImGui::Text( "Editing:" );
					ImGui::SameLine();
					ImGui::SetNextItemWidth( 200 );
					if ( ImGui::InputText( "##profname", renameBuf, sizeof( renameBuf ), ImGuiInputTextFlags_EnterReturnsTrue ) )
					{
						QString newName = QString( renameBuf );
						if ( !newName.isEmpty() && newName != bridge.editingProfession )
						{
							QStringList activeSkills;
							for ( const auto& s : bridge.professionSkills )
							{
								if ( s.active ) activeSkills.append( s.sid );
							}
							bridge.cmdUpdateProfession( bridge.editingProfession, newName, activeSkills );
							bridge.editingProfession = newName;
							bridge.cmdRequestProfessions();
						}
					}

					// Assign to selected gnome
					if ( bridge.selectedGnomeID != 0 )
					{
						ImGui::SameLine();
						if ( ImGui::Button( "Assign to Selected Gnome" ) )
						{
							bridge.cmdSetProfession( bridge.selectedGnomeID, bridge.editingProfession );
						}
					}

					ImGui::Separator();

					// Skills grouped by group
					if ( s_groupIndex.built )
					{
						for ( const auto& grp : s_groupIndex.groups )
						{
							ImGui::PushStyleColor( ImGuiCol_Text, grp.color );
							bool groupOpen = ImGui::TreeNodeEx( grp.name.toStdString().c_str(), ImGuiTreeNodeFlags_DefaultOpen );
							ImGui::PopStyleColor();

							if ( groupOpen )
							{
								for ( int idx : grp.skillIndices )
								{
									if ( idx < bridge.professionSkills.size() )
									{
										auto& skill = bridge.professionSkills[idx];
										ImGui::PushID( idx );
										bool active = skill.active;
										if ( ImGui::Checkbox( skill.name.toStdString().c_str(), &active ) )
										{
											skill.active = active;
											QStringList activeSkills;
											for ( const auto& s : bridge.professionSkills )
											{
												if ( s.active ) activeSkills.append( s.sid );
											}
											bridge.cmdUpdateProfession( bridge.editingProfession, bridge.editingProfession, activeSkills );
										}
										ImGui::PopID();
									}
								}
								ImGui::TreePop();
							}
						}
					}
					else
					{
						// Fallback: flat list
						for ( auto& skill : bridge.professionSkills )
						{
							bool active = skill.active;
							if ( ImGui::Checkbox( skill.name.toStdString().c_str(), &active ) )
							{
								skill.active = active;
								QStringList activeSkills;
								for ( const auto& s : bridge.professionSkills )
								{
									if ( s.active ) activeSkills.append( s.sid );
								}
								bridge.cmdUpdateProfession( bridge.editingProfession, bridge.editingProfession, activeSkills );
							}
						}
					}
				}
				else
				{
					ImGui::TextDisabled( "Select a profession to edit" );
				}
			}
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// =============================================================================
// Military panel
// =============================================================================
void drawMilitaryPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 550, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Military", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	static const char* attitudeNames[] = { "Flee", "Defend", "Attack", "Hunt" };

	if ( ImGui::BeginTabBar( "MilTabs" ) )
	{
		// =================================================================
		// SQUADS TAB
		// =================================================================
		if ( ImGui::BeginTabItem( "Squads" ) )
		{
			if ( ImGui::Button( "Add Squad" ) )
			{
				bridge.cmdAddSquad();
				bridge.cmdRequestMilitaryUpdate();
			}

			// Get unassigned gnomes (squad index 0 = "no squad")
			QList<GuiSquadGnome> unassigned;
			if ( !bridge.squads.isEmpty() && bridge.squads[0].id == 0 )
			{
				unassigned = bridge.squads[0].gnomes;
			}

			// Skip index 0 (the "no squad" virtual squad) — show real squads
			for ( int si = 1; si < bridge.squads.size(); ++si )
			{
				const auto& squad = bridge.squads[si];
				ImGui::PushID( squad.id );

				QString headerLabel = squad.name + " (" + QString::number( squad.gnomes.size() ) + " gnomes)";
				if ( ImGui::CollapsingHeader( headerLabel.toStdString().c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
				{
					// Rename
					static char squadName[128];
					snprintf( squadName, sizeof( squadName ), "%s", squad.name.toStdString().c_str() );
					ImGui::PushItemWidth( 200 );
					if ( ImGui::InputText( "##SquadName", squadName, sizeof( squadName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
					{
						bridge.cmdRenameSquad( squad.id, squadName );
						bridge.cmdRequestMilitaryUpdate();
					}
					ImGui::PopItemWidth();

					// --- Gnome list with combat stats ---
					if ( !squad.gnomes.isEmpty() )
					{
						ImGui::TextColored( ImVec4( 0.6f, 0.7f, 0.9f, 1.0f ), "Members:" );
						for ( const auto& gnome : squad.gnomes )
						{
							ImGui::PushID( (int)gnome.id );
							ImGui::Text( "  %s", gnome.name.toStdString().c_str() );
							ImGui::SameLine();
							ImGui::TextDisabled( "Mel:%d Dge:%d Arm:%d", gnome.meleeSkill, gnome.dodgeSkill, gnome.armorSkill );
							ImGui::SameLine();
							ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.5f, 1.0f ), "[%s]", gnome.weapon.toStdString().c_str() );
							ImGui::SameLine();
							if ( ImGui::SmallButton( "X" ) )
							{
								bridge.cmdRemoveGnomeFromSquad( gnome.id );
								bridge.cmdRequestMilitaryUpdate();
							}
							if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "Remove from squad" );
							ImGui::PopID();
						}
					}

					// --- Add gnome dropdown ---
					if ( !unassigned.isEmpty() )
					{
						ImGui::PushItemWidth( 200 );
						if ( ImGui::BeginCombo( "##AddGnome", "+ Add Gnome..." ) )
						{
							for ( const auto& g : unassigned )
							{
								QString label = g.name + " (Mel:" + QString::number( g.meleeSkill ) + ")";
								if ( ImGui::Selectable( label.toStdString().c_str() ) )
								{
									bridge.cmdAddGnomeToSquad( g.id, squad.id );
									bridge.cmdRequestMilitaryUpdate();
								}
							}
							ImGui::EndCombo();
						}
						ImGui::PopItemWidth();
					}
					else
					{
						ImGui::TextDisabled( "All gnomes assigned" );
					}

					ImGui::Separator();

					// --- Uniform Configuration ---
					if ( !squad.uniform.isEmpty() )
					{
						ImGui::TextColored( ImVec4( 0.7f, 0.9f, 0.5f, 1.0f ), "Uniform:" );
						for ( const auto& slot : squad.uniform )
						{
							ImGui::PushID( slot.slotName.toStdString().c_str() );
							ImGui::Text( "  %s:", slot.slotName.toStdString().c_str() );
							ImGui::SameLine( 130 );

							int typeIdx = slot.possibleTypesForSlot.indexOf( slot.armorType );
							if ( typeIdx < 0 ) typeIdx = 0;

							ImGui::PushItemWidth( 150 );
							if ( ImGui::BeginCombo( "##type", slot.armorType.toStdString().c_str() ) )
							{
								for ( int t = 0; t < slot.possibleTypesForSlot.size(); ++t )
								{
									bool sel = ( t == typeIdx );
									if ( ImGui::Selectable( slot.possibleTypesForSlot[t].toStdString().c_str(), sel ) )
									{
										bridge.cmdSetArmorType( squad.id, slot.slotName, slot.possibleTypesForSlot[t], "any" );
										bridge.cmdRequestMilitaryUpdate();
									}
								}
								ImGui::EndCombo();
							}
							ImGui::PopItemWidth();

							ImGui::PopID();
						}
					}

					ImGui::Separator();

					// --- Target Priorities ---
					if ( !squad.priorities.isEmpty() )
					{
						ImGui::TextColored( ImVec4( 0.9f, 0.7f, 0.4f, 1.0f ), "Target Priorities:" );
						for ( int pi = 0; pi < squad.priorities.size(); ++pi )
						{
							const auto& prio = squad.priorities[pi];
							ImGui::PushID( pi );

							// Creature name
							ImGui::Text( "  %s", prio.name.toStdString().c_str() );
							ImGui::SameLine( 160 );

							// Attitude dropdown
							int att = (int)prio.attitude;
							ImGui::PushItemWidth( 110 );
							if ( ImGui::Combo( "##att", &att, attitudeNames, 4 ) )
							{
								bridge.cmdSetAttitude( squad.id, prio.id, att );
								bridge.cmdRequestMilitaryUpdate();
							}
							ImGui::PopItemWidth();

							// Priority reorder buttons
							ImGui::SameLine();
							if ( pi > 0 )
							{
								if ( ImGui::SmallButton( "^" ) )
								{
									bridge.cmdMovePrioUp( squad.id, prio.id );
									bridge.cmdRequestMilitaryUpdate();
								}
							}
							else
							{
								ImGui::TextDisabled( " " );
							}
							ImGui::SameLine();
							if ( pi < squad.priorities.size() - 1 )
							{
								if ( ImGui::SmallButton( "v" ) )
								{
									bridge.cmdMovePrioDown( squad.id, prio.id );
									bridge.cmdRequestMilitaryUpdate();
								}
							}

							ImGui::PopID();
						}
					}

					ImGui::Separator();

					ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.15f, 0.15f, 1.0f ) );
					if ( ImGui::SmallButton( "Remove Squad" ) )
					{
						bridge.cmdRemoveSquad( squad.id );
						bridge.cmdRequestMilitaryUpdate();
					}
					ImGui::PopStyleColor();
				}

				ImGui::PopID();
			}

			// Show unassigned gnome count
			if ( !unassigned.isEmpty() )
			{
				ImGui::Separator();
				ImGui::TextDisabled( "%d gnomes unassigned", unassigned.size() );
			}

			ImGui::EndTabItem();
		}

		// =================================================================
		// ROLES TAB
		// =================================================================
		if ( ImGui::BeginTabItem( "Uniforms" ) )
		{
			if ( bridge.roles.isEmpty() )
			{
				bridge.cmdRequestRoles();
			}

			if ( ImGui::Button( "Add Role" ) )
			{
				bridge.cmdAddRole();
				bridge.cmdRequestRoles();
			}

			for ( const auto& role : bridge.roles )
			{
				ImGui::PushID( role.id );
				if ( ImGui::CollapsingHeader( role.name.toStdString().c_str() ) )
				{
					// Rename
					static char roleName[128];
					snprintf( roleName, sizeof( roleName ), "%s", role.name.toStdString().c_str() );
					ImGui::PushItemWidth( 200 );
					if ( ImGui::InputText( "##RoleName", roleName, sizeof( roleName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
					{
						bridge.cmdRenameRole( role.id, roleName );
						bridge.cmdRequestRoles();
					}
					ImGui::PopItemWidth();

					bool civ = role.isCivilian;
					if ( ImGui::Checkbox( "Civilian", &civ ) )
					{
						bridge.cmdSetRoleCivilian( role.id, civ );
					}

					// Uniform configuration (only if not civilian)
					if ( !civ && !role.uniform.isEmpty() )
					{
						ImGui::TextColored( ImVec4( 0.6f, 0.7f, 0.9f, 1.0f ), "Uniform:" );
						for ( const auto& slot : role.uniform )
						{
							ImGui::PushID( slot.slotName.toStdString().c_str() );
							ImGui::Text( "  %s:", slot.slotName.toStdString().c_str() );
							ImGui::SameLine( 100 );

							// Armor type dropdown
							int typeIdx = slot.possibleTypesForSlot.indexOf( slot.armorType );
							if ( typeIdx < 0 ) typeIdx = 0;

							ImGui::PushItemWidth( 150 );
							if ( ImGui::BeginCombo( "##type", slot.armorType.toStdString().c_str() ) )
							{
								for ( int t = 0; t < slot.possibleTypesForSlot.size(); ++t )
								{
									bool sel = ( t == typeIdx );
									if ( ImGui::Selectable( slot.possibleTypesForSlot[t].toStdString().c_str(), sel ) )
									{
										bridge.cmdSetArmorType( role.id, slot.slotName, slot.possibleTypesForSlot[t], "any" );
										bridge.cmdRequestRoles();
									}
								}
								ImGui::EndCombo();
							}
							ImGui::PopItemWidth();

							ImGui::PopID();
						}
					}

					ImGui::Separator();
					ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.15f, 0.15f, 1.0f ) );
					if ( ImGui::SmallButton( "Remove Role" ) )
					{
						bridge.cmdRemoveRole( role.id );
						bridge.cmdRequestRoles();
					}
					ImGui::PopStyleColor();
				}
				ImGui::PopID();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// =============================================================================
// Neighbors panel
// =============================================================================
void drawNeighborsPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.65f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Neighbors & Missions", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	if ( ImGui::BeginTabBar( "NeighborTabs" ) )
	{
		if ( ImGui::BeginTabItem( "Neighbors" ) )
		{
			for ( const auto& n : bridge.neighbors )
			{
				ImGui::PushID( n.id );
				if ( ImGui::CollapsingHeader( n.name.toStdString().c_str(), ImGuiTreeNodeFlags_DefaultOpen ) )
				{
					ImGui::Text( "Distance: %s", n.distance.toStdString().c_str() );
					ImGui::Text( "Wealth: %s", n.wealth.toStdString().c_str() );
					ImGui::Text( "Economy: %s", n.economy.toStdString().c_str() );
					ImGui::Text( "Military: %s", n.military.toStdString().c_str() );
					ImGui::Text( "Attitude: %s", n.attitude.toStdString().c_str() );

					if ( ImGui::Button( "Send Ambassador" ) )
					{
						bridge.cmdRequestAvailableGnomes();
						// TODO: Show gnome selection dialog
					}
				}
				ImGui::PopID();
			}

			if ( bridge.neighbors.isEmpty() )
			{
				ImGui::TextDisabled( "No neighbors discovered" );
			}

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Missions" ) )
		{
			if ( bridge.missions.isEmpty() )
			{
				ImGui::TextDisabled( "No active missions" );
			}
			else
			{
				for ( const auto& m : bridge.missions )
				{
					ImGui::Text( "Mission #%u (distance: %d days)", m.id, m.distance );
				}
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// =============================================================================
// Workshop panel
// =============================================================================
// Track selected material index per craft component
static QMap<QString, QMap<int, int>> s_craftSelectedMats;
static int s_craftMode = 0;      // 0=CraftNumber, 1=CraftTo, 2=Repeat
static int s_craftAmount = 1;

// Smart item name lookup: tries $ItemName_X, falls back to readable SID
static QString itemName( const QString& sid )
{
	QString name = S::s( "$ItemName_" + sid );
	if ( !name.isEmpty() && !name.startsWith( "Error" ) && !name.startsWith( "$" ) )
		return name;
	// Insert spaces before capitals: "StoneChair" -> "Stone Chair"
	QString readable;
	for ( int i = 0; i < sid.size(); ++i )
	{
		if ( i > 0 && sid[i].isUpper() && !sid[i - 1].isUpper() )
			readable += ' ';
		readable += sid[i];
	}
	return readable.toLower();
}

static QString materialName( const QString& sid )
{
	QString name = S::s( "$MaterialName_" + sid );
	if ( !name.isEmpty() && !name.startsWith( "Error" ) && !name.startsWith( "$" ) )
		return name;
	return sid;
}

void drawWorkshopPanel( ImGuiBridge& bridge )
{
	if ( !bridge.showWorkshopWindow )
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 130 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 440, io.DisplaySize.y - 200 ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Workshop", &open );

	if ( !open ) bridge.cmdCloseWorkshopWindow();

	auto& ws = bridge.workshopInfo;

	// Header: name + priority
	static char wsName[128];
	snprintf( wsName, sizeof( wsName ), "%s", ws.name.toStdString().c_str() );
	ImGui::SetNextItemWidth( 200 );
	if ( ImGui::InputText( "Name", wsName, sizeof( wsName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
	{
		bridge.cmdWorkshopSetOptions( bridge.activeWorkshopID, wsName, ws.priority, ws.suspended, ws.acceptGenerated, ws.autoCraftMissing, ws.linkStockpile );
	}

	int prio = ws.priority;
	ImGui::SetNextItemWidth( 150 );
	if ( ImGui::SliderInt( "Priority", &prio, 0, 9 ) )
	{
		bridge.cmdWorkshopSetOptions( bridge.activeWorkshopID, ws.name, prio, ws.suspended, ws.acceptGenerated, ws.autoCraftMissing, ws.linkStockpile );
	}

	// Toggles row
	bool suspended = ws.suspended;
	if ( ImGui::Checkbox( "Suspended", &suspended ) )
	{
		bridge.cmdWorkshopSetOptions( bridge.activeWorkshopID, ws.name, ws.priority, suspended, ws.acceptGenerated, ws.autoCraftMissing, ws.linkStockpile );
	}
	ImGui::SameLine();
	bool acceptGen = ws.acceptGenerated;
	if ( ImGui::Checkbox( "Accept generated", &acceptGen ) )
	{
		bridge.cmdWorkshopSetOptions( bridge.activeWorkshopID, ws.name, ws.priority, ws.suspended, acceptGen, ws.autoCraftMissing, ws.linkStockpile );
	}
	ImGui::SameLine();
	bool autoCraft = ws.autoCraftMissing;
	if ( ImGui::Checkbox( "Auto-craft", &autoCraft ) )
	{
		bridge.cmdWorkshopSetOptions( bridge.activeWorkshopID, ws.name, ws.priority, ws.suspended, ws.acceptGenerated, autoCraft, ws.linkStockpile );
	}

	// Special options: Butcher
	if ( ws.gui == "Butcher" )
	{
		ImGui::Separator();
		bool corpses = ws.butcherCorpses;
		bool excess = ws.butcherExcess;
		if ( ImGui::Checkbox( "Butcher corpses", &corpses ) )
			bridge.cmdWorkshopSetButcherOptions( bridge.activeWorkshopID, corpses, ws.butcherExcess );
		ImGui::SameLine();
		if ( ImGui::Checkbox( "Butcher excess", &excess ) )
			bridge.cmdWorkshopSetButcherOptions( bridge.activeWorkshopID, ws.butcherCorpses, excess );
	}

	// Special options: Fishery
	if ( ws.gui == "Fishery" )
	{
		ImGui::Separator();
		bool catchF = ws.catchFish;
		bool processF = ws.processFish;
		if ( ImGui::Checkbox( "Catch fish", &catchF ) )
			bridge.cmdWorkshopSetFisherOptions( bridge.activeWorkshopID, catchF, ws.processFish );
		ImGui::SameLine();
		if ( ImGui::Checkbox( "Process fish", &processF ) )
			bridge.cmdWorkshopSetFisherOptions( bridge.activeWorkshopID, ws.catchFish, processF );
	}

	ImGui::Separator();

	if ( ImGui::BeginTabBar( "WorkshopTabs" ) )
	{
		// =====================================================================
		// Craft Queue tab
		// =====================================================================
		if ( ImGui::BeginTabItem( "Queue" ) )
		{
			if ( ws.jobList.isEmpty() )
			{
				ImGui::TextDisabled( "No craft jobs queued" );
			}
			else
			{
				for ( const auto& job : ws.jobList )
				{
					ImGui::PushID( (int)job.id );

					// Job status line
					const char* modeLabel = "?";
					switch ( job.mode )
					{
						case CraftMode::CraftNumber:      modeLabel = "Craft"; break;
						case CraftMode::CraftTo:          modeLabel = "Until"; break;
						case CraftMode::CraftUntilStock:  modeLabel = "Stock"; break;
						case CraftMode::Repeat:           modeLabel = "Repeat"; break;
					}

					if ( job.mode == CraftMode::Repeat )
						ImGui::Text( "%s %s", modeLabel, job.itemSID.toStdString().c_str() );
					else
						ImGui::Text( "%s %d x %s (%d done)", modeLabel, job.numItemsToCraft, job.itemSID.toStdString().c_str(), job.alreadyCrafted );

					// Controls
					ImGui::SameLine();
					if ( ImGui::SmallButton( "^" ) ) bridge.cmdWorkshopCraftJobCommand( job.id, "Up" );
					ImGui::SameLine();
					if ( ImGui::SmallButton( "v" ) ) bridge.cmdWorkshopCraftJobCommand( job.id, "Down" );
					ImGui::SameLine();
					if ( ImGui::SmallButton( "X" ) ) bridge.cmdWorkshopCraftJobCommand( job.id, "Cancel" );

					ImGui::PopID();
				}
			}

			ImGui::EndTabItem();
		}

		// =====================================================================
		// Craft Recipes tab
		// =====================================================================
		if ( ImGui::BeginTabItem( "Recipes" ) )
		{
			if ( ws.products.isEmpty() )
			{
				ImGui::TextDisabled( "No recipes available" );
			}
			else
			{
				// Craft mode selector
				ImGui::Text( "Mode:" );
				ImGui::SameLine();
				ImGui::RadioButton( "Craft N", &s_craftMode, 0 );
				ImGui::SameLine();
				ImGui::RadioButton( "Until N", &s_craftMode, 1 );
				ImGui::SameLine();
				ImGui::RadioButton( "Repeat", &s_craftMode, 2 );

				// Amount selector with proper width for double digits
				if ( s_craftMode != 2 )
				{
					ImGui::SameLine( 0, 15 );
					ImGui::SetNextItemWidth( 100 );
					ImGui::InputInt( "##amount", &s_craftAmount, 1, 5 );
					if ( s_craftAmount < 1 ) s_craftAmount = 1;
					if ( s_craftAmount > 999 ) s_craftAmount = 999;
				}

				// Search field
				static char recipeSearch[64] = "";
				ImGui::SetNextItemWidth( -1 );
				ImGui::InputTextWithHint( "##recipesearch", "Search recipes...", recipeSearch, sizeof( recipeSearch ) );

				ImGui::Separator();

				QString searchFilter = QString( recipeSearch ).toLower();

				ImGui::BeginChild( "##recipelist", ImVec2( 0, 0 ), false );

				for ( const auto& product : ws.products )
				{
					// Filter by search
					QString productName = itemName( product.sid );
					if ( !searchFilter.isEmpty() && !productName.toLower().contains( searchFilter ) )
						continue;

					ImGui::PushID( product.sid.toStdString().c_str() );

					// Product name (translated)
					ImGui::TextColored( ImVec4( 0.9f, 0.85f, 0.6f, 1.0f ), "%s", productName.toStdString().c_str() );

					// Material dropdowns per component
					QStringList mats;
					bool canCraft = true;

					for ( int c = 0; c < product.components.size(); ++c )
					{
						const auto& comp = product.components[c];
						QString compName = itemName( comp.sid );

						if ( comp.materials.isEmpty() )
						{
							canCraft = false;
							ImGui::TextDisabled( "    %d x %s (unavailable)", comp.amount, compName.toStdString().c_str() );
							mats.append( "any" );
							continue;
						}

						int& selIdx = s_craftSelectedMats[product.sid][c];
						if ( selIdx >= comp.materials.size() ) selIdx = 0;

						// Amount with fixed-width alignment
						ImGui::Text( "    %d x", comp.amount );
						ImGui::SameLine( 0, 5 );

						QString comboLabel = "##comp" + QString::number( c );

						QString matSid = comp.materials[selIdx].sid;
						QString matName = materialName( matSid );
						QString preview = matName + " " + compName + " (" + QString::number( comp.materials[selIdx].amount ) + ")";

						ImGui::SetNextItemWidth( 200 );
						if ( ImGui::BeginCombo( comboLabel.toStdString().c_str(), preview.toStdString().c_str() ) )
						{
							for ( int m = 0; m < comp.materials.size(); ++m )
							{
								QString mSid = comp.materials[m].sid;
								QString mName = materialName( mSid );
								QString label = mName + " " + compName + " (" + QString::number( comp.materials[m].amount ) + ")";
								if ( ImGui::Selectable( label.toStdString().c_str(), m == selIdx ) )
								{
									selIdx = m;
								}
							}
							ImGui::EndCombo();
						}

						mats.append( comp.materials[selIdx].sid );
					}

					// Craft button on the last row
					ImGui::SameLine();
					if ( !canCraft ) ImGui::BeginDisabled();
					if ( ImGui::Button( "Craft" ) )
					{
						bridge.cmdWorkshopCraftItem( product.sid, s_craftMode, QString::number( s_craftAmount ), mats );
					}
					if ( !canCraft ) ImGui::EndDisabled();

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					ImGui::PopID();
				}

				ImGui::EndChild();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// =============================================================================
// Agriculture panel
// =============================================================================
static void drawFarmView( ImGuiBridge& bridge )
{
	auto& farm = bridge.farmInfo;
	if ( farm.name.isEmpty() ) return;

	unsigned int farmID = farm.ID;

	ImGui::TextColored( ImVec4( 0.6f, 0.85f, 0.45f, 1.0f ), "Farm" );
	ImGui::SameLine( 60 );

	static char farmName[128];
	snprintf( farmName, sizeof( farmName ), "%s", farm.name.toStdString().c_str() );
	ImGui::PushItemWidth( -1 );
	if ( ImGui::InputText( "##FarmName", farmName, sizeof( farmName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
	{
		bridge.cmdAgriSetOptions( farmID, farmName, farm.priority, farm.suspended );
	}
	ImGui::PopItemWidth();

	bool suspended = farm.suspended;
	if ( ImGui::Checkbox( "Suspended", &suspended ) )
	{
		bridge.cmdAgriSetOptions( farmID, farm.name, farm.priority, suspended );
	}
	ImGui::SameLine();
	bool harvest = farm.harvest;
	if ( ImGui::Checkbox( "Harvest", &harvest ) )
	{
		bridge.cmdAgriSetHarvestOptions( farmID, harvest, false, false );
	}

	ImGui::Separator();

	QString currentCropName = farm.plantType.isEmpty() ? "None" : farm.product.name;
	if ( currentCropName.isEmpty() && !farm.plantType.isEmpty() ) currentCropName = farm.plantType;

	ImGui::Text( "Crop:" );
	ImGui::PushItemWidth( -1 );
	if ( ImGui::BeginCombo( "##CropSelect", currentCropName.toStdString().c_str() ) )
	{
		for ( const auto& plant : bridge.globalPlants )
		{
			// Only show crops where we have seeds available
			if ( plant.seedCount <= 0 && plant.plantID != farm.plantType )
				continue;

			bool selected = ( plant.plantID == farm.plantType );
			QString label = QString( "%1 (%2 seeds)" ).arg( plant.name ).arg( plant.seedCount );
			if ( ImGui::Selectable( label.toStdString().c_str(), selected ) )
			{
				bridge.cmdAgriSelectProduct( farmID, plant.plantID );
			}
			if ( selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	// Show seed info for selected crop
	if ( !farm.plantType.isEmpty() )
	{
		ImGui::Text( "Seeds available: %d", farm.product.seedCount );
	}

	ImGui::Separator();

	ImGui::Text( "Plots: %d", farm.numPlots );
	ImGui::Text( "Tilled: %d  Planted: %d", farm.tilled, farm.planted );
	ImGui::Text( "Ready to harvest: %d", farm.cropReady );
}

static void drawGroveView( ImGuiBridge& bridge )
{
	auto& grove = bridge.groveInfo;
	if ( grove.name.isEmpty() ) return;

	unsigned int groveID = grove.ID;

	ImGui::TextColored( ImVec4( 0.45f, 0.75f, 0.35f, 1.0f ), "Grove" );
	ImGui::SameLine( 60 );

	static char groveName[128];
	snprintf( groveName, sizeof( groveName ), "%s", grove.name.toStdString().c_str() );
	ImGui::PushItemWidth( -1 );
	if ( ImGui::InputText( "##GroveName", groveName, sizeof( groveName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
	{
		bridge.cmdAgriSetOptions( groveID, groveName, grove.priority, grove.suspended );
	}
	ImGui::PopItemWidth();

	bool suspended = grove.suspended;
	if ( ImGui::Checkbox( "Suspended", &suspended ) )
	{
		bridge.cmdAgriSetOptions( groveID, grove.name, grove.priority, suspended );
	}

	ImGui::Separator();

	QString currentTreeName = grove.treeType.isEmpty() ? "None" : grove.product.name;
	if ( currentTreeName.isEmpty() && !grove.treeType.isEmpty() ) currentTreeName = grove.treeType;

	ImGui::Text( "Tree:" );
	ImGui::PushItemWidth( -1 );
	if ( ImGui::BeginCombo( "##TreeSelect", currentTreeName.toStdString().c_str() ) )
	{
		for ( const auto& tree : bridge.globalTrees )
		{
			bool selected = ( tree.plantID == grove.treeType );
			if ( ImGui::Selectable( tree.name.toStdString().c_str(), selected ) )
			{
				bridge.cmdAgriSelectProduct( groveID, tree.plantID );
			}
			if ( selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::Separator();

	bool plantTrees = grove.plantTrees;
	if ( ImGui::Checkbox( "Plant trees", &plantTrees ) )
	{
		bridge.cmdAgriSetGroveOptions( groveID, grove.pickFruits, plantTrees, grove.fellTrees );
	}
	ImGui::SameLine();
	bool pickFruits = grove.pickFruits;
	if ( ImGui::Checkbox( "Pick fruit", &pickFruits ) )
	{
		bridge.cmdAgriSetGroveOptions( groveID, pickFruits, grove.plantTrees, grove.fellTrees );
	}

	bool fellTrees = grove.fellTrees;
	if ( ImGui::Checkbox( "Fell trees", &fellTrees ) )
	{
		bridge.cmdAgriSetGroveOptions( groveID, grove.pickFruits, grove.plantTrees, fellTrees );
	}

	ImGui::Separator();

	ImGui::Text( "Plots: %d  Trees: %d", grove.numPlots, grove.planted );
}

static void drawPastureView( ImGuiBridge& bridge )
{
	auto& pasture = bridge.pastureInfo;
	if ( pasture.name.isEmpty() ) return;

	ImGui::TextColored( ImVec4( 0.85f, 0.7f, 0.35f, 1.0f ), "Pasture" );
	ImGui::SameLine( 70 );

	static char pastureName[128];
	snprintf( pastureName, sizeof( pastureName ), "%s", pasture.name.toStdString().c_str() );
	ImGui::PushItemWidth( -1 );
	if ( ImGui::InputText( "##PastureName", pastureName, sizeof( pastureName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
	{
		bridge.cmdAgriSetOptions( pasture.ID, pastureName, pasture.priority, pasture.suspended );
	}
	ImGui::PopItemWidth();

	bool suspended = pasture.suspended;
	if ( ImGui::Checkbox( "Suspended", &suspended ) )
	{
		bridge.cmdAgriSetOptions( pasture.ID, pasture.name, pasture.priority, suspended );
	}
	ImGui::SameLine();
	bool harvest = pasture.harvest;
	if ( ImGui::Checkbox( "Harvest", &harvest ) )
	{
		bridge.cmdAgriSetHarvestOptions( pasture.ID, harvest, pasture.harvestHay, false );
	}

	ImGui::Separator();

	QString currentAnimalName = pasture.animalType.isEmpty() ? "None" : pasture.product.name;
	if ( currentAnimalName.isEmpty() ) currentAnimalName = pasture.animalType;

	ImGui::Text( "Animal:" );
	ImGui::PushItemWidth( -1 );
	if ( ImGui::BeginCombo( "##AnimalSelect", currentAnimalName.toStdString().c_str() ) )
	{
		for ( const auto& animal : bridge.globalAnimals )
		{
			bool selected = ( animal.animalID == pasture.animalType );
			if ( ImGui::Selectable( animal.name.toStdString().c_str(), selected ) )
			{
				bridge.cmdAgriSelectProduct( pasture.ID, animal.animalID );
			}
			if ( selected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	if ( !pasture.animalType.isEmpty() )
	{
		ImGui::Separator();

		ImGui::Text( "Males: %d  Females: %d  Total: %d/%d", pasture.numMale, pasture.numFemale,
			pasture.numMale + pasture.numFemale, pasture.maxNumber );

		int maxMale = pasture.maxMale;
		if ( ImGui::SliderInt( "Max Male", &maxMale, 0, qMax( 1, pasture.maxNumber ) ) )
		{
			bridge.cmdAgriSetMaxMale( pasture.ID, maxMale );
		}
		int maxFemale = pasture.maxFemale;
		if ( ImGui::SliderInt( "Max Female", &maxFemale, 0, qMax( 1, pasture.maxNumber ) ) )
		{
			bridge.cmdAgriSetMaxFemale( pasture.ID, maxFemale );
		}

		ImGui::Separator();

		bool harvestHay = pasture.harvestHay;
		if ( ImGui::Checkbox( "Harvest Hay", &harvestHay ) )
		{
			bridge.cmdAgriSetHarvestOptions( pasture.ID, pasture.harvest, harvestHay, false );
		}

		if ( pasture.foodMax > 0 )
		{
			ImGui::Text( "Food: %d/%d", pasture.foodCurrent, pasture.foodMax );
		}

		if ( !pasture.food.isEmpty() )
		{
			if ( ImGui::TreeNode( "Food Settings" ) )
			{
				for ( auto& fi : pasture.food )
				{
					bool checked = fi.checked;
					if ( ImGui::Checkbox( fi.name.toStdString().c_str(), &checked ) )
					{
						bridge.cmdAgriSetFoodItemChecked( fi.itemSID, fi.materialSID, checked );
					}
				}
				ImGui::TreePop();
			}
		}

		if ( !pasture.animals.isEmpty() )
		{
			ImGui::Separator();
			if ( ImGui::TreeNode( "Animals" ) )
			{
				for ( auto& animal : pasture.animals )
				{
					QString genderStr = animal.gender == Gender::MALE ? "M" : "F";
					QString label = QString( "%1 [%2]%3" ).arg( animal.name, genderStr, animal.isYoung ? " (young)" : "" );

					bool butcher = animal.toButcher;
					if ( ImGui::Checkbox( label.toStdString().c_str(), &butcher ) )
					{
						bridge.cmdAgriSetButchering( animal.id, butcher );
					}
				}
				ImGui::TreePop();
			}
		}
	}
}

void drawAgriculturePanel( ImGuiBridge& bridge )
{
	if ( !bridge.showAgriWindow )
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 130 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 280, io.DisplaySize.y - 200 ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Agriculture", &open, 0 );

	if ( !open ) bridge.cmdCloseAgriWindow();

	switch ( bridge.currentAgriType )
	{
		case AgriType::Farm:
			drawFarmView( bridge );
			break;
		case AgriType::Grove:
			drawGroveView( bridge );
			break;
		case AgriType::Pasture:
			drawPastureView( bridge );
			break;
	}

	ImGui::End();
}

// =============================================================================
// Creature info panel
// =============================================================================
void drawCreatureInfoPanel( ImGuiBridge& bridge )
{
	if ( !bridge.showCreatureInfo )
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 360, 150 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 350, io.DisplaySize.y - 220 ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Creature Info", &open );

	if ( !open )
	{
		bridge.showCreatureInfo = false;
		bridge.activeSidePanel = ImGuiBridge::SidePanel::None;
	}

	auto& ci = bridge.creatureInfo;
	bool isGnome = ( ci.creatureType == "Gnome" );

	// Name and type
	ImGui::TextColored( ImVec4( 1.0f, 0.9f, 0.6f, 1.0f ), "%s", ci.name.toStdString().c_str() );
	if ( isGnome )
	{
		ImGui::Text( "%s", ci.profession.toStdString().c_str() );
		if ( !ci.activity.isEmpty() )
			ImGui::TextColored( ImVec4( 0.6f, 0.8f, 0.6f, 1.0f ), "%s", ci.activity.toStdString().c_str() );
	}
	else
	{
		// Animal/Monster: show species and tame/wild status
		ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.9f, 1.0f ), "%s", ci.species.toStdString().c_str() );
		if ( !ci.profession.isEmpty() )
			ImGui::Text( "(%s)", ci.profession.toStdString().c_str() );

		// Health bar
		ImVec4 healthColor;
		if ( ci.healthPercent > 70 )
			healthColor = ImVec4( 0.2f, 0.7f, 0.3f, 1.0f );
		else if ( ci.healthPercent > 30 )
			healthColor = ImVec4( 0.7f, 0.6f, 0.1f, 1.0f );
		else
			healthColor = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f );

		ImGui::Text( "Health" );
		ImGui::SameLine( 80 );
		ImGui::PushStyleColor( ImGuiCol_PlotHistogram, healthColor );
		ImGui::ProgressBar( ci.healthPercent / 100.0f, ImVec2( -50, 0 ), "" );
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::Text( "%d%%", ci.healthPercent );

		if ( ci.healthStatus != "Healthy" )
			ImGui::TextColored( ImVec4( 0.8f, 0.3f, 0.3f, 1.0f ), "%s", ci.healthStatus.toStdString().c_str() );
	}

	ImGui::Separator();

	// Animal/Monster: show relevant info, then end early
	if ( !isGnome )
	{
		if ( ci.creatureType == "Animal" )
		{
			// Hunger bar
			{
				int val = ci.hunger;
				ImVec4 col;
				if ( val > 60 ) col = ImVec4( 0.2f, 0.6f, 0.3f, 1.0f );
				else if ( val > 30 ) col = ImVec4( 0.7f, 0.6f, 0.1f, 1.0f );
				else col = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f );
				ImGui::Text( "Hunger" );
				ImGui::SameLine( 80 );
				ImGui::PushStyleColor( ImGuiCol_PlotHistogram, col );
				ImGui::ProgressBar( qBound( 0.0f, val / 100.0f, 1.0f ), ImVec2( -50, 0 ), "" );
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if ( val <= 5 )
					ImGui::TextColored( ImVec4( 0.9f, 0.2f, 0.2f, 1.0f ), "Starving!" );
				else if ( val <= 20 )
					ImGui::TextColored( ImVec4( 0.8f, 0.5f, 0.2f, 1.0f ), "%d%% Hungry", val );
				else
					ImGui::Text( "%d%%", qBound( 0, val, 100 ) );
			}

			ImGui::Separator();

			// Diet
			if ( !ci.diet.isEmpty() )
			{
				ImGui::Text( "Diet:" );
				ImGui::SameLine();
				ImGui::TextColored( ImVec4( 0.7f, 0.8f, 0.6f, 1.0f ), "%s", ci.diet.toStdString().c_str() );
			}

			// Temperament
			if ( ci.isAggressive )
				ImGui::TextColored( ImVec4( 0.9f, 0.3f, 0.3f, 1.0f ), "Aggressive" );
			else
				ImGui::TextColored( ImVec4( 0.5f, 0.7f, 0.5f, 1.0f ), "Passive" );

			// Combat stats
			if ( ci.attackValue > 0 || ci.damageValue > 0 )
			{
				ImGui::Text( "Attack: %d  Damage: %d", ci.attackValue, ci.damageValue );
			}

			// Butcher drops
			if ( !ci.butcherDrops.isEmpty() )
			{
				ImGui::Separator();
				ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.8f, 1.0f ), "Butcher yields:" );
				for ( const auto& drop : ci.butcherDrops )
				{
					ImGui::Text( "  %s", drop.toStdString().c_str() );
				}
			}
		}

		// Anatomy section (for animals/monsters)
		ImGui::Separator();
		if ( ImGui::CollapsingHeader( "Anatomy" ) )
		{
			// Rot stage
			if ( ci.anatomyStatus & AS_DEAD )
			{
				const char* rotNames[] = { "Fresh", "Decaying", "Rotting", "Skeleton", "Bones" };
				ImVec4 rotColors[] = {
					ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ), ImVec4( 0.7f, 0.6f, 0.3f, 1.0f ),
					ImVec4( 0.6f, 0.3f, 0.1f, 1.0f ), ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
					ImVec4( 0.4f, 0.4f, 0.4f, 1.0f )
				};
				int rs = qBound( 0, ci.rotStage, 4 );
				ImGui::TextColored( rotColors[rs], "Corpse: %s", rotNames[rs] );
				if ( ci.isBuried ) ImGui::SameLine(); ImGui::TextColored( ImVec4( 0.4f, 0.7f, 0.4f, 1.0f ), ci.isBuried ? "(Buried)" : "" );
			}

			// Blood bar
			float bloodRatio = ci.maxBlood > 0 ? ci.bloodLevel / ci.maxBlood : 0;
			ImVec4 bloodColor = bloodRatio > 0.8f ? ImVec4( 0.7f, 0.1f, 0.1f, 1.0f ) :
			                    bloodRatio > 0.4f ? ImVec4( 0.7f, 0.4f, 0.1f, 1.0f ) :
			                                        ImVec4( 0.3f, 0.3f, 0.3f, 1.0f );
			ImGui::Text( "Blood" );
			ImGui::SameLine( 80 );
			ImGui::PushStyleColor( ImGuiCol_PlotHistogram, bloodColor );
			ImGui::ProgressBar( qBound( 0.0f, bloodRatio, 1.0f ), ImVec2( -1, 0 ) );
			ImGui::PopStyleColor();

			if ( ci.bleedingRate > 0 )
			{
				ImGui::TextColored( ImVec4( 1.0f, 0.2f, 0.2f, 1.0f ), "BLEEDING (%.1f/tick)", ci.bleedingRate );
			}

			// Status
			if ( ci.anatomyStatus & AS_UNCONSCIOUS ) ImGui::TextColored( ImVec4( 0.8f, 0.5f, 0.1f, 1.0f ), "Unconscious" );
			if ( ci.anatomyStatus & AS_HALFBLIND ) ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.3f, 1.0f ), "Partially Blind" );
			if ( ci.anatomyStatus & AS_BLIND ) ImGui::TextColored( ImVec4( 0.8f, 0.2f, 0.2f, 1.0f ), "Blind" );

			// Body parts
			for ( const auto& bp : ci.bodyParts )
			{
				float hpRatio = bp.maxHP > 0 ? (float)bp.hp / bp.maxHP : 0;
				ImVec4 partColor;
				if ( bp.hp <= 0 )       partColor = ImVec4( 0.4f, 0.4f, 0.4f, 1.0f ); // destroyed
				else if ( hpRatio > 0.75f ) partColor = ImVec4( 0.2f, 0.7f, 0.3f, 1.0f ); // healthy
				else if ( hpRatio > 0.25f ) partColor = ImVec4( 0.7f, 0.6f, 0.1f, 1.0f ); // damaged
				else                    partColor = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f ); // critical

				QString label = bp.name;
				if ( bp.isVital ) label += " *";

				ImGui::Text( "%s", label.toStdString().c_str() );
				ImGui::SameLine( bp.isInside ? 120 : 100 );
				ImGui::PushStyleColor( ImGuiCol_PlotHistogram, partColor );
				ImGui::PushID( label.toStdString().c_str() );
				ImGui::ProgressBar( qBound( 0.0f, hpRatio, 1.0f ), ImVec2( -1, 0 ), bp.hp <= 0 ? "destroyed" : "" );
				ImGui::PopID();
				ImGui::PopStyleColor();
			}
		}

		ImGui::End();
		return;
	}

	// ===== Everything below is GNOME-ONLY =====

	// Mood bar (prominent, at the top)
	{
		ImVec4 moodColor;
		if ( ci.mood > 65 )
			moodColor = ImVec4( 0.2f, 0.7f, 0.3f, 1.0f );
		else if ( ci.mood > 35 )
			moodColor = ImVec4( 0.7f, 0.7f, 0.2f, 1.0f );
		else if ( ci.mood > 15 )
			moodColor = ImVec4( 0.8f, 0.4f, 0.1f, 1.0f );
		else
			moodColor = ImVec4( 0.8f, 0.1f, 0.1f, 1.0f );

		ImGui::Text( "Mood" );
		ImGui::SameLine( 80 );
		ImGui::PushStyleColor( ImGuiCol_PlotHistogram, moodColor );
		ImGui::ProgressBar( ci.mood / 100.0f, ImVec2( -50, 0 ), "" );
		ImGui::PopStyleColor();
		ImGui::SameLine();
		if ( ci.mentalBreak )
			ImGui::TextColored( ImVec4( 1.0f, 0.2f, 0.2f, 1.0f ), "!!!" );
		else
			ImGui::Text( "%d%%", ci.mood );

		// Mood breakdown tooltip on hover
		if ( ImGui::IsItemHovered() )
		{
			ImGui::SetTooltip( "Mood breakdown:\n  Base (Optimism): %d\n  Thoughts: %+d\n  Needs penalty: %+d\n  ────────\n  Total: %d",
				ci.baseMood, ci.thoughtSum, ci.needsPenalty, ci.mood );
		}
	}

	// Needs bars
	auto needBar = []( const char* label, int value ) {
		ImVec4 col;
		float v = qBound( 0.0f, value / 100.0f, 1.0f );
		if ( value > 60 )
			col = ImVec4( 0.2f, 0.6f, 0.3f, 1.0f );
		else if ( value > 30 )
			col = ImVec4( 0.7f, 0.6f, 0.1f, 1.0f );
		else
			col = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f );
		ImGui::Text( "%s", label );
		ImGui::SameLine( 80 );
		ImGui::PushStyleColor( ImGuiCol_PlotHistogram, col );
		ImGui::ProgressBar( v, ImVec2( -50, 0 ), "" );
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::Text( "%d%%", qBound( 0, value, 100 ) );
	};

	needBar( "Hunger", ci.hunger );
	needBar( "Thirst", ci.thirst );
	needBar( "Sleep", ci.sleep );

	ImGui::Separator();

	// Backstory
	if ( !ci.childhoodTitle.isEmpty() || !ci.adulthoodTitle.isEmpty() )
	{
		if ( ImGui::CollapsingHeader( "Backstory", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui::Indent( 8.0f );
			if ( !ci.childhoodTitle.isEmpty() )
			{
				ImGui::TextColored( ImVec4( 0.6f, 0.7f, 0.9f, 1.0f ), "Youth:" );
				ImGui::SameLine();
				ImGui::Text( "%s", ci.childhoodTitle.toStdString().c_str() );
				if ( ImGui::IsItemHovered() && !ci.childhoodDesc.isEmpty() )
					ImGui::SetTooltip( "%s", ci.childhoodDesc.toStdString().c_str() );
			}
			if ( !ci.adulthoodTitle.isEmpty() )
			{
				ImGui::TextColored( ImVec4( 0.6f, 0.7f, 0.9f, 1.0f ), "Before:" );
				ImGui::SameLine();
				ImGui::Text( "%s", ci.adulthoodTitle.toStdString().c_str() );
				if ( ImGui::IsItemHovered() && !ci.adulthoodDesc.isEmpty() )
					ImGui::SetTooltip( "%s", ci.adulthoodDesc.toStdString().c_str() );
			}
			ImGui::Unindent( 8.0f );
		}
	}

	// Personality Traits
	if ( !ci.traits.isEmpty() )
	{
		if ( ImGui::CollapsingHeader( "Personality", ImGuiTreeNodeFlags_DefaultOpen ) )
		{
			ImGui::Indent( 8.0f );
			bool hasNotable = false;
			for ( const auto& trait : ci.traits )
			{
				if ( trait.label.isEmpty() ) continue;
				hasNotable = true;

				float barFrac = ( trait.value + 50.0f ) / 100.0f;
				ImVec4 barColor = ( trait.value > 0 ) ?
					ImVec4( 0.2f, 0.6f, 0.3f, 1.0f ) :
					ImVec4( 0.7f, 0.3f, 0.2f, 1.0f );

				ImGui::PushStyleColor( ImGuiCol_PlotHistogram, barColor );
				ImGui::ProgressBar( barFrac, ImVec2( 100, 12 ), "" );
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::Text( "%s", trait.label.toStdString().c_str() );
				if ( ImGui::IsItemHovered() && !trait.description.isEmpty() )
					ImGui::SetTooltip( "[%s %+d] %s", trait.id.toStdString().c_str(), trait.value, trait.description.toStdString().c_str() );
			}
			if ( !hasNotable )
				ImGui::TextDisabled( "Unremarkable" );
			ImGui::Unindent( 8.0f );
		}
	}

	// Active Thoughts (sorted by impact, highest first)
	if ( ImGui::CollapsingHeader( "Thoughts", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::Indent( 8.0f );
		if ( ci.thoughts.isEmpty() )
		{
			ImGui::TextDisabled( "No strong feelings right now" );
		}
		else
		{
			for ( const auto& thought : ci.thoughts )
			{
				ImVec4 tColor = thought.moodValue > 0 ?
					ImVec4( 0.3f, 0.8f, 0.3f, 1.0f ) :
					ImVec4( 0.8f, 0.3f, 0.3f, 1.0f );

				// Fade color as thought expires
				float remaining = ( thought.initialDuration > 0 ) ?
					(float)thought.ticksLeft / thought.initialDuration : 1.0f;
				tColor.w = 0.5f + 0.5f * remaining; // alpha fades from 1.0 to 0.5

				ImGui::TextColored( tColor, "%+d", thought.moodValue );
				ImGui::SameLine();
				ImGui::Text( "%s", thought.text.toStdString().c_str() );

				// Tooltip: cause + time remaining
				if ( ImGui::IsItemHovered() )
				{
					int secsLeft = thought.ticksLeft * 50 / 1000; // rough: 50ms per tick
					int minsLeft = secsLeft / 60;
					int pctLeft = ( thought.initialDuration > 0 ) ?
						( thought.ticksLeft * 100 / thought.initialDuration ) : 100;
					QString tip = thought.cause.isEmpty() ? thought.text : thought.cause;
					if ( minsLeft > 0 )
						ImGui::SetTooltip( "%s\nFading: %d%% (%d min left)", tip.toStdString().c_str(), pctLeft, minsLeft );
					else
						ImGui::SetTooltip( "%s\nFading: %d%% (%d sec left)", tip.toStdString().c_str(), pctLeft, secsLeft );
				}
			}
		}
		ImGui::Unindent( 8.0f );
	}

	// Social: relationships + recent memories
	if ( !ci.relationships.isEmpty() || !ci.socialMemories.isEmpty() )
	{
		if ( ImGui::CollapsingHeader( "Social" ) )
		{
			ImGui::Indent( 8.0f );

			for ( const auto& rel : ci.relationships )
			{
				ImVec4 relColor;
				if ( rel.opinion > 20 )
					relColor = ImVec4( 0.3f, 0.7f, 0.3f, 1.0f );
				else if ( rel.opinion < -20 )
					relColor = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f );
				else
					relColor = ImVec4( 0.7f, 0.7f, 0.3f, 1.0f );
				ImGui::TextColored( relColor, "%s", rel.label.toStdString().c_str() );
				ImGui::SameLine();
				ImGui::Text( "%s (%+d)", rel.name.toStdString().c_str(), rel.opinion );
			}

			if ( !ci.socialMemories.isEmpty() )
			{
				ImGui::Spacing();
				ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.8f, 1.0f ), "Recent events:" );
				for ( const auto& mem : ci.socialMemories )
				{
					ImVec4 memColor = mem.change > 0 ?
						ImVec4( 0.4f, 0.7f, 0.4f, 0.8f ) :
						( mem.change < 0 ? ImVec4( 0.7f, 0.4f, 0.4f, 0.8f ) : ImVec4( 0.5f, 0.5f, 0.5f, 0.8f ) );
					ImGui::TextColored( memColor, "  %s", mem.event.toStdString().c_str() );
					if ( ImGui::IsItemHovered() )
					{
						if ( mem.daysAgo == 0 )
							ImGui::SetTooltip( "Today (%+d opinion)", mem.change );
						else
							ImGui::SetTooltip( "%d day%s ago (%+d opinion)", mem.daysAgo, mem.daysAgo > 1 ? "s" : "", mem.change );
					}
				}
			}

			ImGui::Unindent( 8.0f );
		}
	}

	// Skills
	if ( !ci.skills.isEmpty() )
	{
		if ( ImGui::CollapsingHeader( "Skills" ) )
		{
			ImGui::Indent( 8.0f );
			for ( const auto& skill : ci.skills )
			{
				// Color by level
				ImVec4 lvlColor;
				if ( skill.level >= 10 )
					lvlColor = ImVec4( 1.0f, 0.85f, 0.2f, 1.0f ); // gold - master
				else if ( skill.level >= 5 )
					lvlColor = ImVec4( 0.3f, 0.8f, 0.3f, 1.0f ); // green - skilled
				else if ( skill.level >= 1 )
					lvlColor = ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ); // grey - basic
				else
					lvlColor = ImVec4( 0.4f, 0.4f, 0.4f, 1.0f ); // dim - untrained

				// Dim inactive skills
				if ( !skill.active )
					lvlColor.w = 0.4f;

				ImGui::TextColored( lvlColor, "%2d", skill.level );
				ImGui::SameLine();
				if ( skill.active )
					ImGui::Text( "%s", skill.name.toStdString().c_str() );
				else
					ImGui::TextDisabled( "%s (off)", skill.name.toStdString().c_str() );

				if ( ImGui::IsItemHovered() )
				{
					ImGui::SetTooltip( "%s\nLevel: %d\nXP: %d\n%s",
						skill.name.toStdString().c_str(),
						skill.level, skill.xp,
						skill.active ? "Active" : "Inactive — won't take these jobs" );
				}
			}
			ImGui::Unindent( 8.0f );
		}
	}

	// Stats (collapsed by default)
	if ( ImGui::CollapsingHeader( "Stats" ) )
	{
		ImGui::Text( "STR: %d  DEX: %d  CON: %d", ci.str, ci.dex, ci.con );
		ImGui::Text( "INT: %d  WIS: %d  CHA: %d", ci.intel, ci.wis, ci.cha );
	}

	// Equipment & Combat
	if ( ImGui::CollapsingHeader( "Equipment & Combat", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		ImGui::Indent( 8.0f );

		// Equipment slots with names
		auto showSlot = []( const char* label, const EquipmentItem& slot ) {
			ImGui::Text( "  %s:", label );
			ImGui::SameLine( 110 );
			if ( slot.itemID != 0 )
			{
				QString itemName = S::s( "$ItemName_" + slot.item );
				QString matName  = S::s( "$MaterialName_" + slot.material );
				ImGui::TextColored( ImVec4( 0.6f, 0.9f, 0.6f, 1.0f ), "%s %s",
					matName.toStdString().c_str(), itemName.toStdString().c_str() );
			}
			else
			{
				ImGui::TextDisabled( "none" );
			}
		};

		showSlot( "Head",       ci.equipment.head );
		showSlot( "Chest",      ci.equipment.chest );
		showSlot( "Arms",       ci.equipment.arm );
		showSlot( "Hands",      ci.equipment.hand );
		showSlot( "Legs",       ci.equipment.leg );
		showSlot( "Feet",       ci.equipment.foot );
		showSlot( "Left Hand",  ci.equipment.leftHandHeld );
		showSlot( "Right Hand", ci.equipment.rightHandHeld );
		showSlot( "Back",       ci.equipment.back );

		ImGui::Spacing();

		// Combat stats
		ImGui::TextColored( ImVec4( 0.9f, 0.7f, 0.4f, 1.0f ), "Combat Stats:" );

		// Find combat skill levels from ci.skills
		int meleeSkill = 0, dodgeSkill = 0, rangedSkill = 0;
		for ( const auto& skill : ci.skills )
		{
			if ( skill.name == "Melee" ) meleeSkill = skill.level;
			else if ( skill.name == "Dodge" ) dodgeSkill = skill.level;
			else if ( skill.name == "Ranged" ) rangedSkill = skill.level;
		}

		// Attack: weapon damage or unarmed (Str)
		QString weaponName = "Fists";
		int attackDmg = ci.str; // unarmed = Str
		if ( ci.equipment.rightHandHeld.itemID != 0 )
		{
			weaponName = S::s( "$ItemName_" + ci.equipment.rightHandHeld.item );
			// AttackValue from DB for the weapon item type
			int weaponAV = DB::select( "AttackValue", "Items", ci.equipment.rightHandHeld.item ).toInt();
			if ( weaponAV > 0 ) attackDmg = weaponAV;
		}

		ImGui::Text( "  Attack:" );
		ImGui::SameLine( 110 );
		ImGui::Text( "%d (%s)", attackDmg, weaponName.toStdString().c_str() );

		ImGui::Text( "  Melee:" );
		ImGui::SameLine( 110 );
		ImGui::Text( "%d", meleeSkill );

		ImGui::Text( "  Dodge:" );
		ImGui::SameLine( 110 );
		ImGui::Text( "%d", dodgeSkill );

		if ( rangedSkill > 0 )
		{
			ImGui::Text( "  Ranged:" );
			ImGui::SameLine( 110 );
			ImGui::Text( "%d", rangedSkill );
		}

		ImGui::Unindent( 8.0f );
	}

	// Anatomy section (for gnomes)
	ImGui::Separator();
	if ( ImGui::CollapsingHeader( "Anatomy" ) )
	{
		// Rot stage (only for dead gnomes)
		if ( ci.anatomyStatus & AS_DEAD )
		{
			const char* rotNames[] = { "Fresh", "Decaying", "Rotting", "Skeleton", "Bones" };
			ImVec4 rotColors[] = {
				ImVec4( 0.7f, 0.7f, 0.7f, 1.0f ), ImVec4( 0.7f, 0.6f, 0.3f, 1.0f ),
				ImVec4( 0.6f, 0.3f, 0.1f, 1.0f ), ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ),
				ImVec4( 0.4f, 0.4f, 0.4f, 1.0f )
			};
			int rs = qBound( 0, ci.rotStage, 4 );
			ImGui::TextColored( rotColors[rs], "Corpse: %s", rotNames[rs] );
			if ( ci.isBuried ) { ImGui::SameLine(); ImGui::TextColored( ImVec4( 0.4f, 0.7f, 0.4f, 1.0f ), "(Buried)" ); }
		}

		float bloodRatio = ci.maxBlood > 0 ? ci.bloodLevel / ci.maxBlood : 0;
		ImVec4 bloodColor = bloodRatio > 0.8f ? ImVec4( 0.7f, 0.1f, 0.1f, 1.0f ) :
		                    bloodRatio > 0.4f ? ImVec4( 0.7f, 0.4f, 0.1f, 1.0f ) :
		                                        ImVec4( 0.3f, 0.3f, 0.3f, 1.0f );
		ImGui::Text( "Blood" );
		ImGui::SameLine( 80 );
		ImGui::PushStyleColor( ImGuiCol_PlotHistogram, bloodColor );
		ImGui::ProgressBar( qBound( 0.0f, bloodRatio, 1.0f ), ImVec2( -1, 0 ) );
		ImGui::PopStyleColor();

		if ( ci.bleedingRate > 0 )
			ImGui::TextColored( ImVec4( 1.0f, 0.2f, 0.2f, 1.0f ), "BLEEDING (%.1f/tick)", ci.bleedingRate );

		if ( ci.anatomyStatus & AS_UNCONSCIOUS ) ImGui::TextColored( ImVec4( 0.8f, 0.5f, 0.1f, 1.0f ), "Unconscious" );
		if ( ci.anatomyStatus & AS_HALFBLIND ) ImGui::TextColored( ImVec4( 0.7f, 0.7f, 0.3f, 1.0f ), "Partially Blind" );
		if ( ci.anatomyStatus & AS_BLIND ) ImGui::TextColored( ImVec4( 0.8f, 0.2f, 0.2f, 1.0f ), "Blind" );

		for ( const auto& bp : ci.bodyParts )
		{
			float hpRatio = bp.maxHP > 0 ? (float)bp.hp / bp.maxHP : 0;
			ImVec4 partColor;
			if ( bp.hp <= 0 )           partColor = ImVec4( 0.4f, 0.4f, 0.4f, 1.0f );
			else if ( hpRatio > 0.75f ) partColor = ImVec4( 0.2f, 0.7f, 0.3f, 1.0f );
			else if ( hpRatio > 0.25f ) partColor = ImVec4( 0.7f, 0.6f, 0.1f, 1.0f );
			else                        partColor = ImVec4( 0.8f, 0.2f, 0.2f, 1.0f );

			QString label = bp.name + ( bp.isVital ? " *" : "" );
			ImGui::Text( "%s", label.toStdString().c_str() );
			ImGui::SameLine( bp.isInside ? 120 : 100 );
			ImGui::PushStyleColor( ImGuiCol_PlotHistogram, partColor );
			ImGui::PushID( label.toStdString().c_str() );
			ImGui::ProgressBar( qBound( 0.0f, hpRatio, 1.0f ), ImVec2( -1, 0 ), bp.hp <= 0 ? "destroyed" : "" );
			ImGui::PopID();
			ImGui::PopStyleColor();
		}
	}

	ImGui::End();
}

// =============================================================================
// Inventory panel (same as Kingdom but with checkboxes)
// =============================================================================
void drawInventoryPanel( ImGuiBridge& bridge )
{
	// Kingdom panel already handles this
	drawKingdomPanel( bridge );
}

// =============================================================================
// Debug panel
// =============================================================================
// Dev tool DB list cache — populated once on first open
struct DevToolCache
{
	bool initialized = false;
	QStringList monsterIDs;
	QStringList animalIDs;
	QStringList itemCategories;
	QHash<QString, QStringList> itemsByCategory;  // category -> item IDs
	QHash<QString, QStringList> materialsByType;  // material type -> material IDs

	void init()
	{
		if ( initialized ) return;
		monsterIDs = DB::ids( "Monsters" );
		animalIDs  = DB::ids( "Animals" );

		// Build item categories and items per category
		auto itemRows = DB::selectRows( "Items" );
		QSet<QString> catSet;
		for ( const auto& row : itemRows )
		{
			QString cat = row.value( "Category" ).toString();
			QString id  = row.value( "ID" ).toString();
			if ( cat.isEmpty() || id.isEmpty() ) continue;
			catSet.insert( cat );
			itemsByCategory[cat].append( id );
		}
		itemCategories = catSet.values();
		itemCategories.sort();

		// Build materials by type
		auto matRows = DB::selectRows( "Materials" );
		for ( const auto& row : matRows )
		{
			QString type = row.value( "Type" ).toString();
			QString id   = row.value( "ID" ).toString();
			if ( type.isEmpty() || id.isEmpty() ) continue;
			materialsByType[type].append( id );
		}

		initialized = true;
	}
};

static DevToolCache s_devCache;

// Helper: draw a combo from QStringList, returns selected index
static bool drawCombo( const char* label, int& selected, const QStringList& items )
{
	if ( items.isEmpty() ) return false;
	selected = qBound( 0, selected, items.size() - 1 );
	const char* preview = items[selected].toUtf8().constData();

	// Need stable storage for combo items
	static QByteArray previewBuf;
	previewBuf = items[selected].toUtf8();

	bool changed = false;
	if ( ImGui::BeginCombo( label, previewBuf.constData() ) )
	{
		for ( int i = 0; i < items.size(); ++i )
		{
			QByteArray itemBuf = items[i].toUtf8();
			bool isSelected = ( i == selected );
			if ( ImGui::Selectable( itemBuf.constData(), isSelected ) )
			{
				selected = i;
				changed = true;
			}
			if ( isSelected )
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return changed;
}

void drawDebugPanel( ImGuiBridge& bridge )
{
	if ( !bridge.showDebugPanel )
		return;

	s_devCache.init();

	ImGui::SetNextWindowSize( ImVec2( 420, 450 ), ImGuiCond_FirstUseEver );
	ImGui::Begin( "Dev Tools", &bridge.showDebugPanel );

	// Show spawn location info at the top
	unsigned int spawnTile = bridge.selectedTileID;
	if ( spawnTile > 0 )
	{
		Position spawnPos( spawnTile );
		ImGui::TextColored( ImVec4( 0.4f, 0.9f, 0.4f, 1.0f ), "Spawn at: selected tile (%d, %d, %d)",
			spawnPos.x, spawnPos.y, spawnPos.z );
	}
	else
	{
		ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.3f, 1.0f ), "Spawn at: map center (click a tile to target)" );
	}
	ImGui::Separator();

	if ( ImGui::BeginTabBar( "DevToolTabs" ) )
	{
		// ===================== SPAWN CREATURES =====================
		if ( ImGui::BeginTabItem( "Creatures" ) )
		{
			// Gnome spawn
			if ( ImGui::Button( "Spawn Gnome" ) )
			{
				bridge.cmdSpawnGnome( spawnTile );
			}

			ImGui::Separator();
			ImGui::Text( "Monsters" );

			static int monsterIdx = 0;
			drawCombo( "Species##monster", monsterIdx, s_devCache.monsterIDs );

			static int monsterAmt = 1;
			ImGui::SliderInt( "Amount##monster", &monsterAmt, 1, 20 );

			if ( ImGui::Button( "Spawn Monsters" ) )
			{
				bridge.cmdSpawnMonster( s_devCache.monsterIDs[monsterIdx], monsterAmt, spawnTile );
			}

			ImGui::Separator();
			ImGui::Text( "Animals" );

			static int animalIdx = 0;
			drawCombo( "Species##animal", animalIdx, s_devCache.animalIDs );

			static int animalAmt = 1;
			ImGui::SliderInt( "Amount##animal", &animalAmt, 1, 20 );

			if ( ImGui::Button( "Spawn Animals" ) )
			{
				bridge.cmdSpawnAnimal( s_devCache.animalIDs[animalIdx], animalAmt, spawnTile );
			}

			ImGui::EndTabItem();
		}

		// ===================== SPAWN ITEMS =====================
		if ( ImGui::BeginTabItem( "Items" ) )
		{
			static int catIdx = 0;
			if ( drawCombo( "Category", catIdx, s_devCache.itemCategories ) )
			{
				// Reset item selection when category changes
			}

			QString selectedCat = s_devCache.itemCategories.isEmpty() ? "" : s_devCache.itemCategories[catIdx];
			const QStringList& itemsInCat = s_devCache.itemsByCategory[selectedCat];

			static int itemIdx = 0;
			if ( drawCombo( "Item", itemIdx, itemsInCat ) )
			{
				// Reset material when item changes
			}

			// Get allowed material types for selected item
			QString selectedItem = itemsInCat.isEmpty() ? "" : itemsInCat[qBound( 0, itemIdx, itemsInCat.size() - 1 )];
			QStringList availMaterials;
			if ( !selectedItem.isEmpty() )
			{
				QString allowedTypes = DB::select( "AllowedMaterialTypes", "Items", selectedItem ).toString();
				QString allowedMats  = DB::select( "AllowedMaterials", "Items", selectedItem ).toString();
				if ( !allowedMats.isEmpty() )
				{
					availMaterials = allowedMats.split( "|" );
				}
				else if ( !allowedTypes.isEmpty() )
				{
					for ( const auto& type : allowedTypes.split( "|" ) )
					{
						availMaterials.append( s_devCache.materialsByType.value( type ) );
					}
				}
				if ( availMaterials.isEmpty() )
				{
					availMaterials.append( "any" );
				}
			}

			static int matIdx = 0;
			drawCombo( "Material", matIdx, availMaterials );

			static int itemAmt = 1;
			ImGui::SliderInt( "Amount##item", &itemAmt, 1, 50 );

			QString selectedMat = availMaterials.isEmpty() ? "any" : availMaterials[qBound( 0, matIdx, availMaterials.size() - 1 )];

			if ( ImGui::Button( "Spawn Items" ) && !selectedItem.isEmpty() )
			{
				bridge.cmdSpawnItem( selectedItem, selectedMat, itemAmt, spawnTile );
			}

			ImGui::EndTabItem();
		}

		// ===================== TRIGGER EVENTS =====================
		if ( ImGui::BeginTabItem( "Events" ) )
		{
			if ( ImGui::Button( "Trigger Trader" ) )
			{
				bridge.cmdSpawnCreature( "Trader" );
			}

			ImGui::Separator();
			ImGui::Text( "Invasion" );

			static int invasionMonsterIdx = 0;
			drawCombo( "Species##invasion", invasionMonsterIdx, s_devCache.monsterIDs );

			static int invasionAmt = 5;
			ImGui::SliderInt( "Amount##invasion", &invasionAmt, 1, 50 );

			if ( ImGui::Button( "Trigger Invasion" ) )
			{
				bridge.cmdSpawnMonster( s_devCache.monsterIDs[invasionMonsterIdx], invasionAmt, spawnTile );
			}

			ImGui::EndTabItem();
		}

		// ===================== GAME STATE =====================
		if ( ImGui::BeginTabItem( "Game" ) )
		{
			bool paused = Global::cfg->get( "Pause" ).toBool();
			if ( ImGui::Checkbox( "Paused", &paused ) )
			{
				Global::cfg->set( "Pause", paused );
			}

			int speed = Global::cfg->get( "GameSpeed" ).toInt();
			if ( ImGui::SliderInt( "Game Speed", &speed, 1, 50 ) )
			{
				Global::cfg->set( "GameSpeed", speed );
			}

			ImGui::Separator();
			ImGui::Text( "Time: Day %d, %02d:%02d", GameState::day, GameState::hour, GameState::minute );
			ImGui::Text( "Season: %s, Year %d", GameState::seasonString.toUtf8().constData(), GameState::year );
			ImGui::Text( "Tick: %llu", GameState::tick );

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// =============================================================================
// Event Log panel
// =============================================================================
void drawEventLogPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 55 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.65f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Event Log", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	// Filter buttons
	static bool showInfo = true, showWarning = true, showCombat = true, showDeath = true, showWildlife = true, showJobs = false, showDebug = false;
	ImGui::Checkbox( "Info", &showInfo ); ImGui::SameLine();
	ImGui::Checkbox( "Warning", &showWarning ); ImGui::SameLine();
	ImGui::Checkbox( "Combat", &showCombat ); ImGui::SameLine();
	ImGui::Checkbox( "Death", &showDeath ); ImGui::SameLine();
	ImGui::Checkbox( "Wildlife", &showWildlife ); ImGui::SameLine();
	ImGui::Checkbox( "Jobs", &showJobs ); ImGui::SameLine();
	ImGui::Checkbox( "Debug", &showDebug );
	ImGui::Separator();

	// Scrollable log
	ImGui::BeginChild( "LogScroll", ImVec2( 0, 0 ), false, ImGuiWindowFlags_HorizontalScrollbar );

	auto& messages = Global::logger().messages();
	for ( int i = (int)messages.size() - 1; i >= 0; --i )
	{
		auto& lm = messages[i];

		// Filter
		bool show = false;
		switch ( lm.type )
		{
			case LogType::INFO:
			case LogType::MIGRATION: show = showInfo; break;
			case LogType::WARNING:   show = showWarning; break;
			case LogType::COMBAT:    show = showCombat; break;
			case LogType::DEATH:     show = showDeath; break;
			case LogType::WILDLIFE:  show = showWildlife; break;
			case LogType::JOB:
			case LogType::CRAFT:     show = showJobs; break;
			case LogType::DEBUG:     show = showDebug; break;
			default:                 show = true; break;
		}
		if ( !show ) continue;

		// Color by type
		ImVec4 color;
		switch ( lm.type )
		{
			case LogType::DEATH:     color = ImVec4( 1.0f, 0.2f, 0.2f, 1.0f ); break;
			case LogType::DANGER:    color = ImVec4( 1.0f, 0.5f, 0.0f, 1.0f ); break;
			case LogType::WARNING:   color = ImVec4( 1.0f, 0.8f, 0.0f, 1.0f ); break;
			case LogType::COMBAT:    color = ImVec4( 0.9f, 0.4f, 0.4f, 1.0f ); break;
			case LogType::MIGRATION: color = ImVec4( 0.4f, 0.8f, 0.4f, 1.0f ); break;
			case LogType::WILDLIFE:  color = ImVec4( 0.8f, 0.6f, 0.2f, 1.0f ); break;
			case LogType::INFO:      color = ImVec4( 0.7f, 0.7f, 1.0f, 1.0f ); break;
			case LogType::JOB:
			case LogType::CRAFT:     color = ImVec4( 0.6f, 0.6f, 0.6f, 1.0f ); break;
			default:                 color = ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ); break;
		}

		ImGui::PushStyleColor( ImGuiCol_Text, color );
		QString line = "[" + lm.dateTime + "] " + lm.message;
		ImGui::TextWrapped( "%s", line.toStdString().c_str() );
		ImGui::PopStyleColor();
	}

	ImGui::EndChild();
	ImGui::End();
}
