#include "ui_mainmenu.h"
#include "../imguibridge.h"
#include "../eventconnector.h"

#include "../../base/config.h"
#include "../../base/global.h"
#include "../../base/db.h"

#include <imgui.h>
#include <QRandomGenerator>

void drawMainMenu( ImGuiBridge& bridge )
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 center( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f );

	ImGui::SetNextWindowPos( center, ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( 300, 300 ) );

	ImGui::Begin( "Ingnomia", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar );

	ImGui::SetCursorPosX( ( 300 - ImGui::CalcTextSize( "INGNOMIA" ).x ) * 0.5f );
	ImGui::TextUnformatted( "INGNOMIA" );
	ImGui::Separator();
	ImGui::Spacing();

	float buttonWidth = 260;
	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "New Game", ImVec2( buttonWidth, 40 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::NewGame;
	}

	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Continue", ImVec2( buttonWidth, 40 ) ) )
	{
		bridge.cmdContinueLastGame();
	}

	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Load Game", ImVec2( buttonWidth, 40 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::LoadGame;
		Global::eventConnector->aggregatorLoadGame()->onRequestKingdoms();
	}

	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Settings", ImVec2( buttonWidth, 40 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::Settings;
		Global::eventConnector->aggregatorSettings()->onRequestSettings();
	}

	ImGui::Spacing();
	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Exit", ImVec2( buttonWidth, 40 ) ) )
	{
		bridge.cmdEndGame();
	}

	ImGui::End();
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

			ImGui::SliderInt( "World size", &worldSize, 50, 200 );
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
			ImGui::TextWrapped( "Starting item configuration coming soon. Using defaults." );
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
	ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
	ImGui::SetNextWindowSize( ImVec2( 300, 250 ) );

	ImGui::Begin( "Paused", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );

	float buttonWidth = 260;

	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Resume", ImVec2( buttonWidth, 35 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::GameRunning;
		bridge.cmdSetShowMainMenu( false );
	}

	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Save Game", ImVec2( buttonWidth, 35 ) ) )
	{
		bridge.cmdSaveGame();
	}

	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Settings", ImVec2( buttonWidth, 35 ) ) )
	{
		bridge.appState = ImGuiBridge::AppState::Settings;
		Global::eventConnector->aggregatorSettings()->onRequestSettings();
	}

	ImGui::Spacing();
	ImGui::SetCursorPosX( ( 300 - buttonWidth ) * 0.5f );
	if ( ImGui::Button( "Exit to Menu", ImVec2( buttonWidth, 35 ) ) )
	{
		bridge.cmdEndGame();
	}

	ImGui::End();
}
