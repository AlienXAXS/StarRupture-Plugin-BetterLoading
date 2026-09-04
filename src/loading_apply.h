#pragma once

// Applies the whole configured state -- gate patch plus CDO budget -- in one
// place, so plugin init, the config-changed callback and the console command
// cannot drift apart.
//
// Idempotent: safe to call as often as you like.
void ApplyStateFromConfig();

// Puts the game back exactly as it was found.
void RestoreOriginalState();
