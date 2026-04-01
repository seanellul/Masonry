#include "ui_mainmenu.h"
#include "ui_theme.h"
#include "../imguibridge.h"
#include "../imgui_impl_qt5.h"
#include "../updatechecker.h"
#include "../eventconnector.h"

#include "../../base/config.h"
#include "../../base/global.h"
#include "../../base/db.h"
#include "../../base/io.h"
#include "../../game/newgamesettings.h"
#include "../../version.h"
#include "../strings.h"

#include <imgui.h>
#include <QRandomGenerator>
#include <QDesktopServices>
#include <QUrl>

void drawMainMenu( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 center( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f );
	auto& fonts = GetImGuiFonts();

	// Dark gradient background — derived from theme's window color
	ImDrawList* bg = ImGui::GetBackgroundDrawList();
	ImVec4 winBg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
	ImU32 topColor = IM_COL32( (int)(winBg.x * 180), (int)(winBg.y * 180), (int)(winBg.z * 180), 255 );
	ImU32 botColor = IM_COL32( (int)(winBg.x * 255), (int)(winBg.y * 255), (int)(winBg.z * 255), 255 );
	bg->AddRectFilledMultiColor( ImVec2( 0, 0 ), io.DisplaySize, topColor, topColor, botColor, botColor );

	// Subtle vignette overlay
	ImU32 vignetteColor = IM_COL32( 0, 0, 0, 60 );
	float vignetteW = io.DisplaySize.x * 0.15f;
	float vignetteH = io.DisplaySize.y * 0.15f;
	bg->AddRectFilledMultiColor( ImVec2( 0, 0 ), ImVec2( vignetteW, io.DisplaySize.y ),
		vignetteColor, IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 0 ), vignetteColor );
	bg->AddRectFilledMultiColor( ImVec2( io.DisplaySize.x - vignetteW, 0 ), io.DisplaySize,
		IM_COL32( 0, 0, 0, 0 ), vignetteColor, vignetteColor, IM_COL32( 0, 0, 0, 0 ) );
	bg->AddRectFilledMultiColor( ImVec2( 0, 0 ), ImVec2( io.DisplaySize.x, vignetteH ),
		vignetteColor, vignetteColor, IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 0 ) );
	bg->AddRectFilledMultiColor( ImVec2( 0, io.DisplaySize.y - vignetteH ), io.DisplaySize,
		IM_COL32( 0, 0, 0, 0 ), IM_COL32( 0, 0, 0, 0 ), vignetteColor, vignetteColor );

	// Responsive menu sizing
	float menuW = qBound( 300.0f, io.DisplaySize.x * 0.25f, 450.0f );
	float menuH = qBound( 400.0f, io.DisplaySize.y * 0.55f, 550.0f );
	float buttonWidth = menuW - 40.0f;
	float buttonHeight = qBound( 35.0f, menuH * 0.08f, 48.0f );
	float pad = ( menuW - buttonWidth ) * 0.5f;

	ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( menuW, menuH ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 20, 20 ) );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 1.0f );

	ImGui::Begin( "Masonry", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar );

	// Title
	if ( fonts.title )
		ImGui::PushFont( fonts.title );
	ImVec2 titleSize = ImGui::CalcTextSize( "MASONRY" );
	ImGui::SetCursorPosX( ( menuW - titleSize.x ) * 0.5f );
	ImGui::TextUnformatted( "MASONRY" );
	if ( fonts.title )
		ImGui::PopFont();

	// Tagline
	if ( fonts.uiSmall )
		ImGui::PushFont( fonts.uiSmall );
	const char* tagline = "A Colony Simulation";
	ImGui::SetCursorPosX( ( menuW - ImGui::CalcTextSize( tagline ).x ) * 0.5f );
	ImGui::TextDisabled( "%s", tagline );
	if ( fonts.uiSmall )
		ImGui::PopFont();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Push UI font for buttons and body text
	if ( fonts.ui )
		ImGui::PushFont( fonts.ui );

	// Update banner
	if ( bridge.updateChecker && bridge.updateChecker->updateAvailable() )
	{
		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.3f, 1.0f, 0.3f, 1.0f ) );
		QString updateText = QString( "Update available: v%1" ).arg( bridge.updateChecker->latestVersion() );
		QByteArray updateBytes = updateText.toUtf8();
		ImGui::SetCursorPosX( ( menuW - ImGui::CalcTextSize( updateBytes.constData() ).x ) * 0.5f );
		ImGui::TextUnformatted( updateBytes.constData() );
		ImGui::PopStyleColor();

		float dlBtnWidth = qMin( 180.0f, buttonWidth );
		ImGui::SetCursorPosX( ( menuW - dlBtnWidth ) * 0.5f );
		if ( ImGui::Button( "Download Update", ImVec2( dlBtnWidth, 28 ) ) )
		{
			QDesktopServices::openUrl( QUrl( bridge.updateChecker->downloadUrl() ) );
		}
		ImGui::Spacing();
	}

	// Check for existing saves (throttled to once per second)
	static bool hasSaves = false;
	static double lastCheck = -10.0;
	if ( ImGui::GetTime() - lastCheck > 1.0 )
	{
		hasSaves = IO::saveGameExists();
		lastCheck = ImGui::GetTime();
	}

	// Button styling
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 4.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 8, 8 ) );

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "New Game", ImVec2( buttonWidth, buttonHeight ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::NewGame;
	}

	if ( !hasSaves ) ImGui::BeginDisabled();
	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Continue", ImVec2( buttonWidth, buttonHeight ) ) )
	{
		bridge.cmdContinueLastGame();
	}
	if ( !hasSaves ) ImGui::EndDisabled();

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Load Game", ImVec2( buttonWidth, buttonHeight ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::LoadGame;
		Global::eventConnector->aggregatorLoadGame()->onRequestKingdoms();
	}

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Settings", ImVec2( buttonWidth, buttonHeight ) ) )
	{
		bridge.previousAppState = bridge.appState;
		bridge.appState = ImGuiBridge::AppState::Settings;
		Global::eventConnector->aggregatorSettings()->onRequestSettings();
	}

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Exit", ImVec2( buttonWidth, buttonHeight ) ) )
	{
		Global::eventConnector->onExit();
	}

	ImGui::PopStyleVar( 2 ); // FrameRounding, FramePadding

	if ( fonts.ui )
		ImGui::PopFont();

	// Version pinned to bottom
	if ( fonts.uiSmall )
		ImGui::PushFont( fonts.uiSmall );
	const char* ver = PROJECT_VERSION;
	float verHeight = ImGui::CalcTextSize( ver ).y;
	ImGui::SetCursorPosY( menuH - verHeight - 12.0f );
	ImGui::SetCursorPosX( ( menuW - ImGui::CalcTextSize( ver ).x ) * 0.5f );
	ImGui::TextDisabled( "%s", ver );
	if ( fonts.uiSmall )
		ImGui::PopFont();

	ImGui::End();

	ImGui::PopStyleVar( 2 );  // WindowPadding, WindowBorderSize
}

void drawNewGame( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( 600, 500 ) );

	ImGui::Begin( "Set up new game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

	static char kingdomName[128] = "The Light Kingdom";
	static char seed[128] = "";
	static int worldSize = 100;
	static int flatness = 8;
	static int oceanSize = 5;
	static int rivers = 0;
	static int riverSize = 1;
	static int startArea = 18;
	static int numGnomes = 1;
	static int treeDensity = 50;
	static int plantDensity = 50;
	static bool peaceful = false;
	static int maxPerType = 30;
	static int wildAnimals = 473;
	static bool initialized = false;

	if ( !initialized )
	{
		snprintf( seed, sizeof( seed ), "%d", QRandomGenerator::global()->generate() );
		auto cfg = Global::cfg;
		worldSize = cfg->get( "worldSize" ).toInt();
		if ( worldSize == 0 ) worldSize = 100;
		flatness = cfg->get( "flatness" ).toInt();
		if ( flatness == 0 ) flatness = 8;
		oceanSize = cfg->get( "oceanSize" ).toInt();
		rivers = cfg->get( "rivers" ).toInt();
		riverSize = cfg->get( "riverSize" ).toInt();
		if ( riverSize == 0 ) riverSize = 1;
		startArea = cfg->get( "startArea" ).toInt();
		if ( startArea == 0 ) startArea = 18;
		numGnomes = cfg->get( "numGnomes" ).toInt();
		if ( numGnomes == 0 ) numGnomes = 1;
		treeDensity = cfg->get( "treeDensity" ).toInt();
		if ( treeDensity == 0 ) treeDensity = 50;
		plantDensity = cfg->get( "plantDensity" ).toInt();
		if ( plantDensity == 0 ) plantDensity = 50;
		peaceful = cfg->get( "peaceful" ).toBool();
		maxPerType = cfg->get( "maxPerType" ).toInt();
		if ( maxPerType == 0 ) maxPerType = 30;
		wildAnimals = cfg->get( "wildAnimals" ).toInt();
		if ( wildAnimals == 0 ) wildAnimals = 473;
		initialized = true;
	}

	if ( ImGui::BeginTabBar( "NewGameTabs" ) )
	{
		if ( ImGui::BeginTabItem( "World" ) )
		{
			ImGui::InputText( "Kingdom name", kingdomName, sizeof( kingdomName ) );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Random##name" ) )
			{
				// Simple random name
				QStringList parts = { "The", "Great", "Light", "Dark", "Iron", "Golden", "Silver", "Crystal", "Ancient", "Lost" };
				QStringList parts2 = { "Kingdom", "Realm", "Domain", "Empire", "Republic", "Lands" };
				snprintf( kingdomName, sizeof( kingdomName ), "%s %s",
					parts[QRandomGenerator::global()->bounded( parts.size() )].toStdString().c_str(),
					parts2[QRandomGenerator::global()->bounded( parts2.size() )].toStdString().c_str() );
			}

			ImGui::InputText( "Seed", seed, sizeof( seed ) );
			ImGui::SameLine();
			if ( ImGui::SmallButton( "Random##seed" ) )
			{
				snprintf( seed, sizeof( seed ), "%d", QRandomGenerator::global()->generate() );
			}

			ImGui::SliderInt( "World size", &worldSize, 50, 400 );
			ImGui::SliderInt( "Flatness", &flatness, 1, 20 );
			ImGui::SliderInt( "Ocean size", &oceanSize, 0, 10 );
			ImGui::SliderInt( "Rivers", &rivers, 0, 5 );
			ImGui::SliderInt( "River size", &riverSize, 1, 5 );
			ImGui::SliderInt( "Start area", &startArea, 5, 50 );
			ImGui::SliderInt( "Gnomes", &numGnomes, 1, 10 );
			ImGui::SliderInt( "Tree density", &treeDensity, 0, 100 );
			ImGui::SliderInt( "Plant density", &plantDensity, 0, 100 );
			ImGui::Checkbox( "Peaceful", &peaceful );
			ImGui::SliderInt( "Wild animals", &wildAnimals, 0, 1000 );
			ImGui::SliderInt( "Max per type", &maxPerType, 1, 100 );

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Starting items" ) )
		{
			auto* ngs = Global::newGameSettings;

			// Cache item data from DB (once)
			struct ItemEntry
			{
				QString id;
				std::string name;
				QString category;
			};
			static QMap<QString, QList<ItemEntry>> itemsByCategory;
			static bool itemsCached = false;
			if ( !itemsCached )
			{
				auto rows = DB::selectRows( "Items" );
				for ( const auto& row : rows )
				{
					QString id = row.value( "ID" ).toString();
					QString cat = row.value( "Category" ).toString();
					QString translated = S::s( "$ItemName_" + id );
					if ( translated.startsWith( "$" ) )
						translated = id; // fallback if no translation
					itemsByCategory[cat].append( { id, translated.toStdString(), cat } );
				}
				// Sort items within each category
				for ( auto& list : itemsByCategory )
				{
					std::sort( list.begin(), list.end(), []( const ItemEntry& a, const ItemEntry& b )
					{
						return a.name < b.name;
					} );
				}
				itemsCached = true;
			}

			static char searchBuf[128] = "";
			static QString selectedItem;
			static int selectedMat1 = 0;
			static int selectedMat2 = 0;
			static int addAmount = 1;

			ImGui::InputTextWithHint( "##search", "Search items...", searchBuf, sizeof( searchBuf ) );
			ImGui::Separator();

			ImGui::Columns( 2, "startItemCols" );

			// ===== LEFT: Item picker =====
			ImGui::BeginChild( "ItemPicker", ImVec2( 0, 280 ), true );

			QString searchStr = QString( searchBuf ).toLower();

			for ( auto it = itemsByCategory.constBegin(); it != itemsByCategory.constEnd(); ++it )
			{
				// Filter: skip categories with no matching items
				bool hasMatch = false;
				if ( !searchStr.isEmpty() )
				{
					for ( const auto& item : it.value() )
					{
						if ( QString::fromStdString( item.name ).toLower().contains( searchStr ) )
						{
							hasMatch = true;
							break;
						}
					}
					if ( !hasMatch ) continue;
				}

				bool open = ImGui::TreeNode( it.key().toStdString().c_str() );
				if ( open )
				{
					for ( const auto& item : it.value() )
					{
						if ( !searchStr.isEmpty() &&
							 !QString::fromStdString( item.name ).toLower().contains( searchStr ) )
							continue;

						bool isSel = ( selectedItem == item.id );
						if ( ImGui::Selectable( item.name.c_str(), isSel ) )
						{
							selectedItem = item.id;
							selectedMat1 = 0;
							selectedMat2 = 0;
							addAmount = 1;
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::EndChild();

			// Selected item details
			if ( !selectedItem.isEmpty() )
			{
				ImGui::Separator();
				QString itemName = S::s( "$ItemName_" + selectedItem );
				if ( itemName.startsWith( "$" ) ) itemName = selectedItem;
				ImGui::Text( "Selected: %s", itemName.toStdString().c_str() );

				QStringList mats1, mats2;
				ngs->materialsForItem( selectedItem, mats1, mats2 );

				if ( !mats1.isEmpty() )
				{
					if ( selectedMat1 >= mats1.size() ) selectedMat1 = 0;
					QString mat1Label = S::s( "$MaterialName_" + mats1[selectedMat1] );
					if ( mat1Label.startsWith( "$" ) ) mat1Label = mats1[selectedMat1];

					if ( ImGui::BeginCombo( mats2.isEmpty() ? "Material" : "Material 1", mat1Label.toStdString().c_str() ) )
					{
						for ( int i = 0; i < mats1.size(); ++i )
						{
							QString label = S::s( "$MaterialName_" + mats1[i] );
							if ( label.startsWith( "$" ) ) label = mats1[i];
							if ( ImGui::Selectable( label.toStdString().c_str(), selectedMat1 == i ) )
								selectedMat1 = i;
						}
						ImGui::EndCombo();
					}
				}

				if ( !mats2.isEmpty() )
				{
					if ( selectedMat2 >= mats2.size() ) selectedMat2 = 0;
					QString mat2Label = S::s( "$MaterialName_" + mats2[selectedMat2] );
					if ( mat2Label.startsWith( "$" ) ) mat2Label = mats2[selectedMat2];

					if ( ImGui::BeginCombo( "Material 2", mat2Label.toStdString().c_str() ) )
					{
						for ( int i = 0; i < mats2.size(); ++i )
						{
							QString label = S::s( "$MaterialName_" + mats2[i] );
							if ( label.startsWith( "$" ) ) label = mats2[i];
							if ( ImGui::Selectable( label.toStdString().c_str(), selectedMat2 == i ) )
								selectedMat2 = i;
						}
						ImGui::EndCombo();
					}
				}

				ImGui::InputInt( "Amount", &addAmount );
				if ( addAmount < 1 ) addAmount = 1;

				ImGui::SameLine();
				if ( ImGui::Button( "Add" ) && !mats1.isEmpty() )
				{
					QString mat1 = mats1[selectedMat1];
					QString mat2 = mats2.isEmpty() ? "" : mats2[selectedMat2];
					ngs->addStartingItem( selectedItem, mat1, mat2, addAmount );
				}
			}

			// ===== RIGHT: Current starting items =====
			ImGui::NextColumn();

			auto items = ngs->startingItems();
			ImGui::Text( "Starting Items (%d)", items.size() );
			ImGui::Separator();

			ImGui::BeginChild( "CurrentItems", ImVec2( 0, 0 ), true );

			for ( int i = 0; i < items.size(); ++i )
			{
				const auto& si = items[i];
				QString itemName = S::s( "$ItemName_" + si.itemSID );
				if ( itemName.startsWith( "$" ) ) itemName = si.itemSID;
				QString matName = S::s( "$MaterialName_" + si.mat1 );
				if ( matName.startsWith( "$" ) ) matName = si.mat1;
				if ( !si.mat2.isEmpty() )
				{
					QString mat2Name = S::s( "$MaterialName_" + si.mat2 );
					if ( mat2Name.startsWith( "$" ) ) mat2Name = si.mat2;
					matName += " / " + mat2Name;
				}

				ImGui::PushID( i );
				ImGui::Text( "%s (%s) x%d", itemName.toStdString().c_str(),
					matName.toStdString().c_str(), si.amount );
				ImGui::SameLine();
				if ( ImGui::SmallButton( "X" ) )
				{
					QString tag = si.itemSID + "_" + si.mat1;
					if ( !si.mat2.isEmpty() ) tag += "_" + si.mat2;
					ngs->removeStartingItem( tag );
				}
				ImGui::PopID();
			}
			ImGui::EndChild();

			ImGui::Columns( 1 );
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Enemies" ) )
		{
			ImGui::TextWrapped( "Enemy configuration coming soon. Using defaults." );
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Flora" ) )
		{
			ImGui::TextWrapped( "Flora configuration coming soon. Using defaults." );
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Fauna" ) )
		{
			ImGui::TextWrapped( "Fauna configuration coming soon. Using defaults." );
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if ( ImGui::Button( "Back", ImVec2( 120, 35 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::MainMenu;
	}

	ImGui::SameLine( 0, 330 );

	if ( ImGui::Button( "Embark", ImVec2( 120, 35 ) ) )
	{
		auto cfg = Global::cfg;
		cfg->set( "kingdomName", QString( kingdomName ) );
		cfg->set( "seed", QString( seed ) );
		cfg->set( "worldSize", worldSize );
		cfg->set( "flatness", flatness );
		cfg->set( "oceanSize", oceanSize );
		cfg->set( "rivers", rivers );
		cfg->set( "riverSize", riverSize );
		cfg->set( "startArea", startArea );
		cfg->set( "numGnomes", numGnomes );
		cfg->set( "treeDensity", treeDensity );
		cfg->set( "plantDensity", plantDensity );
		cfg->set( "peaceful", peaceful );
		cfg->set( "maxPerType", maxPerType );
		cfg->set( "wildAnimals", wildAnimals );

		initialized = false;
		bridge.cmdStartNewGame();
	}

	ImGui::End();
}

void drawLoadGame( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( 600, 400 ) );

	ImGui::Begin( "Load Game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

	static int selectedKingdom = -1;
	static int selectedSave = -1;

	ImGui::Columns( 2 );

	ImGui::Text( "Kingdoms" );
	ImGui::Separator();
	for ( int i = 0; i < bridge.kingdoms.size(); ++i )
	{
		bool selected = ( selectedKingdom == i );
		if ( ImGui::Selectable( bridge.kingdoms[i].name.toStdString().c_str(), selected ) )
		{
			selectedKingdom = i;
			selectedSave = -1;
			Global::eventConnector->aggregatorLoadGame()->onRequestSaveGames( bridge.kingdoms[i].folder );
		}
	}

	ImGui::NextColumn();

	ImGui::Text( "Saves" );
	ImGui::Separator();
	for ( int i = 0; i < bridge.saveGames.size(); ++i )
	{
		bool selected = ( selectedSave == i );
		QString label = bridge.saveGames[i].name + " - " + bridge.saveGames[i].date.toString();
		if ( ImGui::Selectable( label.toStdString().c_str(), selected ) )
		{
			selectedSave = i;
		}
	}

	ImGui::Columns( 1 );

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if ( ImGui::Button( "Back", ImVec2( 120, 35 ) ) )
	{
		selectedKingdom = -1;
		selectedSave = -1;
		bridge.appState = ImGuiBridge::AppState::MainMenu;
	}

	ImGui::SameLine( 0, 330 );

	bool canLoad = selectedSave >= 0 && selectedSave < bridge.saveGames.size();
	if ( !canLoad ) ImGui::BeginDisabled();
	if ( ImGui::Button( "Load", ImVec2( 120, 35 ) ) )
	{
		bridge.cmdLoadGame( bridge.saveGames[selectedSave].folder );
		selectedKingdom = -1;
		selectedSave = -1;
	}
	if ( !canLoad ) ImGui::EndDisabled();

	ImGui::End();
}

void drawSettings( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( 500, 350 ) );

	ImGui::Begin( "Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

	if ( ImGui::BeginTabBar( "SettingsTabs" ) )
	{
		if ( ImGui::BeginTabItem( "Game" ) )
		{
			static bool fullscreen = Global::cfg->get( "fullscreen" ).toBool();
			if ( ImGui::Checkbox( "Fullscreen", &fullscreen ) )
			{
				bridge.cmdSetFullScreen( fullscreen );
			}

			static float lightMin = Global::cfg->get( "lightMin" ).toFloat();
			if ( ImGui::SliderFloat( "Minimum light", &lightMin, 0.0f, 1.0f ) )
			{
				bridge.cmdSetLightMin( lightMin );
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// UI Theme selector
			int currentTheme = (int)GetActiveUITheme();
			ImGui::Text( "UI Theme" );
			ImGui::SameLine( 120 );
			ImGui::SetNextItemWidth( 200 );
			if ( ImGui::Combo( "##uitheme", &currentTheme, UIThemeNames, (int)UITheme::COUNT ) )
			{
				SetActiveUITheme( (UITheme)currentTheme );
			}

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Controls" ) )
		{
			static float kbSpeed = Global::cfg->get( "keyboardMoveSpeed" ).toFloat();
			if ( ImGui::SliderFloat( "Keyboard speed", &kbSpeed, 0.0f, 100.0f ) )
			{
				bridge.cmdSetKeyboardSpeed( kbSpeed );
			}

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Audio" ) )
		{
			static float masterVol = Global::cfg->get( "AudioMasterVolume" ).toFloat();
			if ( ImGui::SliderFloat( "Master volume", &masterVol, 0.0f, 100.0f ) )
			{
				bridge.cmdSetAudioMasterVolume( masterVol );
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if ( ImGui::Button( "Back", ImVec2( 120, 35 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::MainMenu;
	}

	ImGui::End();
}

void drawWaitScreen( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	float panelW = 400;
	float panelH = 120;
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( panelW, panelH ) );

	ImGui::Begin( "Loading", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar );

	// Title
	const char* title = "Loading...";
	ImGui::SetCursorPosX( ( panelW - ImGui::CalcTextSize( title ).x ) * 0.5f );
	ImGui::TextUnformatted( title );
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Status text from game thread
	if ( !bridge.loadingStatus.isEmpty() )
	{
		std::string status = bridge.loadingStatus.toStdString();
		ImGui::SetCursorPosX( ( panelW - ImGui::CalcTextSize( status.c_str() ).x ) * 0.5f );
		ImGui::TextUnformatted( status.c_str() );
	}
	else
	{
		ImGui::SetCursorPosX( ( panelW - ImGui::CalcTextSize( "Preparing..." ).x ) * 0.5f );
		ImGui::TextDisabled( "Preparing..." );
	}

	ImGui::End();
}

void drawInGameMenu( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	float menuW = 320;
	float menuH = 340;
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( menuW, menuH ) );

	ImGui::Begin( "Paused", nullptr,
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

	float buttonWidth = menuW - 40;
	float pad = ( menuW - buttonWidth ) * 0.5f;

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Resume", ImVec2( buttonWidth, 38 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::GameRunning;
		bridge.cmdSetShowMainMenu( false );
	}

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Save Game", ImVec2( buttonWidth, 38 ) ) )
	{
		bridge.cmdSaveGame();
	}

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Settings", ImVec2( buttonWidth, 38 ) ) )
	{
		bridge.previousAppState = bridge.appState;
		bridge.appState = ImGuiBridge::AppState::Settings;
		Global::eventConnector->aggregatorSettings()->onRequestSettings();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Save & Exit", ImVec2( buttonWidth, 38 ) ) )
	{
		bridge.cmdSaveGame();
		bridge.cmdEndGame();
	}

	ImGui::SetCursorPosX( pad );
	if ( ImGui::Button( "Exit to Menu", ImVec2( buttonWidth, 38 ) ) )
	{
		bridge.cmdEndGame();
	}

	ImGui::End();
}
