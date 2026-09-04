#include "loading_apply.h"
#include "loading_config.h"
#include "spawn_budget.h"
#include "spawn_gate.h"
#include "plugin_helpers.h"

static float g_sliceOverrideMs = kNoSliceOverride;
static bool  g_keepGameCap     = false;

void ApplyStateFromConfig()
{
    // Capture first and unconditionally: it only reads the CDO, and it is what
    // lets `betterloading status` report the game's real values even while the
    // plugin is switched off.
    EnsureBudgetCaptured();

    if (!LoadingConfig::Config::IsEnabled())
    {
        RestoreOriginalState();
        return;
    }

    if (g_keepGameCap)
        RestoreSpawnGatePatch();
    else
        ApplySpawnGatePatch();

    ApplySpawnSlice(static_cast<double>(GetEffectiveSliceMs()) / 1000.0);
}

void RestoreOriginalState()
{
    RestoreSpawnGatePatch();
    RestoreBudgetDefaults();
}

void SetSliceOverrideMs(float ms)
{
    g_sliceOverrideMs = ms;
    ApplyStateFromConfig();
}

float GetSliceOverrideMs()
{
    return g_sliceOverrideMs;
}

float GetEffectiveSliceMs()
{
    return (g_sliceOverrideMs > 0.0f) ? g_sliceOverrideMs : kDefaultSliceMs;
}

void SetKeepGameCap(bool keep)
{
    g_keepGameCap = keep;
    ApplyStateFromConfig();
}

bool GetKeepGameCap()
{
    return g_keepGameCap;
}
