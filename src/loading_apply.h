#pragma once

// The single place that decides what the plugin has actually done to the game,
// so plugin init, the config-changed callback and the console command cannot
// drift apart.
//
// With Enabled on there is nothing to configure: the cap patch goes in and the
// spawn budget goes to kDefaultSliceMs. The two overrides below exist for
// diagnosis -- proving a stutter is or is not us, and finding the number worth
// hardcoding next time -- and are deliberately session-only. Nothing here is
// written to the ini; see the comment in loading_config.h.

// What the plugin does when nobody has overridden anything.
inline constexpr float kDefaultSliceMs = 4.0f;

// Idempotent. Safe to call as often as you like.
void ApplyStateFromConfig();

// Puts the game back exactly as it was found.
void RestoreOriginalState();

// Session-only overrides. Neither is persisted, and both reset on reload.
inline constexpr float kNoSliceOverride = -1.0f;

void  SetSliceOverrideMs(float ms); // kNoSliceOverride clears the override
float GetSliceOverrideMs();         // kNoSliceOverride when unset
float GetEffectiveSliceMs();        // what is actually applied

void SetKeepGameCap(bool keep);     // true = leave the game's own cap in force
bool GetKeepGameCap();
