#include "loading_apply.h"
#include "loading_config.h"
#include "spawn_budget.h"
#include "spawn_gate.h"
#include "plugin_helpers.h"

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

    if (LoadingConfig::Config::ReadRemovePerFrameLimit())
        ApplySpawnGatePatch();
    else
        RestoreSpawnGatePatch();

    ApplyBudgetFromConfig();
}

void RestoreOriginalState()
{
    RestoreSpawnGatePatch();
    RestoreBudgetDefaults();
}
