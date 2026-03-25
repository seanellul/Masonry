#pragma once

#include "../base/logger.h"
#include "aggregatoragri.h"
#include "aggregatorcreatureinfo.h"
#include "aggregatordebug.h"
#include "aggregatorinventory.h"
#include "aggregatorloadgame.h"
#include "aggregatormilitary.h"
#include "aggregatorneighbors.h"
#include "aggregatorpopulation.h"
#include "aggregatorselection.h"
#include "aggregatorsettings.h"
#include "aggregatorstockpile.h"
#include "aggregatortileinfo.h"
#include "aggregatorworkshop.h"

#include "../base/enums.h"

#include <QObject>

class UpdateChecker;

class ImGuiBridge : public QObject
{
	Q_OBJECT

public:
	ImGuiBridge( QObject* parent = nullptr );

	// =========================================================================
	// UI State
	// =========================================================================
	enum class AppState
	{
		MainMenu,
		NewGame,
		LoadGame,
		Settings,
		WaitingForLoad,
		GameRunning,
		InGameMenu
	};
	AppState appState = AppState::MainMenu;

	// Loading screen status
	QString loadingStatus;
	bool rendererReady = false;
	int rendererWarmupFrames = 0; // counts up each frame in GameRunning until ready

	// Which sidebar panel is open (only one at a time)
	enum class SidePanel
	{
		None,
		Kingdom,
		Stockpile,
		Military,
		Population,
		Missions,
		TileInfo,
		Workshop,
		Agriculture,
		CreatureInfo,
		EventLog
	};
	SidePanel activeSidePanel = SidePanel::None;

	// Toast notification system
	struct ToastNotification
	{
		QString text;
		LogType severity;
		quint64 createdTick = 0;
		float alpha         = 1.0f;
		unsigned int entityID = 0;
		int eventPosX = 0, eventPosY = 0, eventPosZ = 0; // stored at log time
		bool dismissed = false;
	};
	QList<ToastNotification> activeToasts;
	quint64 lastLogCount = 0; // track new log entries for toast generation
	static constexpr int MAX_TOASTS = 5; // max visible toasts at once

	// Camera navigation request (set by UI, consumed by MainWindow)
	bool pendingCameraNav = false;
	Position cameraNavTarget;
	void cmdNavigateToEntity( unsigned int entityID );
	void cmdNavigateToPosition( Position pos );

	// Update checker
	UpdateChecker* updateChecker = nullptr;

	bool showDebugPanel = false;

	// Sprite texture cache for rendering game sprites in ImGui
	class SpriteTextureCache* spriteTexCache = nullptr;

	// =========================================================================
	// Game HUD data (from EventConnector / ProxyGameView signals)
	// =========================================================================
	// Time
	int minute = 0, hour = 0, day = 0, year = 0;
	QString season;
	QString sunStatus;

	// Kingdom info
	QString kingdomName;
	QString kingdomInfo1, kingdomInfo2, kingdomInfo3;

	// Stock counters (updated from kingdom info signals)
	int stockFood   = 0;
	int stockDrink  = 0;
	int stockGnomes = 0;
	int stockItems  = 0;

	int viewLevel = 0;

	// Pause / speed
	bool paused = false;
	GameSpeed gameSpeed = GameSpeed::Normal;

	// Render options
	bool renderDesignations = true;
	bool renderJobs = true;
	bool renderWalls = false;
	bool renderAxles = false;

	// Build items
	QList<GuiBuildItem> buildItems;
	ButtonSelection currentToolbar = ButtonSelection::None;
	BuildSelection currentBuildCategory = BuildSelection::None;
	QString currentBuildMaterial; // Subcategory key: material for Wall/Floor, tab for Workshop, group for Furniture, etc.

	// Watch list
	QList<GuiWatchedItem> watchList;

	// Events
	struct EventInfo
	{
		unsigned int id;
		QString title;
		QString msg;
		bool pause;
		bool yesno;
	};
	QList<EventInfo> pendingEvents;

	// Tile info
	unsigned int selectedTileID = 0;
	bool showTileInfo = false;
	GuiTileInfo tileInfo;
	GuiStockpileInfo tileStockpileInfo;

	// =========================================================================
	// Load / Save game data
	// =========================================================================
	QList<GuiSaveInfo> kingdoms;
	QList<GuiSaveInfo> saveGames;

	// =========================================================================
	// Settings data
	// =========================================================================
	GuiSettings settings;

	// =========================================================================
	// New game data
	// =========================================================================
	// (Managed directly in the new game UI — reads/writes to Config)

	// =========================================================================
	// Stockpile data
	// =========================================================================
	unsigned int activeStockpileID = 0;
	bool showStockpileWindow = false;
	GuiStockpileInfo stockpileInfo;

	// =========================================================================
	// Workshop data
	// =========================================================================
	unsigned int activeWorkshopID = 0;
	bool showWorkshopWindow = false;
	GuiWorkshopInfo workshopInfo;
	QList<GuiTradeItem> traderStock;
	QList<GuiTradeItem> playerStock;
	int traderValue = 0;
	int playerValue = 0;

	// =========================================================================
	// Agriculture data
	// =========================================================================
	unsigned int activeAgriID = 0;
	bool showAgriWindow = false;
	AgriType currentAgriType = AgriType::Farm;
	GuiFarmInfo farmInfo;
	GuiPastureInfo pastureInfo;
	GuiGroveInfo groveInfo;
	QList<GuiPlant> globalPlants;
	QList<GuiAnimal> globalAnimals;
	QList<GuiPlant> globalTrees;

	// =========================================================================
	// Population data
	// =========================================================================
	GuiPopulationInfo populationInfo;
	GuiScheduleInfo scheduleInfo;
	QStringList professionList;
	QList<GuiSkillInfo> professionSkills;
	QString editingProfession;

	// =========================================================================
	// Military data
	// =========================================================================
	QList<GuiSquad> squads;
	QList<GuiMilRole> roles;

	// =========================================================================
	// Creature info data
	// =========================================================================
	GuiCreatureInfo creatureInfo;
	QStringList creatureProfessionList;
	bool showCreatureInfo = false;

	// =========================================================================
	// Neighbor data
	// =========================================================================
	QList<GuiNeighborInfo> neighbors;
	QList<Mission> missions;
	QList<GuiAvailableGnome> availableGnomes;

	// =========================================================================
	// Inventory data
	// =========================================================================
	QList<GuiInventoryCategory> inventoryCategories;

	// =========================================================================
	// Commands (emit signals back to game)
	// =========================================================================
	// Main menu
	void cmdStartNewGame();
	void cmdContinueLastGame();
	void cmdLoadGame( const QString& path );
	void cmdSaveGame();
	void cmdEndGame();
	void cmdSetShowMainMenu( bool value );

	// Game HUD
	void cmdSetPaused( bool value );
	void cmdSetGameSpeed( GameSpeed speed );
	void cmdSetRenderOptions( bool d, bool j, bool w, bool a );
	void cmdPropagateEscape();
	void cmdSetSelectionAction( const QString& action );
	void cmdRequestBuildItems( BuildSelection sel, const QString& category );
	void cmdBuild( BuildItemType type, const QString& param, const QString& item, const QStringList& mats );
	void cmdEventAnswer( unsigned int eventID, bool answer );

	// Tile info
	void cmdTerrainCommand( unsigned int tileID, const QString& cmd );
	void cmdManageCommand( unsigned int tileID );
	void cmdRequestStockpileItems( unsigned int tileID );
	void cmdSetTennant( unsigned int designationID, unsigned int gnomeID );
	void cmdSetAlarm( unsigned int designationID, bool value );
	void cmdToggleMechActive( unsigned int id );
	void cmdSetRoomType( unsigned int designationID, RoomType type );
	void cmdToggleMechInvert( unsigned int id );
	void cmdSetAutomatonRefuel( unsigned int id, bool refuel );
	void cmdSetAutomatonCore( unsigned int id, const QString& core );

	// Stockpile
	void cmdStockpileSetActive( unsigned int spID, bool active, const QString& category, const QString& group, const QString& item, const QString& material );
	void cmdStockpileSetOptions( unsigned int spID, const QString& name, int priority, bool suspended, bool pullOthers, bool pullFrom );
	void cmdCloseStockpileWindow();

	// Workshop
	void cmdWorkshopSetOptions( unsigned int wsID, const QString& name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool linkStockpile );
	void cmdWorkshopCraftItem( const QString& sid, int mode, const QString& number, const QStringList& mats );
	void cmdWorkshopCraftJobParams( unsigned int jobID, int mode, const QString& number, bool suspended, bool moveBack );
	void cmdWorkshopCraftJobCommand( unsigned int jobID, const QString& cmd );
	void cmdWorkshopSetButcherOptions( unsigned int wsID, bool corpses, bool excess );
	void cmdWorkshopSetFisherOptions( unsigned int wsID, bool catchFish, bool processFish );
	void cmdWorkshopTraderOfferToStock( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount );
	void cmdWorkshopTraderStockToOffer( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount );
	void cmdWorkshopPlayerOfferToStock( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount );
	void cmdWorkshopPlayerStockToOffer( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount );
	void cmdWorkshopTrade( unsigned int wsID );
	void cmdWorkshopRequestAllTradeItems( unsigned int wsID );
	void cmdCloseWorkshopWindow();

	// Agriculture
	void cmdAgriSetOptions( unsigned int id, const QString& name, int priority, bool suspended );
	void cmdAgriSetHarvestOptions( unsigned int id, bool harvest, bool hay, bool tame );
	void cmdAgriSetGroveOptions( unsigned int id, bool pickFruits, bool plantTrees, bool fellTrees );
	void cmdAgriSelectProduct( unsigned int id, const QString& product );
	void cmdAgriSetMaxMale( unsigned int id, int max );
	void cmdAgriSetMaxFemale( unsigned int id, int max );
	void cmdAgriSetButchering( unsigned int id, bool value );
	void cmdAgriSetFoodItemChecked( const QString& item, const QString& material, bool checked );
	void cmdCloseAgriWindow();

	// Population
	void cmdRequestPopulationUpdate();
	void cmdSetSkillActive( unsigned int gnomeID, const QString& skillID, bool value );
	void cmdSetAllSkills( unsigned int gnomeID, bool value );
	void cmdSetAllGnomes( const QString& skillID, bool value );
	void cmdSetProfession( unsigned int gnomeID, const QString& profession );
	void cmdSortGnomes( const QString& mode );
	void cmdRequestSchedules();
	void cmdSetSchedule( unsigned int gnomeID, int hour, ScheduleActivity activity );
	void cmdSetAllHours( unsigned int gnomeID, ScheduleActivity activity );
	void cmdSetHourForAll( int hour, ScheduleActivity activity );
	void cmdRequestProfessions();
	void cmdRequestSkills( const QString& profession );
	void cmdUpdateProfession( const QString& name, const QString& newName, const QStringList& skills );
	void cmdDeleteProfession( const QString& name );
	void cmdNewProfession();

	// Military
	void cmdRequestMilitaryUpdate();
	void cmdAddSquad();
	void cmdRemoveSquad( unsigned int id );
	void cmdRenameSquad( unsigned int id, const QString& name );
	void cmdMoveSquadLeft( unsigned int id );
	void cmdMoveSquadRight( unsigned int id );
	void cmdRemoveGnomeFromSquad( unsigned int id );
	void cmdAddGnomeToSquad( unsigned int gnomeID, unsigned int squadID );
	void cmdMoveGnomeLeft( unsigned int id );
	void cmdMoveGnomeRight( unsigned int id );
	void cmdSetRole( unsigned int gnomeID, const QString& role );
	void cmdSetAttitude( unsigned int squadID, const QString& target, int attitude );
	void cmdMovePrioUp( unsigned int squadID, const QString& target );
	void cmdMovePrioDown( unsigned int squadID, const QString& target );
	void cmdRequestRoles();
	void cmdAddRole();
	void cmdRemoveRole( unsigned int id );
	void cmdRenameRole( unsigned int id, const QString& name );
	void cmdSetRoleCivilian( unsigned int id, bool value );
	void cmdSetArmorType( unsigned int roleID, const QString& slot, const QString& type, const QString& material );

	// Creature info
	void cmdRequestCreatureUpdate( unsigned int id );
	void cmdSetCreatureProfession( unsigned int id, const QString& profession );

	// Neighbors
	void cmdRequestNeighborsUpdate();
	void cmdRequestMissions();
	void cmdRequestAvailableGnomes();
	void cmdStartMission( int type, int action, unsigned int targetID, unsigned int gnomeID );

	// Inventory
	void cmdRequestInventoryCategories();
	void cmdInventorySetActive( bool active, const GuiWatchedItem& item );

	// Debug
	void cmdSpawnCreature( const QString& type );
	void cmdSetWindowSize( int w, int h );

	// Settings
	void cmdSetLanguage( const QString& lang );
	void cmdSetUIScale( float scale );
	void cmdSetFullScreen( bool value );
	void cmdSetKeyboardSpeed( float value );
	void cmdSetLightMin( float value );
	void cmdSetAudioMasterVolume( float value );

public slots:
	// Slots for receiving data from aggregators
	void onTimeAndDate( int minute, int hour, int day, QString season, int year, QString sunStatus );
	void onKingdomInfo( QString name, QString info1, QString info2, QString info3 );
	void onViewLevel( int level );
	void onUpdatePause( bool value );
	void onUpdateGameSpeed( GameSpeed speed );
	void onUpdateRenderOptions( bool d, bool j, bool w, bool a );
	void onBuild();
	void onBuildItems( const QList<GuiBuildItem>& items );
	void onWatchList( const QList<GuiWatchedItem>& items );
	void onKeyEsc();
	void onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno );
	void onOpenCreatureInfo( unsigned int creatureID );
	void onHeartbeat( int value );
	void onResume();
	void onLoadGameDone( bool value );
	void onLoadStatus( QString status );
	void onWindowSize( int w, int h );

	// Tile info
	void onShowTileInfo( unsigned int tileID );
	void onUpdateTileInfo( const GuiTileInfo& info );
	void onUpdateSPInfo( const GuiStockpileInfo& info );

	// Stockpile
	void onOpenStockpileWindow( unsigned int stockpileID );
	void onStockpileUpdateInfo( const GuiStockpileInfo& info );
	void onStockpileUpdateContent( const GuiStockpileInfo& info );

	// Workshop
	void onOpenWorkshopWindow( unsigned int workshopID );
	void onWorkshopUpdateInfo( const GuiWorkshopInfo& info );
	void onWorkshopUpdateContent( const GuiWorkshopInfo& info );
	void onWorkshopUpdateCraftList( const GuiWorkshopInfo& info );
	void onTraderStock( const QList<GuiTradeItem>& items );
	void onPlayerStock( const QList<GuiTradeItem>& items );
	void onUpdateTraderValue( int value );
	void onUpdatePlayerValue( int value );

	// Agriculture
	void onShowAgri( unsigned int id );
	void onUpdateFarm( const GuiFarmInfo& info );
	void onUpdatePasture( const GuiPastureInfo& info );
	void onUpdateGrove( const GuiGroveInfo& info );
	void onGlobalPlantInfo( const QList<GuiPlant>& info );
	void onGlobalAnimalInfo( const QList<GuiAnimal>& info );
	void onGlobalTreeInfo( const QList<GuiPlant>& info );

	// Population
	void onPopulationUpdate( const GuiPopulationInfo& info );
	void onScheduleUpdate( const GuiScheduleInfo& info );
	void onProfessionList( const QStringList& professions );
	void onProfessionSkills( QString profession, const QList<GuiSkillInfo>& skills );

	// Military
	void onSquads( const QList<GuiSquad>& squads );
	void onRoles( const QList<GuiMilRole>& roles );

	// Creature info
	void onCreatureUpdate( const GuiCreatureInfo& info );
	void onCreatureProfessionList( const QStringList& profs );

	// Neighbors
	void onNeighborsUpdate( const QList<GuiNeighborInfo>& infos );
	void onMissions( const QList<Mission>& missions );
	void onAvailableGnomes( const QList<GuiAvailableGnome>& gnomes );

	// Inventory
	void onInventoryCategories( const QList<GuiInventoryCategory>& categories );

	// Settings
	void onUpdateSettings( const GuiSettings& info );

	// Load game
	void onKingdoms( const QList<GuiSaveInfo>& kingdoms );
	void onSaveGames( const QList<GuiSaveInfo>& saves );

signals:
	// Internal signals for sending commands to EventConnector/Aggregators
	void signalStartNewGame();
	void signalContinueLastGame();
	void signalLoadGame( QString path );
	void signalSaveGame();
	void signalEndGame();
	void signalSetShowMainMenu( bool value );
	void signalSetPaused( bool value );
	void signalSetGameSpeed( GameSpeed speed );
	void signalSetRenderOptions( bool d, bool j, bool w, bool a );
	void signalPropagateEscape();
	void signalSetSelectionAction( QString action );
	void signalRequestBuildItems( BuildSelection sel, QString category );
	void signalCmdBuild( BuildItemType type, QString param, QString item, QStringList mats );
	void signalEventAnswer( unsigned int eventID, bool answer );
	void signalHeartbeatResponse( int value );
	void signalTerrainCommand( unsigned int tileID, QString cmd );
	void signalManageCommand( unsigned int tileID );
	void signalRequestStockpileItems( unsigned int tileID );
	void signalSetTennant( unsigned int designationID, unsigned int gnomeID );
	void signalSetAlarm( unsigned int designationID, bool value );
	void signalToggleMechActive( unsigned int id );
	void signalToggleMechInvert( unsigned int id );
	void signalSetAutomatonRefuel( unsigned int id, bool refuel );
	void signalSetAutomatonCore( unsigned int id, QString core );

private:
	int m_windowWidth = 1920;
	int m_windowHeight = 1080;
};
