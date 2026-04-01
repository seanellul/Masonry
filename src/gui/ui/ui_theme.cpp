#include "ui_theme.h"
#include "../../base/global.h"
#include "../../base/config.h"
#include <imgui.h>

static UITheme s_activeTheme = UITheme::Medieval;

UITheme GetActiveUITheme()
{
	return s_activeTheme;
}

void SetActiveUITheme( UITheme theme )
{
	s_activeTheme = theme;
	Global::cfg->set( "uiTheme", (int)theme );
	ApplyUITheme( theme );
}

// ============================================================================
// Theme definitions
// ============================================================================

static void applyMedieval()
{
	ImVec4* c = ImGui::GetStyle().Colors;
	c[ImGuiCol_Text]                 = ImVec4( 0.92f, 0.88f, 0.78f, 1.00f );
	c[ImGuiCol_TextDisabled]         = ImVec4( 0.55f, 0.50f, 0.42f, 1.00f );
	c[ImGuiCol_WindowBg]             = ImVec4( 0.12f, 0.10f, 0.08f, 0.94f );
	c[ImGuiCol_ChildBg]              = ImVec4( 0.12f, 0.10f, 0.08f, 0.00f );
	c[ImGuiCol_PopupBg]              = ImVec4( 0.14f, 0.12f, 0.09f, 0.96f );
	c[ImGuiCol_Border]               = ImVec4( 0.35f, 0.28f, 0.18f, 0.55f );
	c[ImGuiCol_BorderShadow]         = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_FrameBg]              = ImVec4( 0.18f, 0.15f, 0.11f, 1.00f );
	c[ImGuiCol_FrameBgHovered]       = ImVec4( 0.25f, 0.20f, 0.14f, 1.00f );
	c[ImGuiCol_FrameBgActive]        = ImVec4( 0.32f, 0.25f, 0.16f, 1.00f );
	c[ImGuiCol_TitleBg]              = ImVec4( 0.12f, 0.10f, 0.07f, 1.00f );
	c[ImGuiCol_TitleBgActive]        = ImVec4( 0.18f, 0.14f, 0.09f, 1.00f );
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4( 0.12f, 0.10f, 0.07f, 0.75f );
	c[ImGuiCol_MenuBarBg]            = ImVec4( 0.14f, 0.12f, 0.09f, 1.00f );
	c[ImGuiCol_Button]               = ImVec4( 0.22f, 0.18f, 0.13f, 1.00f );
	c[ImGuiCol_ButtonHovered]        = ImVec4( 0.38f, 0.30f, 0.18f, 1.00f );
	c[ImGuiCol_ButtonActive]         = ImVec4( 0.52f, 0.40f, 0.20f, 1.00f );
	c[ImGuiCol_Header]               = ImVec4( 0.22f, 0.18f, 0.13f, 1.00f );
	c[ImGuiCol_HeaderHovered]        = ImVec4( 0.38f, 0.30f, 0.18f, 1.00f );
	c[ImGuiCol_HeaderActive]         = ImVec4( 0.52f, 0.40f, 0.20f, 1.00f );
	c[ImGuiCol_Tab]                  = ImVec4( 0.18f, 0.15f, 0.10f, 1.00f );
	c[ImGuiCol_TabHovered]           = ImVec4( 0.38f, 0.30f, 0.18f, 1.00f );
	c[ImGuiCol_TabSelected]          = ImVec4( 0.30f, 0.24f, 0.14f, 1.00f );
	c[ImGuiCol_ScrollbarBg]          = ImVec4( 0.12f, 0.10f, 0.08f, 0.50f );
	c[ImGuiCol_ScrollbarGrab]        = ImVec4( 0.30f, 0.25f, 0.18f, 1.00f );
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.40f, 0.32f, 0.22f, 1.00f );
	c[ImGuiCol_ScrollbarGrabActive]  = ImVec4( 0.50f, 0.40f, 0.25f, 1.00f );
	c[ImGuiCol_SliderGrab]           = ImVec4( 0.60f, 0.48f, 0.25f, 1.00f );
	c[ImGuiCol_SliderGrabActive]     = ImVec4( 0.72f, 0.56f, 0.28f, 1.00f );
	c[ImGuiCol_CheckMark]            = ImVec4( 0.75f, 0.60f, 0.28f, 1.00f );
	c[ImGuiCol_Separator]            = ImVec4( 0.35f, 0.28f, 0.18f, 0.50f );
	c[ImGuiCol_SeparatorHovered]     = ImVec4( 0.55f, 0.42f, 0.22f, 0.80f );
	c[ImGuiCol_SeparatorActive]      = ImVec4( 0.65f, 0.50f, 0.25f, 1.00f );
	c[ImGuiCol_ResizeGrip]           = ImVec4( 0.35f, 0.28f, 0.18f, 0.25f );
	c[ImGuiCol_ResizeGripHovered]    = ImVec4( 0.55f, 0.42f, 0.22f, 0.65f );
	c[ImGuiCol_ResizeGripActive]     = ImVec4( 0.65f, 0.50f, 0.25f, 0.90f );
	c[ImGuiCol_PlotHistogram]        = ImVec4( 0.60f, 0.48f, 0.25f, 1.00f );
	c[ImGuiCol_TableHeaderBg]        = ImVec4( 0.18f, 0.15f, 0.10f, 1.00f );
	c[ImGuiCol_TableBorderStrong]    = ImVec4( 0.30f, 0.24f, 0.16f, 0.60f );
	c[ImGuiCol_TableBorderLight]     = ImVec4( 0.25f, 0.20f, 0.14f, 0.40f );
	c[ImGuiCol_TableRowBg]           = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_TableRowBgAlt]        = ImVec4( 0.18f, 0.15f, 0.10f, 0.35f );
}

static void applySlate()
{
	ImVec4* c = ImGui::GetStyle().Colors;
	c[ImGuiCol_Text]                 = ImVec4( 0.90f, 0.92f, 0.95f, 1.00f );
	c[ImGuiCol_TextDisabled]         = ImVec4( 0.45f, 0.48f, 0.52f, 1.00f );
	c[ImGuiCol_WindowBg]             = ImVec4( 0.10f, 0.12f, 0.14f, 0.94f );
	c[ImGuiCol_ChildBg]              = ImVec4( 0.10f, 0.12f, 0.14f, 0.00f );
	c[ImGuiCol_PopupBg]              = ImVec4( 0.12f, 0.14f, 0.17f, 0.96f );
	c[ImGuiCol_Border]               = ImVec4( 0.25f, 0.28f, 0.32f, 0.55f );
	c[ImGuiCol_BorderShadow]         = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_FrameBg]              = ImVec4( 0.15f, 0.18f, 0.22f, 1.00f );
	c[ImGuiCol_FrameBgHovered]       = ImVec4( 0.20f, 0.25f, 0.32f, 1.00f );
	c[ImGuiCol_FrameBgActive]        = ImVec4( 0.25f, 0.32f, 0.42f, 1.00f );
	c[ImGuiCol_TitleBg]              = ImVec4( 0.10f, 0.12f, 0.14f, 1.00f );
	c[ImGuiCol_TitleBgActive]        = ImVec4( 0.12f, 0.16f, 0.22f, 1.00f );
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4( 0.10f, 0.12f, 0.14f, 0.75f );
	c[ImGuiCol_MenuBarBg]            = ImVec4( 0.12f, 0.14f, 0.17f, 1.00f );
	c[ImGuiCol_Button]               = ImVec4( 0.18f, 0.22f, 0.28f, 1.00f );
	c[ImGuiCol_ButtonHovered]        = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
	c[ImGuiCol_ButtonActive]         = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	c[ImGuiCol_Header]               = ImVec4( 0.18f, 0.22f, 0.28f, 1.00f );
	c[ImGuiCol_HeaderHovered]        = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
	c[ImGuiCol_HeaderActive]         = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	c[ImGuiCol_Tab]                  = ImVec4( 0.15f, 0.18f, 0.22f, 1.00f );
	c[ImGuiCol_TabHovered]           = ImVec4( 0.25f, 0.35f, 0.50f, 1.00f );
	c[ImGuiCol_TabSelected]          = ImVec4( 0.20f, 0.30f, 0.45f, 1.00f );
	c[ImGuiCol_ScrollbarBg]          = ImVec4( 0.10f, 0.12f, 0.14f, 0.50f );
	c[ImGuiCol_ScrollbarGrab]        = ImVec4( 0.25f, 0.28f, 0.32f, 1.00f );
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.30f, 0.35f, 0.42f, 1.00f );
	c[ImGuiCol_ScrollbarGrabActive]  = ImVec4( 0.40f, 0.45f, 0.52f, 1.00f );
	c[ImGuiCol_SliderGrab]           = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	c[ImGuiCol_SliderGrabActive]     = ImVec4( 0.40f, 0.60f, 0.80f, 1.00f );
	c[ImGuiCol_CheckMark]            = ImVec4( 0.35f, 0.60f, 0.85f, 1.00f );
	c[ImGuiCol_Separator]            = ImVec4( 0.25f, 0.28f, 0.32f, 0.50f );
	c[ImGuiCol_SeparatorHovered]     = ImVec4( 0.30f, 0.45f, 0.65f, 0.80f );
	c[ImGuiCol_SeparatorActive]      = ImVec4( 0.35f, 0.55f, 0.75f, 1.00f );
	c[ImGuiCol_ResizeGrip]           = ImVec4( 0.25f, 0.28f, 0.32f, 0.25f );
	c[ImGuiCol_ResizeGripHovered]    = ImVec4( 0.30f, 0.45f, 0.65f, 0.65f );
	c[ImGuiCol_ResizeGripActive]     = ImVec4( 0.35f, 0.55f, 0.75f, 0.90f );
	c[ImGuiCol_PlotHistogram]        = ImVec4( 0.30f, 0.50f, 0.70f, 1.00f );
	c[ImGuiCol_TableHeaderBg]        = ImVec4( 0.14f, 0.17f, 0.21f, 1.00f );
	c[ImGuiCol_TableBorderStrong]    = ImVec4( 0.22f, 0.25f, 0.30f, 0.60f );
	c[ImGuiCol_TableBorderLight]     = ImVec4( 0.20f, 0.22f, 0.26f, 0.40f );
	c[ImGuiCol_TableRowBg]           = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_TableRowBgAlt]        = ImVec4( 0.14f, 0.17f, 0.21f, 0.35f );
}

static void applyForest()
{
	ImVec4* c = ImGui::GetStyle().Colors;
	c[ImGuiCol_Text]                 = ImVec4( 0.88f, 0.92f, 0.85f, 1.00f );
	c[ImGuiCol_TextDisabled]         = ImVec4( 0.48f, 0.52f, 0.44f, 1.00f );
	c[ImGuiCol_WindowBg]             = ImVec4( 0.08f, 0.12f, 0.08f, 0.94f );
	c[ImGuiCol_ChildBg]              = ImVec4( 0.08f, 0.12f, 0.08f, 0.00f );
	c[ImGuiCol_PopupBg]              = ImVec4( 0.10f, 0.14f, 0.10f, 0.96f );
	c[ImGuiCol_Border]               = ImVec4( 0.22f, 0.32f, 0.20f, 0.55f );
	c[ImGuiCol_BorderShadow]         = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_FrameBg]              = ImVec4( 0.12f, 0.18f, 0.12f, 1.00f );
	c[ImGuiCol_FrameBgHovered]       = ImVec4( 0.16f, 0.25f, 0.16f, 1.00f );
	c[ImGuiCol_FrameBgActive]        = ImVec4( 0.20f, 0.32f, 0.20f, 1.00f );
	c[ImGuiCol_TitleBg]              = ImVec4( 0.08f, 0.12f, 0.08f, 1.00f );
	c[ImGuiCol_TitleBgActive]        = ImVec4( 0.12f, 0.18f, 0.10f, 1.00f );
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4( 0.08f, 0.12f, 0.08f, 0.75f );
	c[ImGuiCol_MenuBarBg]            = ImVec4( 0.10f, 0.14f, 0.10f, 1.00f );
	c[ImGuiCol_Button]               = ImVec4( 0.15f, 0.22f, 0.14f, 1.00f );
	c[ImGuiCol_ButtonHovered]        = ImVec4( 0.22f, 0.38f, 0.20f, 1.00f );
	c[ImGuiCol_ButtonActive]         = ImVec4( 0.28f, 0.50f, 0.25f, 1.00f );
	c[ImGuiCol_Header]               = ImVec4( 0.15f, 0.22f, 0.14f, 1.00f );
	c[ImGuiCol_HeaderHovered]        = ImVec4( 0.22f, 0.38f, 0.20f, 1.00f );
	c[ImGuiCol_HeaderActive]         = ImVec4( 0.28f, 0.50f, 0.25f, 1.00f );
	c[ImGuiCol_Tab]                  = ImVec4( 0.12f, 0.18f, 0.11f, 1.00f );
	c[ImGuiCol_TabHovered]           = ImVec4( 0.22f, 0.38f, 0.20f, 1.00f );
	c[ImGuiCol_TabSelected]          = ImVec4( 0.18f, 0.30f, 0.16f, 1.00f );
	c[ImGuiCol_ScrollbarBg]          = ImVec4( 0.08f, 0.12f, 0.08f, 0.50f );
	c[ImGuiCol_ScrollbarGrab]        = ImVec4( 0.22f, 0.30f, 0.20f, 1.00f );
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.30f, 0.40f, 0.28f, 1.00f );
	c[ImGuiCol_ScrollbarGrabActive]  = ImVec4( 0.38f, 0.50f, 0.35f, 1.00f );
	c[ImGuiCol_SliderGrab]           = ImVec4( 0.35f, 0.58f, 0.30f, 1.00f );
	c[ImGuiCol_SliderGrabActive]     = ImVec4( 0.42f, 0.68f, 0.36f, 1.00f );
	c[ImGuiCol_CheckMark]            = ImVec4( 0.40f, 0.70f, 0.35f, 1.00f );
	c[ImGuiCol_Separator]            = ImVec4( 0.22f, 0.32f, 0.20f, 0.50f );
	c[ImGuiCol_SeparatorHovered]     = ImVec4( 0.35f, 0.52f, 0.30f, 0.80f );
	c[ImGuiCol_SeparatorActive]      = ImVec4( 0.42f, 0.62f, 0.36f, 1.00f );
	c[ImGuiCol_ResizeGrip]           = ImVec4( 0.22f, 0.32f, 0.20f, 0.25f );
	c[ImGuiCol_ResizeGripHovered]    = ImVec4( 0.35f, 0.52f, 0.30f, 0.65f );
	c[ImGuiCol_ResizeGripActive]     = ImVec4( 0.42f, 0.62f, 0.36f, 0.90f );
	c[ImGuiCol_PlotHistogram]        = ImVec4( 0.35f, 0.58f, 0.30f, 1.00f );
	c[ImGuiCol_TableHeaderBg]        = ImVec4( 0.12f, 0.18f, 0.11f, 1.00f );
	c[ImGuiCol_TableBorderStrong]    = ImVec4( 0.20f, 0.28f, 0.18f, 0.60f );
	c[ImGuiCol_TableBorderLight]     = ImVec4( 0.18f, 0.24f, 0.16f, 0.40f );
	c[ImGuiCol_TableRowBg]           = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_TableRowBgAlt]        = ImVec4( 0.12f, 0.18f, 0.11f, 0.35f );
}

static void applyNordic()
{
	ImVec4* c = ImGui::GetStyle().Colors;
	c[ImGuiCol_Text]                 = ImVec4( 0.88f, 0.92f, 0.96f, 1.00f );
	c[ImGuiCol_TextDisabled]         = ImVec4( 0.42f, 0.46f, 0.52f, 1.00f );
	c[ImGuiCol_WindowBg]             = ImVec4( 0.08f, 0.09f, 0.11f, 0.95f );
	c[ImGuiCol_ChildBg]              = ImVec4( 0.08f, 0.09f, 0.11f, 0.00f );
	c[ImGuiCol_PopupBg]              = ImVec4( 0.10f, 0.11f, 0.14f, 0.96f );
	c[ImGuiCol_Border]               = ImVec4( 0.20f, 0.24f, 0.30f, 0.55f );
	c[ImGuiCol_BorderShadow]         = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_FrameBg]              = ImVec4( 0.12f, 0.14f, 0.18f, 1.00f );
	c[ImGuiCol_FrameBgHovered]       = ImVec4( 0.16f, 0.20f, 0.28f, 1.00f );
	c[ImGuiCol_FrameBgActive]        = ImVec4( 0.20f, 0.26f, 0.38f, 1.00f );
	c[ImGuiCol_TitleBg]              = ImVec4( 0.08f, 0.09f, 0.11f, 1.00f );
	c[ImGuiCol_TitleBgActive]        = ImVec4( 0.10f, 0.13f, 0.18f, 1.00f );
	c[ImGuiCol_TitleBgCollapsed]     = ImVec4( 0.08f, 0.09f, 0.11f, 0.75f );
	c[ImGuiCol_MenuBarBg]            = ImVec4( 0.10f, 0.11f, 0.14f, 1.00f );
	c[ImGuiCol_Button]               = ImVec4( 0.14f, 0.17f, 0.24f, 1.00f );
	c[ImGuiCol_ButtonHovered]        = ImVec4( 0.22f, 0.32f, 0.52f, 1.00f );
	c[ImGuiCol_ButtonActive]         = ImVec4( 0.28f, 0.42f, 0.68f, 1.00f );
	c[ImGuiCol_Header]               = ImVec4( 0.14f, 0.17f, 0.24f, 1.00f );
	c[ImGuiCol_HeaderHovered]        = ImVec4( 0.22f, 0.32f, 0.52f, 1.00f );
	c[ImGuiCol_HeaderActive]         = ImVec4( 0.28f, 0.42f, 0.68f, 1.00f );
	c[ImGuiCol_Tab]                  = ImVec4( 0.11f, 0.14f, 0.20f, 1.00f );
	c[ImGuiCol_TabHovered]           = ImVec4( 0.22f, 0.32f, 0.52f, 1.00f );
	c[ImGuiCol_TabSelected]          = ImVec4( 0.18f, 0.26f, 0.42f, 1.00f );
	c[ImGuiCol_ScrollbarBg]          = ImVec4( 0.08f, 0.09f, 0.11f, 0.50f );
	c[ImGuiCol_ScrollbarGrab]        = ImVec4( 0.22f, 0.26f, 0.34f, 1.00f );
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.30f, 0.36f, 0.46f, 1.00f );
	c[ImGuiCol_ScrollbarGrabActive]  = ImVec4( 0.38f, 0.46f, 0.58f, 1.00f );
	c[ImGuiCol_SliderGrab]           = ImVec4( 0.40f, 0.55f, 0.80f, 1.00f );
	c[ImGuiCol_SliderGrabActive]     = ImVec4( 0.50f, 0.65f, 0.90f, 1.00f );
	c[ImGuiCol_CheckMark]            = ImVec4( 0.50f, 0.68f, 0.92f, 1.00f );
	c[ImGuiCol_Separator]            = ImVec4( 0.20f, 0.24f, 0.30f, 0.50f );
	c[ImGuiCol_SeparatorHovered]     = ImVec4( 0.35f, 0.48f, 0.70f, 0.80f );
	c[ImGuiCol_SeparatorActive]      = ImVec4( 0.42f, 0.58f, 0.82f, 1.00f );
	c[ImGuiCol_ResizeGrip]           = ImVec4( 0.20f, 0.24f, 0.30f, 0.25f );
	c[ImGuiCol_ResizeGripHovered]    = ImVec4( 0.35f, 0.48f, 0.70f, 0.65f );
	c[ImGuiCol_ResizeGripActive]     = ImVec4( 0.42f, 0.58f, 0.82f, 0.90f );
	c[ImGuiCol_PlotHistogram]        = ImVec4( 0.40f, 0.55f, 0.80f, 1.00f );
	c[ImGuiCol_TableHeaderBg]        = ImVec4( 0.11f, 0.14f, 0.20f, 1.00f );
	c[ImGuiCol_TableBorderStrong]    = ImVec4( 0.18f, 0.22f, 0.30f, 0.60f );
	c[ImGuiCol_TableBorderLight]     = ImVec4( 0.16f, 0.20f, 0.26f, 0.40f );
	c[ImGuiCol_TableRowBg]           = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	c[ImGuiCol_TableRowBgAlt]        = ImVec4( 0.11f, 0.14f, 0.20f, 0.35f );
}

void ApplyUITheme( UITheme theme )
{
	s_activeTheme = theme;
	switch ( theme )
	{
		case UITheme::Medieval: applyMedieval(); break;
		case UITheme::Slate:    applySlate();    break;
		case UITheme::Forest:   applyForest();   break;
		case UITheme::Nordic:   applyNordic();   break;
		default:                applyMedieval();  break;
	}
}
