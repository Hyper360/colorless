#pragma once

// Global toggle state for all impairment and accessibility settings.
// Read by rendering code in later PRs; set by the pause menu.
namespace Settings {

// --- Impairments (simulate real visual conditions) ---
inline bool deuteranopia   = false; // red-green: green receptor deficiency
inline bool protanopia     = false; // red-green: red receptor deficiency
inline bool tritanopia     = false; // blue-yellow colorblindness
inline bool achromatopsia  = false; // total colorblindness (greyscale)
inline bool lowAcuity      = false; // blurred vision
inline bool tunnelVision   = false; // reduced peripheral field

// --- Accessibility features (mitigations) ---
inline bool highContrast   = false; // high-contrast tile outlines
inline bool patternTiles   = false; // pattern/texture overlays on tiles
inline bool shapeLabels    = false; // icon labels on tile types

// Enforce mutual exclusion among colorblind modes (only one active at a time).
// Pass the one being turned ON; all others are cleared.
inline void setColorblindMode(bool *chosen) {
  deuteranopia  = false;
  protanopia    = false;
  tritanopia    = false;
  achromatopsia = false;
  if (chosen) *chosen = true;
}

} // namespace Settings
