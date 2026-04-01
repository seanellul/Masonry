#include "imgui_impl_qt5.h"
#include "ui/ui_theme.h"
#include "../base/global.h"
#include "../base/config.h"

#include <imgui.h>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QFileInfo>

static ImGuiFonts s_fonts;

ImGuiFonts& GetImGuiFonts()
{
	return s_fonts;
}

namespace
{

ImGuiKey QtKeyToImGui( int key )
{
	switch ( key )
	{
		case Qt::Key_Tab: return ImGuiKey_Tab;
		case Qt::Key_Left: return ImGuiKey_LeftArrow;
		case Qt::Key_Right: return ImGuiKey_RightArrow;
		case Qt::Key_Up: return ImGuiKey_UpArrow;
		case Qt::Key_Down: return ImGuiKey_DownArrow;
		case Qt::Key_PageUp: return ImGuiKey_PageUp;
		case Qt::Key_PageDown: return ImGuiKey_PageDown;
		case Qt::Key_Home: return ImGuiKey_Home;
		case Qt::Key_End: return ImGuiKey_End;
		case Qt::Key_Insert: return ImGuiKey_Insert;
		case Qt::Key_Delete: return ImGuiKey_Delete;
		case Qt::Key_Backspace: return ImGuiKey_Backspace;
		case Qt::Key_Space: return ImGuiKey_Space;
		case Qt::Key_Return: return ImGuiKey_Enter;
		case Qt::Key_Enter: return ImGuiKey_KeypadEnter;
		case Qt::Key_Escape: return ImGuiKey_Escape;
		case Qt::Key_A: return ImGuiKey_A;
		case Qt::Key_C: return ImGuiKey_C;
		case Qt::Key_V: return ImGuiKey_V;
		case Qt::Key_X: return ImGuiKey_X;
		case Qt::Key_Y: return ImGuiKey_Y;
		case Qt::Key_Z: return ImGuiKey_Z;
		default: return ImGuiKey_None;
	}
}

} // namespace

void ImGuiQt5::Init( QOpenGLWindow* window )
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Set display size
	float dpr = window->devicePixelRatioF();
	io.DisplaySize = ImVec2( window->width(), window->height() );
	io.DisplayFramebufferScale = ImVec2( dpr, dpr );

	// Style — dark semi-transparent theme
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	// Shape language: soft rectangles, consistent rounding
	style.WindowRounding    = 4.0f;
	style.FrameRounding     = 3.0f;
	style.GrabRounding      = 3.0f;
	style.TabRounding       = 3.0f;

	// Spacing: breathing room for 16px font
	style.WindowPadding     = ImVec2( 12, 10 );
	style.FramePadding      = ImVec2( 8, 5 );
	style.ItemSpacing       = ImVec2( 8, 6 );
	style.IndentSpacing     = 18.0f;
	style.ScrollbarSize     = 16.0f;

	// Apply color theme from user config (defaults to Medieval)
	{
		int themeIdx = 0;
		if ( Global::cfg )
			themeIdx = Global::cfg->get( "uiTheme" ).toInt();
		if ( themeIdx < 0 || themeIdx >= (int)UITheme::COUNT )
			themeIdx = 0;
		ApplyUITheme( (UITheme)themeIdx );
	}

	style.ScaleAllSizes( dpr );
	io.FontGlobalScale = 1.0f;

	// =========================================================================
	// Typography system
	// =========================================================================
	// Built-in font as fallback (must be first in atlas)
	io.Fonts->AddFontDefault();

	QString exePath = QCoreApplication::applicationDirPath();
	std::string titlePath = ( exePath + "/content/xaml/Fonts/HermeneusOne-Regular.ttf" ).toStdString();
	std::string uiPath    = ( exePath + "/content/xaml/Theme/Fonts/PT Root UI_Regular.otf" ).toStdString();

	if ( QFileInfo::exists( QString::fromStdString( titlePath ) ) )
		s_fonts.title = io.Fonts->AddFontFromFileTTF( titlePath.c_str(), 48.0f );

	if ( QFileInfo::exists( QString::fromStdString( uiPath ) ) )
	{
		s_fonts.uiMedium  = io.Fonts->AddFontFromFileTTF( uiPath.c_str(), 20.0f );
		s_fonts.ui        = io.Fonts->AddFontFromFileTTF( uiPath.c_str(), 18.0f );
		s_fonts.uiDefault = io.Fonts->AddFontFromFileTTF( uiPath.c_str(), 16.0f );
		s_fonts.uiSmall   = io.Fonts->AddFontFromFileTTF( uiPath.c_str(), 14.0f );

		// Set 16px as the default for all in-game rendering
		io.FontDefault = s_fonts.uiDefault;
	}
}

void ImGuiQt5::Shutdown()
{
	ImGui::DestroyContext();
}

void ImGuiQt5::NewFrame( QOpenGLWindow* window )
{
	ImGuiIO& io = ImGui::GetIO();

	float dpr = window->devicePixelRatioF();
	io.DisplaySize = ImVec2( window->width(), window->height() );
	io.DisplayFramebufferScale = ImVec2( dpr, dpr );

	// Time
	static qint64 lastTime = 0;
	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
	if ( lastTime > 0 )
	{
		io.DeltaTime = ( currentTime - lastTime ) / 1000.0f;
	}
	else
	{
		io.DeltaTime = 1.0f / 60.0f;
	}
	lastTime = currentTime;
	if ( io.DeltaTime <= 0.0f )
	{
		io.DeltaTime = 1.0f / 60.0f;
	}

	// Modifier keys
	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	io.AddKeyEvent( ImGuiMod_Ctrl, modifiers & Qt::ControlModifier );
	io.AddKeyEvent( ImGuiMod_Shift, modifiers & Qt::ShiftModifier );
	io.AddKeyEvent( ImGuiMod_Alt, modifiers & Qt::AltModifier );
	io.AddKeyEvent( ImGuiMod_Super, modifiers & Qt::MetaModifier );
}

bool ImGuiQt5::ProcessKeyEvent( QKeyEvent* event )
{
	ImGuiIO& io = ImGui::GetIO();

	ImGuiKey key = QtKeyToImGui( event->key() );
	if ( key != ImGuiKey_None )
	{
		io.AddKeyEvent( key, event->type() == QEvent::KeyPress );
	}

	// Text input
	if ( event->type() == QEvent::KeyPress )
	{
		QString text = event->text();
		if ( !text.isEmpty() )
		{
			for ( const QChar& c : text )
			{
				if ( c.unicode() >= 32 )
				{
					io.AddInputCharacter( c.unicode() );
				}
			}
		}
	}

	return io.WantCaptureKeyboard;
}

bool ImGuiQt5::ProcessMouseEvent( QMouseEvent* event )
{
	ImGuiIO& io = ImGui::GetIO();

	io.AddMousePosEvent( event->x(), event->y() );

	if ( event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease )
	{
		bool pressed = event->type() == QEvent::MouseButtonPress;
		if ( event->button() == Qt::LeftButton )
			io.AddMouseButtonEvent( 0, pressed );
		if ( event->button() == Qt::RightButton )
			io.AddMouseButtonEvent( 1, pressed );
		if ( event->button() == Qt::MiddleButton )
			io.AddMouseButtonEvent( 2, pressed );
	}

	return io.WantCaptureMouse;
}

bool ImGuiQt5::ProcessWheelEvent( QWheelEvent* event )
{
	ImGuiIO& io = ImGui::GetIO();

	float wheel = event->angleDelta().y() / 120.0f;
	io.AddMouseWheelEvent( 0, wheel );

	return io.WantCaptureMouse;
}
