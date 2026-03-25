/*	
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"

#include "../base/config.h"
#include "../base/io.h"
#include "../gui/eventconnector.h"
#include "../gui/aggregatorselection.h"

#include "mainwindowrenderer.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include "imgui_impl_qt5.h"
#include "imguibridge.h"
#include "ui/ui_mainmenu.h"
#include "ui/ui_gamehud.h"
#include "ui/ui_tileinfo.h"
#include "ui/ui_sidepanels.h"

#include <string>

static MainWindow* instance;


MainWindow::MainWindow( QWidget* parent ) :
	QOpenGLWindow()
{
	qDebug() << "Create main window.";
	connect( Global::eventConnector, &EventConnector::signalExit, this, &MainWindow::onExit );
	connect( this, &MainWindow::signalWindowSize, Global::eventConnector, &EventConnector::onWindowSize );
	connect( this, &MainWindow::signalViewLevel, Global::eventConnector, &EventConnector::onViewLevel );
	connect( this, &MainWindow::signalKeyPress, Global::eventConnector, &EventConnector::onKeyPress );
	connect( this, &MainWindow::signalTogglePause, Global::eventConnector, &EventConnector::onTogglePause );
	connect( this, &MainWindow::signalUpdateRenderOptions, Global::eventConnector, &EventConnector::onUpdateRenderOptions );
		
	connect( this, &MainWindow::signalMouse, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onMouse, Qt::QueuedConnection );
	connect( this, &MainWindow::signalLeftClick, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onLeftClick, Qt::QueuedConnection );
	connect( this, &MainWindow::signalRightClick, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onRightClick, Qt::QueuedConnection );
	connect( this, &MainWindow::signalRotateSelection, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onRotateSelection, Qt::QueuedConnection );
	connect( this, &MainWindow::signalRenderParams, Global::eventConnector->aggregatorSelection(), &AggregatorSelection::onRenderParams, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorDebug(), &AggregatorDebug::signalSetWindowSize, this, &MainWindow::onSetWindowSize, Qt::QueuedConnection );

	connect( Global::eventConnector->aggregatorSettings(), &AggregatorSettings::signalFullScreen, this, &MainWindow::onFullScreen, Qt::QueuedConnection );

	connect( Global::eventConnector, &EventConnector::signalInitView, this, &MainWindow::onInitViewAfterLoad, Qt::QueuedConnection );

	instance = this;
}

MainWindow::~MainWindow()
{
	qDebug() << "MainWindow destructor";

	if( !m_isFullScreen )
	{
		Global::cfg->set( "WindowWidth", this->width() );
		Global::cfg->set( "WindowHeight", this->height() );
		Global::cfg->set( "WindowPosX", this->position().x() );
		Global::cfg->set( "WindowPosY", this->position().y() );
	}
	
	IO::saveConfig();
	instance = nullptr;
}

MainWindow& MainWindow::getInstance()
{
	return *instance;
}

void MainWindow::onExit()
{
	this->close();
}

void MainWindow::takeScreenshot()
{
	static int counter = 0;
	QString dir = QCoreApplication::applicationDirPath() + "/../docs/updates/gui";
	QDir().mkpath( dir );
	QString path = dir + "/screenshot_" + QString::number( counter++ ).rightJustified( 3, '0' ) + ".png";
	takeScreenshot( path );
}

void MainWindow::takeScreenshot( const QString& outputPath )
{
	QFileInfo fi( outputPath );
	QDir().mkpath( fi.absolutePath() );
	QImage img = this->grabFramebuffer();
	if ( img.save( outputPath ) )
	{
		qDebug() << "Screenshot saved:" << outputPath;
	}
	else
	{
		qDebug() << "Screenshot failed:" << outputPath;
	}
}

void MainWindow::toggleFullScreen()
{
	QOpenGLWindow* w = this;
	m_isFullScreen = !m_isFullScreen;
	if ( m_isFullScreen )
	{
		w->showFullScreen();
		Global::cfg->set( "fullscreen", true );
	}
	else
	{
		// Reset from fullscreen:
		w->showNormal();
		Global::cfg->set( "fullscreen", false );
	}
	m_renderer->onRenderParamsChanged();
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
}

void MainWindow::onFullScreen( bool value )
{
	QOpenGLWindow* w = this;
	m_isFullScreen = value;
	Global::cfg->set( "fullscreen", value );
	if ( value )
	{
		w->showFullScreen();
	}
	else
	{
		// Reset from fullscreen:
		w->showNormal();
	}
	m_renderer->onRenderParamsChanged();
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
}

void MainWindow::keyPressEvent( QKeyEvent* event )
{
	ImGuiQt5::ProcessKeyEvent( event );

	// Space and Escape always work regardless of ImGui focus
	if ( event->key() == Qt::Key_Space )
	{
		emit signalTogglePause();
		return;
	}
	if ( event->key() == Qt::Key_Escape )
	{
		emit signalKeyPress( event->key() );
		return;
	}

	if ( ImGui::GetIO().WantCaptureKeyboard )
		return;

	int qtKey = event->key();

	{
		switch ( event->key() )
		{
			case Qt::Key_H:
				Global::wallsLowered = !Global::wallsLowered;
				emit signalUpdateRenderOptions();
				break;
			case Qt::Key_K:
				break;
			case Qt::Key_O:
				if ( event->modifiers() & Qt::ControlModifier )
				{
					Global::debugMode = !Global::debugMode;
				}
				else
				{
					Global::showDesignations = !Global::showDesignations;
					emit signalUpdateRenderOptions();
				}
				m_renderer->onRenderParamsChanged();
				break;
			case Qt::Key_F:
				//toggleFullScreen();
				break;
			case Qt::Key_R:
				if ( event->modifiers() & Qt::ControlModifier )
				{
					Global::debugOpenGL = !Global::debugOpenGL;
				}
				else
				{
					emit signalRotateSelection();
					redraw();
				}
				break;
			case Qt::Key_Comma:
				m_renderer->rotate( 1 );
				break;
			case Qt::Key_Period:
				m_renderer->rotate( -1 );
				break;
			// Space and Escape handled above (before ImGui capture check)
			case Qt::Key_W:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Up;
				redraw();
				break;
			case Qt::Key_S:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Down;
				redraw();
				break;
			case Qt::Key_A:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Left;
				redraw();
				break;
			case Qt::Key_D:
				keyboardMove();
				m_keyboardMove += KeyboardMove::Right;
				redraw();
				break;
			case Qt::Key_F12:
				takeScreenshot();
				break;
			case Qt::Key_PageUp:
				keyboardZPlus( event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
				redraw();
				break;
			case Qt::Key_PageDown:
				keyboardZMinus( event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
				redraw();
				break;
		}
		emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
		emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
	}
}

void MainWindow::keyReleaseEvent( QKeyEvent* event )
{
	ImGuiQt5::ProcessKeyEvent( event );
	if ( ImGui::GetIO().WantCaptureKeyboard )
		return;

	switch ( event->key() )
	{
		case Qt::Key_W:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Up;
			redraw();
			break;
		case Qt::Key_S:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Down;
			redraw();
			break;
		case Qt::Key_A:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Left;
			redraw();
			break;
		case Qt::Key_D:
			keyboardMove();
			m_keyboardMove -= KeyboardMove::Right;
			redraw();
			break;
	}
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
}

bool MainWindow::isOverGui( int x, int y )
{
	Q_UNUSED( x );
	Q_UNUSED( y );
	return ImGui::GetIO().WantCaptureMouse;
}

void MainWindow::keyboardMove()
{
	int x = 0;
	int y = 0;
	if( (bool)( m_keyboardMove & KeyboardMove::Up ) ) y -= 1;
	if( (bool)( m_keyboardMove & KeyboardMove::Down ) ) y += 1;
	if( (bool)( m_keyboardMove & KeyboardMove::Left ) ) x -= 1;
	if( (bool)( m_keyboardMove & KeyboardMove::Right ) ) x += 1;

	// Elapsed time in second
	const float elapsedTime = m_keyboardMovementTimer.nsecsElapsed() * 0.000000001f;
	m_keyboardMovementTimer.restart();

	if( m_renderer && (x || y) )
	{
		const float keyboardMoveSpeed = (Global::cfg->get( "keyboardMoveSpeed" ).toFloat() + 50.f) * 4.f;

		float moveX = -x * keyboardMoveSpeed * elapsedTime;
		float moveY = -y * keyboardMoveSpeed * elapsedTime;

		if (x && y)
		{
			moveX /= sqrt( 2 );
			moveY /= sqrt( 2 );
		}

		m_renderer->move( moveX, moveY );
		emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	}
}

void MainWindow::mouseMoveEvent( QMouseEvent* event )
{
	ImGuiQt5::ProcessMouseEvent( event );
	if ( ImGui::GetIO().WantCaptureMouse )
		return;

	auto gp = this->mapFromGlobal( event->globalPos() );
	{
		if ( event->buttons() & Qt::LeftButton && m_leftDown )
		{
			if ( ( abs( gp.x() - m_clickX ) > 5 || abs( gp.y() - m_clickY ) > 5 ) && !m_isMove )
			{
				m_isMove = true;
				m_moveX  = m_clickX;
				m_moveY  = m_clickY;
			}

			if ( m_isMove )
			{
				m_renderer->move( gp.x() - m_moveX, gp.y() - m_moveY );

				m_moveX = gp.x();
				m_moveY = gp.y();
			}
			emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
		}
		else
		{
			//qDebug() << "huhu";
		}
	}

	m_mouseX  = gp.x();
	m_mouseY  = gp.y();


	emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );

	redraw();
}

void MainWindow::onInitViewAfterLoad()
{
	m_renderer->setScale( 1.0 );
	m_moveX = GameState::moveX;
	m_moveY = GameState::moveY;
	m_renderer->move( m_moveX, m_moveY );
	
	m_renderer->setScale( GameState::scale );
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
}

void MainWindow::mousePressEvent( QMouseEvent* event )
{
	ImGuiQt5::ProcessMouseEvent( event );
	if ( ImGui::GetIO().WantCaptureMouse )
	{
		redraw();
		return;
	}

	//qDebug() << "mousePressEvent";
	auto gp = this->mapFromGlobal( event->globalPos() );
	if ( event->button() & Qt::LeftButton )
	{
		if ( !isOverGui( gp.x(), gp.y() ) )
		{
			m_clickX   = gp.x();
			m_clickY   = gp.y();
			m_isMove   = false;
			m_leftDown = true;
		}
	}
	else if ( event->button() & Qt::RightButton )
	{
		if ( !isOverGui( gp.x(), gp.y() ) )
		{
			m_rightDown = true;
		}
	}
}

void MainWindow::mouseReleaseEvent( QMouseEvent* event )
{
	ImGuiQt5::ProcessMouseEvent( event );
	if ( ImGui::GetIO().WantCaptureMouse )
	{
		redraw();
		return;
	}

	//qDebug() << "mouseReleaseEvent";
	if ( event->button() & Qt::LeftButton )
	{
		auto gp = this->mapFromGlobal( event->globalPos() );
		if ( !m_isMove && m_leftDown )
		{
			emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
			emit signalLeftClick( event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
			redraw();
		}
		m_isMove   = false;
		m_leftDown = false;
	}
	else if ( event->button() & Qt::RightButton )
	{
		if ( m_rightDown )
		{
			emit signalMouse( m_mouseX, m_mouseY, event->modifiers() & Qt::ShiftModifier, event->modifiers() & Qt::ControlModifier );
			emit signalRightClick();
			redraw();
		}
		m_rightDown = false;
	}
}

void MainWindow::wheelEvent( QWheelEvent* event )
{
	ImGuiQt5::ProcessWheelEvent( event );
	if ( ImGui::GetIO().WantCaptureMouse )
		return;

	QWheelEvent* wEvent = event;
	{
		if ( (bool)( wEvent->modifiers() & Qt::ControlModifier ) ^ Global::cfg->get( "toggleMouseWheel" ).toBool() )
		{
			auto delta = wEvent->delta();
			m_renderer->scale( pow( 1.002, delta ) );
		}
		else
		{
			// Accumulate wheel delta and only change Z-level at threshold
			// This prevents trackpads/smooth-scroll mice from jumping 3-5 levels
			m_wheelAccumZ += wEvent->delta();
			while ( m_wheelAccumZ >= m_wheelThreshold )
			{
				m_wheelAccumZ -= m_wheelThreshold;
				keyboardZPlus( event->modifiers() & Qt::ShiftModifier );
			}
			while ( m_wheelAccumZ <= -m_wheelThreshold )
			{
				m_wheelAccumZ += m_wheelThreshold;
				keyboardZMinus( event->modifiers() & Qt::ShiftModifier );
			}
		}
		emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	}
}

void MainWindow::focusInEvent( QFocusEvent* e )
{
	Q_UNUSED( e );
	redraw();
}

void MainWindow::focusOutEvent( QFocusEvent* e )
{
	Q_UNUSED( e );
}

void MainWindow::keyboardZPlus( bool shift, bool ctrl )
{
	int dimZ      = Global::dimZ - 1;
	GameState::viewLevel += 1;
	GameState::viewLevel = qMax( 0, qMin( dimZ, GameState::viewLevel ) );

	m_renderer->onRenderParamsChanged();
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	emit signalViewLevel( GameState::viewLevel );

	emit signalMouse( m_mouseX, m_mouseY, shift, ctrl );
	redraw();
}

void MainWindow::keyboardZMinus( bool shift, bool ctrl )
{
	int dimZ      = Global::dimZ - 1;
	GameState::viewLevel -= 1;
	GameState::viewLevel = qMax( 0, qMin( dimZ, GameState::viewLevel ) );

	m_renderer->onRenderParamsChanged();
	emit signalViewLevel( GameState::viewLevel );
	emit signalRenderParams( width(), height(), m_renderer->moveX(), m_renderer->moveY(), m_renderer->scale(), m_renderer->rotation() );
	emit signalMouse( m_mouseX, m_mouseY, shift, ctrl );
	redraw();
}


void MainWindow::imguiInit()
{
	ImGuiQt5::Init( this );
	ImGui_ImplOpenGL3_Init( "#version 410" );
	m_bridge = new ImGuiBridge( this );
}

void MainWindow::imguiShutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGuiQt5::Shutdown();
	delete m_bridge;
	m_bridge = nullptr;
}

void MainWindow::drawImGui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGuiQt5::NewFrame( this );
	ImGui::NewFrame();

	if ( m_showImGuiDemo )
	{
		ImGui::ShowDemoWindow( &m_showImGuiDemo );
	}

	if ( m_bridge )
	{
		switch ( m_bridge->appState )
		{
			case ImGuiBridge::AppState::MainMenu:
				drawMainMenu( *m_bridge );
				break;
			case ImGuiBridge::AppState::NewGame:
				drawNewGame( *m_bridge );
				break;
			case ImGuiBridge::AppState::LoadGame:
				drawLoadGame( *m_bridge );
				break;
			case ImGuiBridge::AppState::Settings:
				drawSettings( *m_bridge );
				break;
			case ImGuiBridge::AppState::WaitingForLoad:
				drawWaitScreen( *m_bridge );
				break;
			case ImGuiBridge::AppState::InGameMenu:
				drawGameHUD( *m_bridge );
				drawInGameMenu( *m_bridge );
				break;
			case ImGuiBridge::AppState::GameRunning:
				if ( !m_bridge->rendererReady )
				{
					// Show loading overlay while renderer warms up.
					// GL textures + tile data init happens on the first paint frames.
					m_bridge->rendererWarmupFrames++;
					if ( m_bridge->rendererWarmupFrames > 10 )
					{
						m_bridge->rendererReady = true;
						m_bridge->loadingStatus.clear();
					}
					else
					{
						drawWaitScreen( *m_bridge );
						break;
					}
				}
				drawGameHUD( *m_bridge );
				drawTileInfo( *m_bridge );

				switch ( m_bridge->activeSidePanel )
				{
					case ImGuiBridge::SidePanel::Kingdom:
						drawKingdomPanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::Stockpile:
						drawStockpilePanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::Military:
						drawMilitaryPanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::Population:
						drawPopulationPanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::Missions:
						drawNeighborsPanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::Workshop:
						drawWorkshopPanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::Agriculture:
						drawAgriculturePanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::CreatureInfo:
						drawCreatureInfoPanel( *m_bridge );
						break;
					case ImGuiBridge::SidePanel::EventLog:
						drawEventLogPanel( *m_bridge );
						break;
					default:
						break;
				}

				if ( m_bridge->showDebugPanel )
				{
					drawDebugPanel( *m_bridge );
				}

				break;
		}
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void MainWindow::idleRenderTick()
{
	// Check for ongoing keyboard movement
	keyboardMove();

	// Check if redraw is required
	if ( !m_pendingUpdate )
	{
		m_pendingUpdate = true;
		update();
	}
	else
	{
		// check again later
		m_timer->start( 20 );
	}
}

void MainWindow::paintGL()
{
	// Apply latest position
	keyboardMove();

	// Process pending camera navigation from toast "Go To" buttons
	if ( m_bridge && m_bridge->pendingCameraNav )
	{
		m_bridge->pendingCameraNav = false;
		m_renderer->onCenterCameraPosition( m_bridge->cameraNavTarget );
		GameState::viewLevel = m_bridge->cameraNavTarget.z;
		m_renderer->setViewLevel( GameState::viewLevel );
		emit signalViewLevel( GameState::viewLevel );
	}

	// Render the game world
	m_renderer->paintWorld();

	// Render ImGui UI on top
	drawImGui();

	m_timer->start( 0 );
	m_pendingUpdate = false;
}

void MainWindow::resizeGL( int w, int h )
{
	QOpenGLWindow::resizeGL( w, h );

	if( !m_isFullScreen )
	{
		Global::cfg->set( "WindowWidth", w );
		Global::cfg->set( "WindowHeight", h );
	}

	m_renderer->resize( this->width(), this->height() );

	context()->functions()->glViewport( 0, 0, this->width() * devicePixelRatioF(), this->height() * devicePixelRatioF() );

	emit signalWindowSize( this->width(), this->height() );

	update();
}

void MainWindow::onSetWindowSize( int width, int height )
{
	this->resize( width, height );
}

void MainWindow::redraw()
{
	if ( !m_pendingUpdate )
	{
		// Trigger rendering
		m_pendingUpdate = true;
		update();
	}
}

void MainWindow::initializeGL()
{
	QOpenGLWindow::initializeGL();

	m_renderer = new MainWindowRenderer( this );
	m_renderer->initializeGL();

	imguiInit();
	m_timer = new QTimer( this );
	connect( m_timer, &QTimer::timeout, this, &MainWindow::idleRenderTick );

	update();
}

MainWindowRenderer* MainWindow::renderer()
{
	return m_renderer;
}


