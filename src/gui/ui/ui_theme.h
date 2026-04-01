#pragma once

// Available UI color themes
enum class UITheme : int
{
	Medieval = 0,  // Warm browns, gold accents, parchment text
	Slate,         // Cool blue-grey, steel accents (original)
	Forest,        // Deep greens, earth tones
	Nordic,        // Dark charcoal, ice-blue accents
	COUNT
};

// Theme display names (for settings UI)
inline const char* UIThemeNames[] = { "Medieval", "Slate", "Forest", "Nordic" };

// Apply a theme's colors to the current ImGui style.
// Safe to call at any time — takes effect next frame.
void ApplyUITheme( UITheme theme );

// Get/set the active theme (persisted in Config as "uiTheme")
UITheme GetActiveUITheme();
void    SetActiveUITheme( UITheme theme );
