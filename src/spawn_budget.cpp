#include "spawn_budget.h"
#include "plugin_helpers.h"
#include <MassSimulation_classes.hpp>

static SDK::UMassSimulationSettings* g_cdo = nullptr;

static double g_defaultSpawnSlice = 0.0;

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

    LOG_INFO("Budget: CDO at %p -- game values: spawn slice %.4f s, failed-spawn retry %.2f s",
             static_cast<void*>(cdo),
             g_defaultSpawnSlice,
             cdo->DesiredActorFailedSpawningRetryTimeInterval);
    return true;
}

void ApplySpawnSlice(double seconds)
{
    if (!g_cdo || seconds <= 0.0)
        return;

    if (g_cdo->DesiredActorSpawningTimeSlicePerTick == seconds)
        return;

    g_cdo->DesiredActorSpawningTimeSlicePerTick = seconds;
    LOG_INFO("Budget: actor spawn time slice = %.2f ms (game default %.2f ms)",
             seconds * 1000.0, g_defaultSpawnSlice * 1000.0);
}

void RestoreBudgetDefaults()
{
    if (!g_cdo)
        return;

    if (g_cdo->DesiredActorSpawningTimeSlicePerTick == g_defaultSpawnSlice)
        return;

    g_cdo->DesiredActorSpawningTimeSlicePerTick = g_defaultSpawnSlice;
    LOG_DEBUG("Budget: spawn time slice restored to %.4f s", g_defaultSpawnSlice);
}

bool   IsBudgetCaptured()            { return g_cdo != nullptr; }
double GetSpawnSliceSeconds()        { return g_cdo ? g_cdo->DesiredActorSpawningTimeSlicePerTick : 0.0; }
double GetDefaultSpawnSliceSeconds() { return g_defaultSpawnSlice; }
float  GetRetrySeconds()             { return g_cdo ? g_cdo->DesiredActorFailedSpawningRetryTimeInterval : 0.0f; }
