#include "ui_sidepanels.h"
#include "../imguibridge.h"
#include "../../base/global.h"
#include "../../base/gamestate.h"
#include "../../base/logger.h"
#include <imgui.h>

// =============================================================================
// Kingdom / Inventory panel
// =============================================================================
void drawKingdomPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 50 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Kingdom - Inventory", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	ImGui::Columns( 4, "invCols" );
	ImGui::Text( "Category" ); ImGui::NextColumn();
	ImGui::Text( "In Stock" ); ImGui::NextColumn();
	ImGui::Text( "Total" ); ImGui::NextColumn();
	ImGui::Text( "" ); ImGui::NextColumn();
	ImGui::Separator();

	for ( const auto& cat : bridge.inventoryCategories )
	{
		ImGui::Text( "%s", cat.name.toStdString().c_str() ); ImGui::NextColumn();
		ImGui::Text( "%d", cat.countInStockpiles ); ImGui::NextColumn();
		ImGui::Text( "%d", cat.countTotal ); ImGui::NextColumn();
		ImGui::Text( "" ); ImGui::NextColumn();
	}

	ImGui::Columns( 1 );
	ImGui::End();
}

// =============================================================================
// Stockpile management
// =============================================================================
void drawStockpilePanel( ImGuiBridge& bridge )
{

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 130 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 370, io.DisplaySize.y - 200 ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Stockpile Management", &open, 0 );

	if ( !open )
	{
		bridge.cmdCloseStockpileWindow();
	}

	auto& sp = bridge.stockpileInfo;

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

	ImGui::EndChild();

	ImGui::EndTabItem();
	} // end Filters tab

	ImGui::EndTabBar();
	} // end TabBar

	ImGui::End();
}

// =============================================================================
// Population panel
// =============================================================================
void drawPopulationPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 50 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Population", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	if ( ImGui::BeginTabBar( "PopTabs" ) )
	{
		if ( ImGui::BeginTabItem( "Skills" ) )
		{
			// Column headers
			if ( !bridge.populationInfo.gnomes.isEmpty() && !bridge.populationInfo.gnomes[0].skills.isEmpty() )
			{
				ImGui::Columns( bridge.populationInfo.gnomes[0].skills.size() + 2 );
				ImGui::Text( "Name" ); ImGui::NextColumn();
				ImGui::Text( "Profession" ); ImGui::NextColumn();

				for ( const auto& skill : bridge.populationInfo.gnomes[0].skills )
				{
					// Vertical-ish text (abbreviated)
					ImGui::Text( "%s", skill.name.left( 4 ).toStdString().c_str() );
					ImGui::NextColumn();
				}
				ImGui::Separator();

				for ( const auto& gnome : bridge.populationInfo.gnomes )
				{
					ImGui::Text( "%s", gnome.name.toStdString().c_str() );
					ImGui::NextColumn();

					ImGui::Text( "%s", gnome.profession.toStdString().c_str() );
					ImGui::NextColumn();

					for ( const auto& skill : gnome.skills )
					{
						ImGui::PushID( ( QString::number( gnome.id ) + skill.sid ).toStdString().c_str() );
						bool active = skill.active;
						if ( ImGui::Checkbox( "", &active ) )
						{
							bridge.cmdSetSkillActive( gnome.id, skill.sid, active );
						}
						if ( ImGui::IsItemHovered() )
						{
							ImGui::SetTooltip( "%s: Level %d", skill.name.toStdString().c_str(), skill.level );
						}
						ImGui::PopID();
						ImGui::NextColumn();
					}
				}

				ImGui::Columns( 1 );
			}
			else
			{
				ImGui::TextDisabled( "No gnomes" );
			}

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Schedule" ) )
		{
			if ( bridge.scheduleInfo.schedules.isEmpty() )
			{
				bridge.cmdRequestSchedules();
				ImGui::TextDisabled( "Loading schedules..." );
			}
			else
			{
				ImGui::Columns( 26 );
				ImGui::SetColumnWidth( 0, 120.0f );
				for ( int col = 1; col <= 25; ++col )
					ImGui::SetColumnWidth( col, 28.0f );
				ImGui::Text( "Name" ); ImGui::NextColumn();
				for ( int h = 0; h < 24; ++h )
				{
					ImGui::Text( "%02d", h ); ImGui::NextColumn();
				}
				ImGui::Text( "" ); ImGui::NextColumn();
				ImGui::Separator();

				for ( const auto& gnome : bridge.scheduleInfo.schedules )
				{
					ImGui::Text( "%s", gnome.name.toStdString().c_str() ); ImGui::NextColumn();
					for ( int h = 0; h < 24; ++h )
					{
						ImGui::PushID( gnome.id * 100 + h );
						const char* label = "W";
						ImVec4 col( 0.3f, 0.3f, 0.3f, 1.0f );
						if ( h < gnome.schedule.size() )
						{
							switch ( gnome.schedule[h] )
							{
								case ScheduleActivity::None: label = "W"; col = ImVec4( 0.2f, 0.5f, 0.2f, 1.0f ); break;
								case ScheduleActivity::Eat: label = "E"; col = ImVec4( 0.7f, 0.4f, 0.0f, 1.0f ); break;
								case ScheduleActivity::Sleep: label = "S"; col = ImVec4( 0.0f, 0.3f, 0.7f, 1.0f ); break;
								case ScheduleActivity::Training: label = "T"; col = ImVec4( 0.7f, 0.0f, 0.0f, 1.0f ); break;
							}
						}
						ImGui::PushStyleColor( ImGuiCol_Button, col );
						if ( ImGui::SmallButton( label ) )
						{
							// Cycle through activities
							ScheduleActivity next = ScheduleActivity::None;
							if ( h < gnome.schedule.size() )
							{
								switch ( gnome.schedule[h] )
								{
									case ScheduleActivity::None: next = ScheduleActivity::Eat; break;
									case ScheduleActivity::Eat: next = ScheduleActivity::Sleep; break;
									case ScheduleActivity::Sleep: next = ScheduleActivity::Training; break;
									default: next = ScheduleActivity::None; break;
								}
							}
							bridge.cmdSetSchedule( gnome.id, h, next );
						}
						ImGui::PopStyleColor();
						ImGui::PopID();
						ImGui::NextColumn();
					}
					ImGui::Text( "" ); ImGui::NextColumn();
				}

				ImGui::Columns( 1 );
			}

			ImGui::EndTabItem();
		}

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

					// Traits section — only show notable traits (|value| > 25)
					ImGui::TextColored( ImVec4( 0.7f, 0.85f, 1.0f, 1.0f ), "Personality Traits" );
					ImGui::Indent( 10.0f );
					bool hasNotable = false;
					for ( const auto& trait : gnome.traits )
					{
						if ( trait.label.isEmpty() )
							continue; // not extreme enough to show

						hasNotable = true;
						// Color based on positive/negative
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

		if ( ImGui::BeginTabItem( "Social" ) )
		{
			for ( const auto& gnome : bridge.populationInfo.gnomes )
			{
				if ( gnome.relationships.isEmpty() ) continue;

				if ( ImGui::TreeNode( gnome.name.toStdString().c_str() ) )
				{
					for ( const auto& rel : gnome.relationships )
					{
						// Color: green for positive, red for negative, yellow for neutral
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
					ImGui::TreePop();
					ImGui::Separator();
				}
			}

			bool anyRelationships = false;
			for ( const auto& gnome : bridge.populationInfo.gnomes )
			{
				if ( !gnome.relationships.isEmpty() ) { anyRelationships = true; break; }
			}
			if ( !anyRelationships )
			{
				ImGui::TextDisabled( "No relationships yet — gnomes develop opinions over time" );
			}
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Professions" ) )
		{
			if ( bridge.professionList.isEmpty() )
			{
				bridge.cmdRequestProfessions();
			}

			if ( ImGui::Button( "New Profession" ) )
			{
				bridge.cmdNewProfession();
				bridge.cmdRequestProfessions();
			}

			for ( const auto& prof : bridge.professionList )
			{
				if ( ImGui::Selectable( prof.toStdString().c_str(), bridge.editingProfession == prof ) )
				{
					bridge.cmdRequestSkills( prof );
				}
			}

			if ( !bridge.editingProfession.isEmpty() && !bridge.professionSkills.isEmpty() )
			{
				ImGui::Separator();
				ImGui::Text( "Edit: %s", bridge.editingProfession.toStdString().c_str() );

				for ( auto& skill : bridge.professionSkills )
				{
					bool active = skill.active;
					if ( ImGui::Checkbox( skill.name.toStdString().c_str(), &active ) )
					{
						skill.active = active;
						// Collect all active skills and update
						QStringList activeSkills;
						for ( const auto& s : bridge.professionSkills )
						{
							if ( s.active ) activeSkills.append( s.sid );
						}
						bridge.cmdUpdateProfession( bridge.editingProfession, bridge.editingProfession, activeSkills );
					}
				}
			}

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
	ImGui::SetNextWindowPos( ImVec2( 5, 50 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Military", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	if ( ImGui::BeginTabBar( "MilTabs" ) )
	{
		if ( ImGui::BeginTabItem( "Squads" ) )
		{
			if ( ImGui::Button( "Add Squad" ) )
			{
				bridge.cmdAddSquad();
				bridge.cmdRequestMilitaryUpdate();
			}

			for ( const auto& squad : bridge.squads )
			{
				ImGui::PushID( squad.id );

				if ( ImGui::CollapsingHeader( squad.name.toStdString().c_str() ) )
				{
					char squadName[128];
					snprintf( squadName, sizeof( squadName ), "%s", squad.name.toStdString().c_str() );
					if ( ImGui::InputText( "Name", squadName, sizeof( squadName ), ImGuiInputTextFlags_EnterReturnsTrue ) )
					{
						bridge.cmdRenameSquad( squad.id, squadName );
					}

					for ( const auto& gnome : squad.gnomes )
					{
						ImGui::Text( "  %s", gnome.name.toStdString().c_str() );
						ImGui::SameLine();
						ImGui::PushID( (int)gnome.id );
						if ( ImGui::SmallButton( "Remove" ) )
						{
							bridge.cmdRemoveGnomeFromSquad( gnome.id );
							bridge.cmdRequestMilitaryUpdate();
						}
						ImGui::PopID();
					}

					// Add gnome to squad - show available gnomes from population
					if ( !bridge.populationInfo.gnomes.isEmpty() )
					{
						static int selectedGnome = 0;
						if ( ImGui::Button( "+ Add Gnome" ) )
						{
							if ( selectedGnome >= 0 && selectedGnome < bridge.populationInfo.gnomes.size() )
							{
								bridge.cmdMoveGnomeRight( bridge.populationInfo.gnomes[selectedGnome].id );
								bridge.cmdRequestMilitaryUpdate();
							}
						}
					}

					if ( ImGui::SmallButton( "Remove Squad" ) )
					{
						bridge.cmdRemoveSquad( squad.id );
						bridge.cmdRequestMilitaryUpdate();
					}
				}

				ImGui::PopID();
			}

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Roles" ) )
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
					bool civ = role.isCivilian;
					if ( ImGui::Checkbox( "Civilian", &civ ) )
					{
						bridge.cmdSetRoleCivilian( role.id, civ );
					}
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
	ImGui::SetNextWindowPos( ImVec2( 5, 50 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

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

void drawWorkshopPanel( ImGuiBridge& bridge )
{
	if ( !bridge.showWorkshopWindow )
		return;

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 130 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 400, io.DisplaySize.y - 200 ), ImGuiCond_FirstUseEver );

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
					if ( ImGui::SmallButton( "X" ) ) bridge.cmdWorkshopCraftJobCommand( job.id, "Delete" );

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
				// Craft mode + amount selector
				ImGui::Text( "Mode:" );
				ImGui::SameLine();
				ImGui::RadioButton( "Craft N", &s_craftMode, 0 );
				ImGui::SameLine();
				ImGui::RadioButton( "Until N", &s_craftMode, 1 );
				ImGui::SameLine();
				ImGui::RadioButton( "Repeat", &s_craftMode, 2 );

				if ( s_craftMode != 2 )
				{
					ImGui::SameLine();
					ImGui::SetNextItemWidth( 80 );
					ImGui::InputInt( "##amount", &s_craftAmount );
					if ( s_craftAmount < 1 ) s_craftAmount = 1;
				}

				ImGui::Separator();

				for ( const auto& product : ws.products )
				{
					ImGui::PushID( product.sid.toStdString().c_str() );

					ImGui::Text( "%s", product.sid.toStdString().c_str() );

					// Material dropdowns per component
					QStringList mats;
					bool canCraft = true;

					for ( int c = 0; c < product.components.size(); ++c )
					{
						const auto& comp = product.components[c];
						if ( comp.materials.isEmpty() )
						{
							canCraft = false;
							ImGui::TextDisabled( "  %d x %s (unavailable)", comp.amount, comp.sid.toStdString().c_str() );
							mats.append( "any" );
							continue;
						}

						int& selIdx = s_craftSelectedMats[product.sid][c];
						if ( selIdx >= comp.materials.size() ) selIdx = 0;

						ImGui::Text( "  %d", comp.amount );
						ImGui::SameLine();

						QString comboLabel = "##comp" + QString::number( c );
						QString preview = comp.materials[selIdx].sid + " (" + QString::number( comp.materials[selIdx].amount ) + ")";

						ImGui::SetNextItemWidth( 180 );
						if ( ImGui::BeginCombo( comboLabel.toStdString().c_str(), preview.toStdString().c_str() ) )
						{
							for ( int m = 0; m < comp.materials.size(); ++m )
							{
								QString label = comp.materials[m].sid + " (" + QString::number( comp.materials[m].amount ) + ")";
								if ( ImGui::Selectable( label.toStdString().c_str(), m == selIdx ) )
								{
									selIdx = m;
								}
							}
							ImGui::EndCombo();
						}

						mats.append( comp.materials[selIdx].sid );
					}

					// Craft button
					ImGui::SameLine();
					if ( !canCraft ) ImGui::BeginDisabled();
					if ( ImGui::Button( "Craft" ) )
					{
						bridge.cmdWorkshopCraftItem( product.sid, s_craftMode, QString::number( s_craftAmount ), mats );
					}
					if ( !canCraft ) ImGui::EndDisabled();

					ImGui::Separator();
					ImGui::PopID();
				}
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
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 340, 150 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 330, io.DisplaySize.y - 220 ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Creature Info", &open );

	if ( !open )
	{
		bridge.showCreatureInfo = false;
		bridge.activeSidePanel = ImGuiBridge::SidePanel::None;
	}

	auto& ci = bridge.creatureInfo;

	// Name and profession
	ImGui::TextColored( ImVec4( 1.0f, 0.9f, 0.6f, 1.0f ), "%s", ci.name.toStdString().c_str() );
	ImGui::Text( "%s", ci.profession.toStdString().c_str() );
	if ( !ci.activity.isEmpty() )
	{
		ImGui::TextColored( ImVec4( 0.6f, 0.8f, 0.6f, 1.0f ), "%s", ci.activity.toStdString().c_str() );
	}

	ImGui::Separator();

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

	// Stats (collapsed by default)
	if ( ImGui::CollapsingHeader( "Stats" ) )
	{
		ImGui::Text( "STR: %d  DEX: %d  CON: %d", ci.str, ci.dex, ci.con );
		ImGui::Text( "INT: %d  WIS: %d  CHA: %d", ci.intel, ci.wis, ci.cha );
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
void drawDebugPanel( ImGuiBridge& bridge )
{
	if ( !bridge.showDebugPanel )
		return;

	ImGui::SetNextWindowSize( ImVec2( 300, 200 ), ImGuiCond_FirstUseEver );

	ImGui::Begin( "Debug", &bridge.showDebugPanel );

	static char creatureType[64] = "Gnome";
	ImGui::InputText( "Creature type", creatureType, sizeof( creatureType ) );
	if ( ImGui::Button( "Spawn" ) )
	{
		bridge.cmdSpawnCreature( creatureType );
	}

	ImGui::End();
}

// =============================================================================
// Event Log panel
// =============================================================================
void drawEventLogPanel( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( 5, 50 ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( io.DisplaySize.x * 0.6f, io.DisplaySize.y * 0.7f ), ImGuiCond_FirstUseEver );

	bool open = true;
	ImGui::Begin( "Event Log", &open, 0 );

	if ( !open ) bridge.activeSidePanel = ImGuiBridge::SidePanel::None;

	// Filter buttons
	static bool showInfo = true, showWarning = true, showCombat = true, showDeath = true, showJobs = false, showDebug = false;
	ImGui::Checkbox( "Info", &showInfo ); ImGui::SameLine();
	ImGui::Checkbox( "Warning", &showWarning ); ImGui::SameLine();
	ImGui::Checkbox( "Combat", &showCombat ); ImGui::SameLine();
	ImGui::Checkbox( "Death", &showDeath ); ImGui::SameLine();
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
