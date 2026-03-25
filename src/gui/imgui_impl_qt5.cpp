#include "imgui_impl_qt5.h"

#include <imgui.h>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <QGuiApplication>

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

	// Style — dark semi-transparent theme matching old Noesis UI
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 2.0f;
	style.FrameRounding = 2.0f;
	style.GrabRounding = 2.0f;
	style.TabRounding = 2.0f;

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg]           = ImVec4( 0.10f, 0.12f, 0.14f, 0.92f );
	colors[ImGuiCol_ChildBg]            = ImVec4( 0.10f, 0.12f, 0.14f, 0.00f );
	colors[ImGuiCol_PopupBg]            = ImVec4( 0.12f, 0.14f, 0.17f, 0.95f );
	colors[ImGuiCol_Border]             = ImVec4( 0.25f, 0.28f, 0.32f, 0.50f );
	colors[ImGuiCol_FrameBg]            = ImVec4( 0.15f, 0.18f, 0.22f, 1.00f );
	colors[ImGuiCol_FrameBgHovered]     = ImVec4( 0.20f, 0.25f, 0.32f, 1.00f );
	colors[ImGuiCol_FrameBgActive]      = ImVec4( 0.25f, 0.32f, 0.42f, 1.00f );
	colors[ImGuiCol_TitleBg]            = ImVec4( 0.10f, 0.12f, 0.14f, 1.00f );
	colors[ImGuiCol_TitleBgActive]      = ImVec4( 0.12f, 0.16f, 0.22f, 1.00f );
	colors[ImGuiCol_Button]             = ImVec4( 0.18f, 0.22f, 0.28f, 1.00f );
	colors[ImGuiCol_ButtonHovered]      = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
	colors[ImGuiCol_ButtonActive]       = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	colors[ImGuiCol_Header]             = ImVec4( 0.18f, 0.22f, 0.28f, 1.00f );
	colors[ImGuiCol_HeaderHovered]      = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
	colors[ImGuiCol_HeaderActive]       = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	colors[ImGuiCol_Tab]                = ImVec4( 0.15f, 0.18f, 0.22f, 1.00f );
	colors[ImGuiCol_TabHovered]         = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
	colors[ImGuiCol_TabSelected]        = ImVec4( 0.20f, 0.30f, 0.45f, 1.00f );
	colors[ImGuiCol_ScrollbarBg]        = ImVec4( 0.10f, 0.12f, 0.14f, 0.50f );
	colors[ImGuiCol_ScrollbarGrab]      = ImVec4( 0.25f, 0.28f, 0.32f, 1.00f );
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.30f, 0.35f, 0.42f, 1.00f );
	colors[ImGuiCol_SliderGrab]         = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	colors[ImGuiCol_CheckMark]          = ImVec4( 0.35f, 0.60f, 0.85f, 1.00f );
	colors[ImGuiCol_Separator]          = ImVec4( 0.25f, 0.28f, 0.32f, 0.50f );

	style.ScaleAllSizes( dpr );
	io.FontGlobalScale = 1.0f;
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
