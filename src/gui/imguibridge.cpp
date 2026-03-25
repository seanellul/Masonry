#include "imguibridge.h"

#include "../base/global.h"
#include "eventconnector.h"

ImGuiBridge::ImGuiBridge( QObject* parent )
	: QObject( parent )
{
	auto* ec = Global::eventConnector;

	// =========================================================================
	// Receive data from EventConnector
	// =========================================================================
	connect( ec, &EventConnector::signalTimeAndDate, this, &ImGuiBridge::onTimeAndDate, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalKingdomInfo, this, &ImGuiBridge::onKingdomInfo, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalViewLevel, this, &ImGuiBridge::onViewLevel, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalUpdatePause, this, &ImGuiBridge::onUpdatePause, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalUpdateGameSpeed, this, &ImGuiBridge::onUpdateGameSpeed, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalUpdateRenderOptions, this, &ImGuiBridge::onUpdateRenderOptions, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalBuild, this, &ImGuiBridge::onBuild, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalKeyEsc, this, &ImGuiBridge::onKeyEsc, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalEvent, this, &ImGuiBridge::onEvent, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalOpenCreatureInfo, this, &ImGuiBridge::onOpenCreatureInfo, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalHeartbeat, this, &ImGuiBridge::onHeartbeat, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalResume, this, &ImGuiBridge::onResume, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalLoadGameDone, this, &ImGuiBridge::onLoadGameDone, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalLoadStatus, this, &ImGuiBridge::onLoadStatus, Qt::QueuedConnection );
	connect( ec, &EventConnector::signalWindowSize, this, &ImGuiBridge::onWindowSize, Qt::QueuedConnection );

	// =========================================================================
	// Send commands to EventConnector
	// =========================================================================
	connect( this, &ImGuiBridge::signalStartNewGame, ec, &EventConnector::onStartNewGame, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalContinueLastGame, ec, &EventConnector::onContinueLastGame, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalLoadGame, ec, &EventConnector::onLoadGame, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSaveGame, ec, &EventConnector::onSaveGame, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalEndGame, ec, &EventConnector::onEndGame, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetShowMainMenu, ec, &EventConnector::onSetShowMainMenu, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetPaused, ec, &EventConnector::onSetPause, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetGameSpeed, ec, &EventConnector::onSetGameSpeed, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetRenderOptions, ec, &EventConnector::onSetRenderOptions, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalPropagateEscape, ec, &EventConnector::onPropagateEscape );
	connect( this, &ImGuiBridge::signalSetSelectionAction, ec, &EventConnector::onSetSelectionAction, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalCmdBuild, ec, &EventConnector::onCmdBuild, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalEventAnswer, ec, &EventConnector::onAnswer, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalHeartbeatResponse, ec, &EventConnector::onHeartbeatResponse, Qt::QueuedConnection );

	// =========================================================================
	// Build items from inventory aggregator
	// =========================================================================
	connect( this, &ImGuiBridge::signalRequestBuildItems, ec->aggregatorInventory(), &AggregatorInventory::onRequestBuildItems, Qt::QueuedConnection );
	connect( ec->aggregatorInventory(), &AggregatorInventory::signalBuildItems, this, &ImGuiBridge::onBuildItems, Qt::QueuedConnection );
	connect( ec->aggregatorInventory(), &AggregatorInventory::signalWatchList, this, &ImGuiBridge::onWatchList, Qt::QueuedConnection );

	// =========================================================================
	// Tile info
	// =========================================================================
	connect( ec->aggregatorTileInfo(), &AggregatorTileInfo::signalShowTileInfo, this, &ImGuiBridge::onShowTileInfo, Qt::QueuedConnection );
	connect( ec->aggregatorTileInfo(), &AggregatorTileInfo::signalUpdateTileInfo, this, &ImGuiBridge::onUpdateTileInfo, Qt::QueuedConnection );
	connect( ec->aggregatorTileInfo(), &AggregatorTileInfo::signalUpdateSPInfo, this, &ImGuiBridge::onUpdateSPInfo, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalTerrainCommand, ec, &EventConnector::onTerrainCommand, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalManageCommand, ec, &EventConnector::onManageCommand, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalRequestStockpileItems, ec->aggregatorTileInfo(), &AggregatorTileInfo::onRequestStockpileItems, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetTennant, ec->aggregatorTileInfo(), &AggregatorTileInfo::onSetTennant, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetAlarm, ec->aggregatorTileInfo(), &AggregatorTileInfo::onSetAlarm, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalToggleMechActive, ec->aggregatorTileInfo(), &AggregatorTileInfo::onToggleMechActive, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalToggleMechInvert, ec->aggregatorTileInfo(), &AggregatorTileInfo::onToggleMechInvert, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetAutomatonRefuel, ec->aggregatorTileInfo(), &AggregatorTileInfo::onSetAutomatonRefuel, Qt::QueuedConnection );
	connect( this, &ImGuiBridge::signalSetAutomatonCore, ec->aggregatorTileInfo(), &AggregatorTileInfo::onSetAutomatonCore, Qt::QueuedConnection );

	// =========================================================================
	// Stockpile
	// =========================================================================
	connect( ec->aggregatorStockpile(), &AggregatorStockpile::signalOpenStockpileWindow, this, &ImGuiBridge::onOpenStockpileWindow, Qt::QueuedConnection );
	connect( ec->aggregatorStockpile(), &AggregatorStockpile::signalUpdateInfo, this, &ImGuiBridge::onStockpileUpdateInfo, Qt::QueuedConnection );
	connect( ec->aggregatorStockpile(), &AggregatorStockpile::signalUpdateContent, this, &ImGuiBridge::onStockpileUpdateContent, Qt::QueuedConnection );

	// =========================================================================
	// Workshop
	// =========================================================================
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalOpenWorkshopWindow, this, &ImGuiBridge::onOpenWorkshopWindow, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateInfo, this, &ImGuiBridge::onWorkshopUpdateInfo, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateContent, this, &ImGuiBridge::onWorkshopUpdateContent, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateCraftList, this, &ImGuiBridge::onWorkshopUpdateCraftList, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalTraderStock, this, &ImGuiBridge::onTraderStock, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalPlayerStock, this, &ImGuiBridge::onPlayerStock, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdateTraderValue, this, &ImGuiBridge::onUpdateTraderValue, Qt::QueuedConnection );
	connect( ec->aggregatorWorkshop(), &AggregatorWorkshop::signalUpdatePlayerValue, this, &ImGuiBridge::onUpdatePlayerValue, Qt::QueuedConnection );

	// =========================================================================
	// Agriculture
	// =========================================================================
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalShowAgri, this, &ImGuiBridge::onShowAgri, Qt::QueuedConnection );
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalUpdateFarm, this, &ImGuiBridge::onUpdateFarm, Qt::QueuedConnection );
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalUpdatePasture, this, &ImGuiBridge::onUpdatePasture, Qt::QueuedConnection );
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalUpdateGrove, this, &ImGuiBridge::onUpdateGrove, Qt::QueuedConnection );
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalGlobalPlantInfo, this, &ImGuiBridge::onGlobalPlantInfo, Qt::QueuedConnection );
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalGlobalAnimalInfo, this, &ImGuiBridge::onGlobalAnimalInfo, Qt::QueuedConnection );
	connect( ec->aggregatorAgri(), &AggregatorAgri::signalGlobalTreeInfo, this, &ImGuiBridge::onGlobalTreeInfo, Qt::QueuedConnection );

	// =========================================================================
	// Population
	// =========================================================================
	connect( ec->aggregatorPopulation(), &AggregatorPopulation::signalPopulationUpdate, this, &ImGuiBridge::onPopulationUpdate, Qt::QueuedConnection );
	connect( ec->aggregatorPopulation(), &AggregatorPopulation::signalScheduleUpdate, this, &ImGuiBridge::onScheduleUpdate, Qt::QueuedConnection );
	connect( ec->aggregatorPopulation(), &AggregatorPopulation::signalProfessionList, this, &ImGuiBridge::onProfessionList, Qt::QueuedConnection );
	connect( ec->aggregatorPopulation(), &AggregatorPopulation::signalProfessionSkills, this, &ImGuiBridge::onProfessionSkills, Qt::QueuedConnection );

	// =========================================================================
	// Military
	// =========================================================================
	connect( ec->aggregatorMilitary(), &AggregatorMilitary::signalSquads, this, &ImGuiBridge::onSquads, Qt::QueuedConnection );
	connect( ec->aggregatorMilitary(), &AggregatorMilitary::signalRoles, this, &ImGuiBridge::onRoles, Qt::QueuedConnection );

	// =========================================================================
	// Creature info
	// =========================================================================
	connect( ec->aggregatorCreatureInfo(), &AggregatorCreatureInfo::signalCreatureUpdate, this, &ImGuiBridge::onCreatureUpdate, Qt::QueuedConnection );
	connect( ec->aggregatorCreatureInfo(), &AggregatorCreatureInfo::signalProfessionList, this, &ImGuiBridge::onCreatureProfessionList, Qt::QueuedConnection );

	// =========================================================================
	// Neighbors
	// =========================================================================
	connect( ec->aggregatorNeighbors(), &AggregatorNeighbors::signalNeighborsUpdate, this, &ImGuiBridge::onNeighborsUpdate, Qt::QueuedConnection );
	connect( ec->aggregatorNeighbors(), &AggregatorNeighbors::signalMissions, this, &ImGuiBridge::onMissions, Qt::QueuedConnection );
	connect( ec->aggregatorNeighbors(), &AggregatorNeighbors::signalAvailableGnomes, this, &ImGuiBridge::onAvailableGnomes, Qt::QueuedConnection );

	// =========================================================================
	// Inventory
	// =========================================================================
	connect( ec->aggregatorInventory(), &AggregatorInventory::signalInventoryCategories, this, &ImGuiBridge::onInventoryCategories, Qt::QueuedConnection );

	// =========================================================================
	// Settings
	// =========================================================================
	connect( ec->aggregatorSettings(), &AggregatorSettings::signalUpdateSettings, this, &ImGuiBridge::onUpdateSettings, Qt::QueuedConnection );

	// =========================================================================
	// Load game
	// =========================================================================
	connect( ec->aggregatorLoadGame(), &AggregatorLoadGame::signalKingdoms, this, &ImGuiBridge::onKingdoms, Qt::QueuedConnection );
	connect( ec->aggregatorLoadGame(), &AggregatorLoadGame::signalSaveGames, this, &ImGuiBridge::onSaveGames, Qt::QueuedConnection );
}

// =============================================================================
// Slot implementations — store received data
// =============================================================================

void ImGuiBridge::onTimeAndDate( int min, int hr, int d, QString s, int y, QString sun )
{
	minute = min; hour = hr; day = d; season = s; year = y; sunStatus = sun;
}

void ImGuiBridge::onKingdomInfo( QString name, QString i1, QString i2, QString i3 )
{
	kingdomName = name; kingdomInfo1 = i1; kingdomInfo2 = i2; kingdomInfo3 = i3;

	// Parse stock counters from kingdom info strings
	auto parseCount = []( const QString& s ) -> int {
		int idx = s.indexOf( ':' );
		if ( idx >= 0 ) return s.mid( idx + 1 ).trimmed().toInt();
		return 0;
	};
	stockGnomes = parseCount( i1 );
	stockItems  = parseCount( i3 );

	// i2 format: "Food: N | Drink: N"
	QStringList parts = i2.split( '|' );
	if ( parts.size() >= 2 )
	{
		stockFood  = parseCount( parts[0] );
		stockDrink = parseCount( parts[1] );
	}
}

void ImGuiBridge::onViewLevel( int level ) { viewLevel = level; }
void ImGuiBridge::onUpdatePause( bool value ) { paused = value; }
void ImGuiBridge::onUpdateGameSpeed( GameSpeed speed ) { gameSpeed = speed; }

void ImGuiBridge::onUpdateRenderOptions( bool d, bool j, bool w, bool a )
{
	renderDesignations = d; renderJobs = j; renderWalls = w; renderAxles = a;
}

void ImGuiBridge::onBuild() { /* Build mode activated — toolbar handles this */ }

void ImGuiBridge::onBuildItems( const QList<GuiBuildItem>& items ) { buildItems = items; }
void ImGuiBridge::onWatchList( const QList<GuiWatchedItem>& items ) { watchList = items; }

void ImGuiBridge::onKeyEsc()
{
	if ( appState == AppState::GameRunning )
	{
		// Priority 1: If a build/action tool is active, clear it first
		if ( currentToolbar != ButtonSelection::None || currentBuildCategory != BuildSelection::None )
		{
			currentToolbar = ButtonSelection::None;
			currentBuildCategory = BuildSelection::None;
			currentBuildMaterial.clear();
			buildItems.clear();
			cmdPropagateEscape();
		}
		// Priority 2: Close side panels
		else if ( activeSidePanel != SidePanel::None )
		{
			activeSidePanel = SidePanel::None;
			cmdPropagateEscape();
		}
		// Priority 3: Only open pause menu when nothing else to dismiss
		else
		{
			appState = AppState::InGameMenu;
			cmdSetShowMainMenu( true );
		}
	}
	else if ( appState == AppState::InGameMenu )
	{
		appState = AppState::GameRunning;
		cmdSetShowMainMenu( false );
	}
	else if ( appState == AppState::NewGame || appState == AppState::LoadGame || appState == AppState::Settings )
	{
		appState = AppState::MainMenu;
	}
}

void ImGuiBridge::onEvent( unsigned int id, QString title, QString msg, bool pause, bool yesno )
{
	pendingEvents.append( { id, title, msg, pause, yesno } );
}

void ImGuiBridge::onOpenCreatureInfo( unsigned int creatureID )
{
	cmdRequestCreatureUpdate( creatureID );
	activeSidePanel = SidePanel::CreatureInfo;
}

void ImGuiBridge::onHeartbeat( int value )
{
	emit signalHeartbeatResponse( value );
}

void ImGuiBridge::onResume()
{
	appState = AppState::GameRunning;
	cmdSetShowMainMenu( false );
}

void ImGuiBridge::onLoadGameDone( bool value )
{
	if ( value )
	{
		appState = AppState::GameRunning;
		rendererReady = false;
		rendererWarmupFrames = 0;
		loadingStatus = "Initializing renderer...";
		cmdSetShowMainMenu( false );
	}
	else
	{
		appState = AppState::MainMenu;
	}
}

void ImGuiBridge::onLoadStatus( QString status )
{
	loadingStatus = status;
}

void ImGuiBridge::onWindowSize( int w, int h )
{
	m_windowWidth = w; m_windowHeight = h;
}

// Tile info
void ImGuiBridge::onShowTileInfo( unsigned int tileID )
{
	selectedTileID = tileID;
	showTileInfo = true;
	activeSidePanel = SidePanel::TileInfo;
}

void ImGuiBridge::onUpdateTileInfo( const GuiTileInfo& info ) { tileInfo = info; }
void ImGuiBridge::onUpdateSPInfo( const GuiStockpileInfo& info ) { tileStockpileInfo = info; }

// Stockpile
void ImGuiBridge::onOpenStockpileWindow( unsigned int id )
{
	activeStockpileID = id;
	showStockpileWindow = true;
	activeSidePanel = SidePanel::Stockpile;
}

void ImGuiBridge::onStockpileUpdateInfo( const GuiStockpileInfo& info ) { stockpileInfo = info; }
void ImGuiBridge::onStockpileUpdateContent( const GuiStockpileInfo& info ) { stockpileInfo = info; }

// Workshop
void ImGuiBridge::onOpenWorkshopWindow( unsigned int id )
{
	activeWorkshopID = id;
	showWorkshopWindow = true;
	activeSidePanel = SidePanel::Workshop;
}

void ImGuiBridge::onWorkshopUpdateInfo( const GuiWorkshopInfo& info ) { workshopInfo = info; }
void ImGuiBridge::onWorkshopUpdateContent( const GuiWorkshopInfo& info ) { workshopInfo = info; }
void ImGuiBridge::onWorkshopUpdateCraftList( const GuiWorkshopInfo& info ) { workshopInfo = info; }
void ImGuiBridge::onTraderStock( const QList<GuiTradeItem>& items ) { traderStock = items; }
void ImGuiBridge::onPlayerStock( const QList<GuiTradeItem>& items ) { playerStock = items; }
void ImGuiBridge::onUpdateTraderValue( int value ) { traderValue = value; }
void ImGuiBridge::onUpdatePlayerValue( int value ) { playerValue = value; }

// Agriculture
void ImGuiBridge::onShowAgri( unsigned int id )
{
	activeAgriID = id;
	showAgriWindow = true;
	activeSidePanel = SidePanel::Agriculture;

	// Determine type from which info struct was just updated
	if ( farmInfo.ID != 0 )
		currentAgriType = AgriType::Farm;
	else if ( pastureInfo.ID != 0 )
		currentAgriType = AgriType::Pasture;
	else if ( groveInfo.ID != 0 )
		currentAgriType = AgriType::Grove;

	// Request global lists if not yet loaded
	if ( globalPlants.isEmpty() )
		Global::eventConnector->aggregatorAgri()->onRequestGlobalPlantInfo();
	if ( globalTrees.isEmpty() )
		Global::eventConnector->aggregatorAgri()->onRequestGlobalTreeInfo();
	if ( globalAnimals.isEmpty() )
		Global::eventConnector->aggregatorAgri()->onRequestGlobalAnimalInfo();
}

void ImGuiBridge::onUpdateFarm( const GuiFarmInfo& info ) { farmInfo = info; }
void ImGuiBridge::onUpdatePasture( const GuiPastureInfo& info ) { pastureInfo = info; }
void ImGuiBridge::onUpdateGrove( const GuiGroveInfo& info ) { groveInfo = info; }
void ImGuiBridge::onGlobalPlantInfo( const QList<GuiPlant>& info ) { globalPlants = info; }
void ImGuiBridge::onGlobalAnimalInfo( const QList<GuiAnimal>& info ) { globalAnimals = info; }
void ImGuiBridge::onGlobalTreeInfo( const QList<GuiPlant>& info ) { globalTrees = info; }

// Population
void ImGuiBridge::onPopulationUpdate( const GuiPopulationInfo& info ) { populationInfo = info; }
void ImGuiBridge::onScheduleUpdate( const GuiScheduleInfo& info ) { scheduleInfo = info; }
void ImGuiBridge::onProfessionList( const QStringList& profs ) { professionList = profs; }
void ImGuiBridge::onProfessionSkills( QString profession, const QList<GuiSkillInfo>& skills )
{
	editingProfession = profession;
	professionSkills = skills;
}

// Military
void ImGuiBridge::onSquads( const QList<GuiSquad>& s ) { squads = s; }
void ImGuiBridge::onRoles( const QList<GuiMilRole>& r ) { roles = r; }

// Creature info
void ImGuiBridge::onCreatureUpdate( const GuiCreatureInfo& info )
{
	creatureInfo = info;
	showCreatureInfo = true;
}

void ImGuiBridge::onCreatureProfessionList( const QStringList& profs ) { creatureProfessionList = profs; }

// Neighbors
void ImGuiBridge::onNeighborsUpdate( const QList<GuiNeighborInfo>& infos ) { neighbors = infos; }
void ImGuiBridge::onMissions( const QList<Mission>& m ) { missions = m; }
void ImGuiBridge::onAvailableGnomes( const QList<GuiAvailableGnome>& g ) { availableGnomes = g; }

// Inventory
void ImGuiBridge::onInventoryCategories( const QList<GuiInventoryCategory>& cats ) { inventoryCategories = cats; }

// Settings
void ImGuiBridge::onUpdateSettings( const GuiSettings& info ) { settings = info; }

// Load game
void ImGuiBridge::onKingdoms( const QList<GuiSaveInfo>& k ) { kingdoms = k; }
void ImGuiBridge::onSaveGames( const QList<GuiSaveInfo>& s ) { saveGames = s; }

// =============================================================================
// Command implementations — emit signals to game
// =============================================================================

void ImGuiBridge::cmdStartNewGame() { emit signalStartNewGame(); appState = AppState::WaitingForLoad; }
void ImGuiBridge::cmdContinueLastGame() { emit signalContinueLastGame(); appState = AppState::WaitingForLoad; }
void ImGuiBridge::cmdLoadGame( const QString& path ) { emit signalLoadGame( path ); appState = AppState::WaitingForLoad; }
void ImGuiBridge::cmdSaveGame() { emit signalSaveGame(); }
void ImGuiBridge::cmdEndGame() { emit signalEndGame(); appState = AppState::MainMenu; }
void ImGuiBridge::cmdSetShowMainMenu( bool value ) { emit signalSetShowMainMenu( value ); }
void ImGuiBridge::cmdSetPaused( bool value ) { emit signalSetPaused( value ); }
void ImGuiBridge::cmdSetGameSpeed( GameSpeed speed ) { emit signalSetGameSpeed( speed ); }
void ImGuiBridge::cmdSetRenderOptions( bool d, bool j, bool w, bool a ) { emit signalSetRenderOptions( d, j, w, a ); }
void ImGuiBridge::cmdPropagateEscape() { emit signalPropagateEscape(); }
void ImGuiBridge::cmdSetSelectionAction( const QString& action ) { emit signalSetSelectionAction( action ); }
void ImGuiBridge::cmdRequestBuildItems( BuildSelection sel, const QString& category ) { emit signalRequestBuildItems( sel, category ); }
void ImGuiBridge::cmdBuild( BuildItemType type, const QString& param, const QString& item, const QStringList& mats ) { emit signalCmdBuild( type, param, item, mats ); }
void ImGuiBridge::cmdEventAnswer( unsigned int eventID, bool answer ) { emit signalEventAnswer( eventID, answer ); }

// Tile info commands
void ImGuiBridge::cmdTerrainCommand( unsigned int tileID, const QString& cmd ) { emit signalTerrainCommand( tileID, cmd ); }
void ImGuiBridge::cmdManageCommand( unsigned int tileID ) { emit signalManageCommand( tileID ); }
void ImGuiBridge::cmdRequestStockpileItems( unsigned int tileID ) { emit signalRequestStockpileItems( tileID ); }
void ImGuiBridge::cmdSetTennant( unsigned int did, unsigned int gid ) { emit signalSetTennant( did, gid ); }
void ImGuiBridge::cmdSetAlarm( unsigned int did, bool value ) { emit signalSetAlarm( did, value ); }
void ImGuiBridge::cmdToggleMechActive( unsigned int id ) { emit signalToggleMechActive( id ); }
void ImGuiBridge::cmdToggleMechInvert( unsigned int id ) { emit signalToggleMechInvert( id ); }
void ImGuiBridge::cmdSetAutomatonRefuel( unsigned int id, bool refuel ) { emit signalSetAutomatonRefuel( id, refuel ); }
void ImGuiBridge::cmdSetAutomatonCore( unsigned int id, const QString& core ) { emit signalSetAutomatonCore( id, core ); }

// Stockpile commands
void ImGuiBridge::cmdStockpileSetActive( unsigned int spID, bool active, const QString& cat, const QString& group, const QString& item, const QString& material )
{
	Global::eventConnector->aggregatorStockpile()->onSetActive( spID, active, cat, group, item, material );
}

void ImGuiBridge::cmdStockpileSetOptions( unsigned int spID, const QString& name, int priority, bool suspended, bool pullOthers, bool pullFrom )
{
	Global::eventConnector->aggregatorStockpile()->onSetBasicOptions( spID, name, priority, suspended, pullOthers, pullFrom );
}

void ImGuiBridge::cmdCloseStockpileWindow()
{
	showStockpileWindow = false;
	activeSidePanel = SidePanel::None;
	Global::eventConnector->aggregatorStockpile()->onCloseWindow();
}

// Workshop commands
void ImGuiBridge::cmdWorkshopSetOptions( unsigned int wsID, const QString& name, int priority, bool suspended, bool acceptGenerated, bool autoCraftMissing, bool linkStockpile )
{
	Global::eventConnector->aggregatorWorkshop()->onSetBasicOptions( wsID, name, priority, suspended, acceptGenerated, autoCraftMissing, linkStockpile );
}

void ImGuiBridge::cmdWorkshopCraftItem( const QString& sid, int mode, const QString& number, const QStringList& mats )
{
	Global::eventConnector->aggregatorWorkshop()->onCraftItem( activeWorkshopID, sid, mode, number.toInt(), mats );
}

void ImGuiBridge::cmdWorkshopCraftJobParams( unsigned int jobID, int mode, const QString& number, bool suspended, bool moveBack )
{
	Global::eventConnector->aggregatorWorkshop()->onCraftJobParams( activeWorkshopID, jobID, mode, number.toInt(), suspended, moveBack );
}

void ImGuiBridge::cmdWorkshopCraftJobCommand( unsigned int jobID, const QString& cmd )
{
	Global::eventConnector->aggregatorWorkshop()->onCraftJobCommand( activeWorkshopID, jobID, cmd );
}

void ImGuiBridge::cmdWorkshopSetButcherOptions( unsigned int wsID, bool corpses, bool excess )
{
	Global::eventConnector->aggregatorWorkshop()->onSetButcherOptions( wsID, corpses, excess );
}

void ImGuiBridge::cmdWorkshopSetFisherOptions( unsigned int wsID, bool catchFish, bool processFish )
{
	Global::eventConnector->aggregatorWorkshop()->onSetFisherOptions( wsID, catchFish, processFish );
}

void ImGuiBridge::cmdWorkshopTraderOfferToStock( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount )
{
	Global::eventConnector->aggregatorWorkshop()->onTraderOffertoStock( wsID, item, mat, (unsigned char)quality, amount );
}

void ImGuiBridge::cmdWorkshopTraderStockToOffer( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount )
{
	Global::eventConnector->aggregatorWorkshop()->onTraderStocktoOffer( wsID, item, mat, (unsigned char)quality, amount );
}

void ImGuiBridge::cmdWorkshopPlayerOfferToStock( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount )
{
	Global::eventConnector->aggregatorWorkshop()->onPlayerOffertoStock( wsID, item, mat, (unsigned char)quality, amount );
}

void ImGuiBridge::cmdWorkshopPlayerStockToOffer( unsigned int wsID, const QString& item, const QString& mat, int quality, int amount )
{
	Global::eventConnector->aggregatorWorkshop()->onPlayerStocktoOffer( wsID, item, mat, (unsigned char)quality, amount );
}

void ImGuiBridge::cmdWorkshopTrade( unsigned int wsID )
{
	Global::eventConnector->aggregatorWorkshop()->onTrade( wsID );
}

void ImGuiBridge::cmdWorkshopRequestAllTradeItems( unsigned int wsID )
{
	Global::eventConnector->aggregatorWorkshop()->onRequestAllTradeItems( wsID );
}

void ImGuiBridge::cmdCloseWorkshopWindow()
{
	showWorkshopWindow = false;
	activeSidePanel = SidePanel::None;
	Global::eventConnector->aggregatorWorkshop()->onCloseWindow();
}

// Agriculture commands
void ImGuiBridge::cmdAgriSetOptions( unsigned int id, const QString& name, int priority, bool suspended )
{
	Global::eventConnector->aggregatorAgri()->onSetBasicOptions( currentAgriType, id, name, priority, suspended );
}

void ImGuiBridge::cmdAgriSetHarvestOptions( unsigned int id, bool harvest, bool hay, bool tame )
{
	Global::eventConnector->aggregatorAgri()->onSetHarvestOptions( currentAgriType, id, harvest, hay, tame );
}

void ImGuiBridge::cmdAgriSetGroveOptions( unsigned int id, bool pickFruits, bool plantTrees, bool fellTrees )
{
	Global::eventConnector->aggregatorAgri()->onSetGroveOptions( id, pickFruits, plantTrees, fellTrees );
}

void ImGuiBridge::cmdAgriSelectProduct( unsigned int id, const QString& product )
{
	Global::eventConnector->aggregatorAgri()->onSelectProduct( currentAgriType, id, product );
}

void ImGuiBridge::cmdAgriSetMaxMale( unsigned int id, int max )
{
	Global::eventConnector->aggregatorAgri()->onSetMaxMale( id, max );
}

void ImGuiBridge::cmdAgriSetMaxFemale( unsigned int id, int max )
{
	Global::eventConnector->aggregatorAgri()->onSetMaxFemale( id, max );
}

void ImGuiBridge::cmdAgriSetButchering( unsigned int id, bool value )
{
	Global::eventConnector->aggregatorAgri()->onSetButchering( id, value );
}

void ImGuiBridge::cmdAgriSetFoodItemChecked( const QString& item, const QString& material, bool checked )
{
	Global::eventConnector->aggregatorAgri()->onSetFoodItemChecked( activeAgriID, item, material, checked );
}

void ImGuiBridge::cmdCloseAgriWindow()
{
	showAgriWindow = false;
	activeSidePanel = SidePanel::None;
	Global::eventConnector->aggregatorAgri()->onCloseWindow();
}

// Population commands
void ImGuiBridge::cmdRequestPopulationUpdate() { Global::eventConnector->aggregatorPopulation()->onRequestPopulationUpdate(); }
void ImGuiBridge::cmdSetSkillActive( unsigned int gid, const QString& sid, bool value ) { Global::eventConnector->aggregatorPopulation()->onSetSkillActive( gid, sid, value ); }
void ImGuiBridge::cmdSetAllSkills( unsigned int gid, bool value ) { Global::eventConnector->aggregatorPopulation()->onSetAllSkills( gid, value ); }
void ImGuiBridge::cmdSetAllGnomes( const QString& sid, bool value ) { Global::eventConnector->aggregatorPopulation()->onSetAllGnomes( sid, value ); }
void ImGuiBridge::cmdSetProfession( unsigned int gid, const QString& prof ) { Global::eventConnector->aggregatorPopulation()->onSetProfession( gid, prof ); }
void ImGuiBridge::cmdSortGnomes( const QString& mode ) { Global::eventConnector->aggregatorPopulation()->onSortGnomes( mode ); }
void ImGuiBridge::cmdRequestSchedules() { Global::eventConnector->aggregatorPopulation()->onRequestSchedules(); }
void ImGuiBridge::cmdSetSchedule( unsigned int gid, int h, ScheduleActivity act ) { Global::eventConnector->aggregatorPopulation()->onSetSchedule( gid, h, act ); }
void ImGuiBridge::cmdSetAllHours( unsigned int gid, ScheduleActivity act ) { Global::eventConnector->aggregatorPopulation()->onSetAllHours( gid, act ); }
void ImGuiBridge::cmdSetHourForAll( int h, ScheduleActivity act ) { Global::eventConnector->aggregatorPopulation()->onSetHourForAll( h, act ); }
void ImGuiBridge::cmdRequestProfessions() { Global::eventConnector->aggregatorPopulation()->onRequestProfessions(); }
void ImGuiBridge::cmdRequestSkills( const QString& prof ) { Global::eventConnector->aggregatorPopulation()->onRequestSkills( prof ); }
void ImGuiBridge::cmdUpdateProfession( const QString& name, const QString& newName, const QStringList& skills ) { Global::eventConnector->aggregatorPopulation()->onUpdateProfession( name, newName, skills ); }
void ImGuiBridge::cmdDeleteProfession( const QString& name ) { Global::eventConnector->aggregatorPopulation()->onDeleteProfession( name ); }
void ImGuiBridge::cmdNewProfession() { Global::eventConnector->aggregatorPopulation()->onNewProfession(); }

// Military commands
void ImGuiBridge::cmdRequestMilitaryUpdate() { Global::eventConnector->aggregatorMilitary()->onRequestMilitary(); }
void ImGuiBridge::cmdAddSquad() { Global::eventConnector->aggregatorMilitary()->onAddSquad(); }
void ImGuiBridge::cmdRemoveSquad( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onRemoveSquad( id ); }
void ImGuiBridge::cmdRenameSquad( unsigned int id, const QString& name ) { Global::eventConnector->aggregatorMilitary()->onRenameSquad( id, name ); }
void ImGuiBridge::cmdMoveSquadLeft( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onMoveSquadLeft( id ); }
void ImGuiBridge::cmdMoveSquadRight( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onMoveSquadRight( id ); }
void ImGuiBridge::cmdRemoveGnomeFromSquad( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onRemoveGnomeFromSquad( id ); }
void ImGuiBridge::cmdMoveGnomeLeft( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onMoveGnomeLeft( id ); }
void ImGuiBridge::cmdMoveGnomeRight( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onMoveGnomeRight( id ); }
void ImGuiBridge::cmdSetRole( unsigned int gid, const QString& role ) { Global::eventConnector->aggregatorMilitary()->onSetRole( gid, role.toUInt() ); }
void ImGuiBridge::cmdSetAttitude( unsigned int sid, const QString& target, int att ) { Global::eventConnector->aggregatorMilitary()->onSetAttitude( sid, target, (MilAttitude)att ); }
void ImGuiBridge::cmdMovePrioUp( unsigned int sid, const QString& target ) { Global::eventConnector->aggregatorMilitary()->onMovePrioUp( sid, target ); }
void ImGuiBridge::cmdMovePrioDown( unsigned int sid, const QString& target ) { Global::eventConnector->aggregatorMilitary()->onMovePrioDown( sid, target ); }
void ImGuiBridge::cmdRequestRoles() { Global::eventConnector->aggregatorMilitary()->onRequestRoles(); }
void ImGuiBridge::cmdAddRole() { Global::eventConnector->aggregatorMilitary()->onAddRole(); }
void ImGuiBridge::cmdRemoveRole( unsigned int id ) { Global::eventConnector->aggregatorMilitary()->onRemoveRole( id ); }
void ImGuiBridge::cmdRenameRole( unsigned int id, const QString& name ) { Global::eventConnector->aggregatorMilitary()->onRenameRole( id, name ); }
void ImGuiBridge::cmdSetRoleCivilian( unsigned int id, bool value ) { Global::eventConnector->aggregatorMilitary()->onSetRoleCivilian( id, value ); }
void ImGuiBridge::cmdSetArmorType( unsigned int roleID, const QString& slot, const QString& type, const QString& material )
{
	Global::eventConnector->aggregatorMilitary()->onSetArmorType( roleID, slot, type, material );
}

// Creature info commands
void ImGuiBridge::cmdRequestCreatureUpdate( unsigned int id ) { Global::eventConnector->aggregatorCreatureInfo()->onRequestCreatureUpdate( id ); }
void ImGuiBridge::cmdSetCreatureProfession( unsigned int id, const QString& prof ) { Global::eventConnector->aggregatorCreatureInfo()->onSetProfession( id, prof ); }

// Neighbor commands
void ImGuiBridge::cmdRequestNeighborsUpdate() { Global::eventConnector->aggregatorNeighbors()->onRequestNeighborsUpdate(); }
void ImGuiBridge::cmdRequestMissions() { Global::eventConnector->aggregatorNeighbors()->onRequestMissions(); }
void ImGuiBridge::cmdRequestAvailableGnomes() { Global::eventConnector->aggregatorNeighbors()->onRequestAvailableGnomes(); }
void ImGuiBridge::cmdStartMission( int type, int action, unsigned int targetID, unsigned int gnomeID )
{
	Global::eventConnector->aggregatorNeighbors()->onStartMission( (MissionType)type, (MissionAction)action, targetID, gnomeID );
}

// Inventory commands
void ImGuiBridge::cmdRequestInventoryCategories() { Global::eventConnector->aggregatorInventory()->onRequestCategories(); }
void ImGuiBridge::cmdInventorySetActive( bool active, const GuiWatchedItem& item )
{
	Global::eventConnector->aggregatorInventory()->onSetActive( active, item );
}

// Debug commands
void ImGuiBridge::cmdSpawnCreature( const QString& type ) { Global::eventConnector->aggregatorDebug()->onSpawnCreature( type ); }
void ImGuiBridge::cmdSetWindowSize( int w, int h ) { Global::eventConnector->aggregatorDebug()->onSetWindowSize( w, h ); }

// Settings commands
void ImGuiBridge::cmdSetLanguage( const QString& lang ) { Global::eventConnector->aggregatorSettings()->onSetLanguage( lang ); }
void ImGuiBridge::cmdSetUIScale( float scale ) { Global::eventConnector->aggregatorSettings()->onSetUIScale( scale ); }
void ImGuiBridge::cmdSetFullScreen( bool value ) { Global::eventConnector->aggregatorSettings()->onSetFullScreen( value ); }
void ImGuiBridge::cmdSetKeyboardSpeed( float value ) { Global::eventConnector->aggregatorSettings()->onSetKeyboardSpeed( value ); }
void ImGuiBridge::cmdSetLightMin( float value ) { Global::eventConnector->aggregatorSettings()->onSetLightMin( value ); }
void ImGuiBridge::cmdSetAudioMasterVolume( float value ) { Global::eventConnector->aggregatorSettings()->onSetAudioMasterVolume( value ); }
