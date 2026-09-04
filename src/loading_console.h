#pragma once

struct IPluginSelf;

// `betterloading` -- status / live tuning. Registered in both console
// front-ends, which on a dedicated server is the only way to reach any of this.
void RegisterLoadingConsoleCommand(IPluginSelf* self);
void UnregisterLoadingConsoleCommand(IPluginSelf* self);
