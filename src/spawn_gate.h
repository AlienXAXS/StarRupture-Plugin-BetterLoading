#pragma once

#include <cstdint>

struct IPluginSelf;
struct IPluginHookScanner;

// Resolve the per-frame building spawn gate. Callable only from
// OnPluginLoadHooks -- the loader refuses scans made anywhere else.
void ResolveSpawnGate(IPluginSelf* self, IPluginHookScanner* scanner);

// Write / restore the patch. Both are idempotent and safe to call when the
// pattern missed (they log and do nothing).
bool ApplySpawnGatePatch();
void RestoreSpawnGatePatch();

bool      IsSpawnGatePatched();
uintptr_t GetSpawnGateAddress(); // 0 when the AOB missed
