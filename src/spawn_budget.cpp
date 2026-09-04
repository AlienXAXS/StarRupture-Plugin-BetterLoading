#include "spawn_budget.h"
#include "loading_config.h"
#include "plugin_helpers.h"
#include <MassSimulation_classes.hpp>

static SDK::UMassSimulationSettings* g_cdo = nullptr;

static double g_defaultSpawnSlice = 0.0;
static float  g_defaultRetry      = 0.0f;

bool EnsureBudgetCaptured()
{
    if (g_cdo)
        return true;

    SDK::UMassSimulationSettings* cdo = SDK::UMassSimulationSettings::GetDefaultObj();
    if (!cdo)
    {
        LOG_DEBUG("Budget: UMassSimulationSettings CDO not reachable yet");
        return false;
    }

    g_cdo               = cdo;
    g_defaultSpawnSlice = cdo->DesiredActorSpawningTimeSlicePerTick;
    g_defaultRetry      = cdo->DesiredActorFailedSpawningRetryTimeInterval;

    LOG_INFO("Budget: CDO at %p -- engine defaults: spawn slice %.4f s, failed-spawn retry %.2f s",
             static_cast<void*>(cdo), g_defaultSpawnSlice, g_defaultRetry);
    return true;
}

void ApplyBudgetFromConfig()
{
    if (!EnsureBudgetCaptured())
        return;

    const float sliceMs = LoadingConfig::Config::ReadSpawnTimeSliceMs();
    const double slice  = (sliceMs > 0.0f) ? (static_cast<double>(sliceMs) / 1000.0)
                                           : g_defaultSpawnSlice;

    if (g_cdo->DesiredActorSpawningTimeSlicePerTick != slice)
    {
        g_cdo->DesiredActorSpawningTimeSlicePerTick = slice;
        LOG_INFO("Budget: actor spawn time slice = %.4f s (%.2f ms)", slice, slice * 1000.0);
    }

    // The retry interval is a different shape of knob and is left alone by
    // default. UMassRepresentationSubsystem::Initialize caches it into
    // RetryTimeInterval when the world's subsystem is created, so a change here
    // only reaches worlds loaded after this point -- which is why it is applied
    // at plugin init rather than on world begin play.
    const float retry = LoadingConfig::Config::ReadFailedSpawnRetrySeconds();
    if (retry > 0.0f && g_cdo->DesiredActorFailedSpawningRetryTimeInterval != retry)
    {
        g_cdo->DesiredActorFailedSpawningRetryTimeInterval = retry;
        LOG_INFO("Budget: failed-spawn retry interval = %.2f s (was %.2f s) -- applies to worlds loaded from now on",
                 retry, g_defaultRetry);
    }
}

void RestoreBudgetDefaults()
{
    if (!g_cdo)
        return;

    g_cdo->DesiredActorSpawningTimeSlicePerTick        = g_defaultSpawnSlice;
    g_cdo->DesiredActorFailedSpawningRetryTimeInterval = g_defaultRetry;

    LOG_DEBUG("Budget: CDO defaults restored (slice %.4f s, retry %.2f s)",
              g_defaultSpawnSlice, g_defaultRetry);
}

bool   IsBudgetCaptured()            { return g_cdo != nullptr; }
double GetSpawnSliceSeconds()        { return g_cdo ? g_cdo->DesiredActorSpawningTimeSlicePerTick : 0.0; }
double GetDefaultSpawnSliceSeconds() { return g_defaultSpawnSlice; }
float  GetRetrySeconds()             { return g_cdo ? g_cdo->DesiredActorFailedSpawningRetryTimeInterval : 0.0f; }
float  GetDefaultRetrySeconds()      { return g_defaultRetry; }
