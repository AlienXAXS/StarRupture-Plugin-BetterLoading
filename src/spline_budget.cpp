#include "spline_budget.h"
#include "plugin_helpers.h"
#include <Chimera_classes.hpp>

static SDK::UCrSplineBasedBuildingsSpawnerSubsystemSettings* g_cdo = nullptr;

static float g_defaultPerFrame = 0.0f;
static float g_defaultMax      = 0.0f;

bool EnsureSplineBudgetCaptured()
{
    if (g_cdo)
        return true;

    SDK::UCrSplineBasedBuildingsSpawnerSubsystemSettings* cdo =
        SDK::UCrSplineBasedBuildingsSpawnerSubsystemSettings::GetDefaultObj();

    if (!cdo)
    {
        LOG_DEBUG("Spline: CrSplineBasedBuildingsSpawnerSubsystemSettings CDO not reachable yet");
        return false;
    }

    g_cdo             = cdo;
    g_defaultPerFrame = cdo->TimeBudgetForSpawningSplineMeshComponentsPerFrame;
    g_defaultMax      = cdo->MaxTimeBudgetForSpawningSplineMeshComponents;

    // Logged at INFO on purpose: nobody has seen these numbers yet, and one line
    // in a real run is what tells us whether the multiplier below is sensible.
    LOG_INFO("Spline: CDO at %p -- game values: perFrame=%.6f max=%.6f rampStart=%.4f rampEnd=%.4f",
             static_cast<void*>(cdo),
             g_defaultPerFrame,
             g_defaultMax,
             cdo->StartTimeSpendGeneratingSplineMeshComponentsSeconds,
             cdo->EndTimeSpendGeneratingSplineMeshComponentsSeconds);
    return true;
}

void ApplySplineBudget(float multiplier)
{
    if (!g_cdo || multiplier <= 0.0f)
        return;

    const float perFrame = g_defaultPerFrame * multiplier;
    const float maxBudget = g_defaultMax * multiplier;

    if (g_cdo->TimeBudgetForSpawningSplineMeshComponentsPerFrame == perFrame &&
        g_cdo->MaxTimeBudgetForSpawningSplineMeshComponents == maxBudget)
    {
        return;
    }

    g_cdo->TimeBudgetForSpawningSplineMeshComponentsPerFrame = perFrame;
    g_cdo->MaxTimeBudgetForSpawningSplineMeshComponents      = maxBudget;

    LOG_INFO("Spline: rail/belt budget x%.2f -- perFrame %.6f -> %.6f, max %.6f -> %.6f",
             multiplier, g_defaultPerFrame, perFrame, g_defaultMax, maxBudget);
}

void RestoreSplineBudgetDefaults()
{
    if (!g_cdo)
        return;

    if (g_cdo->TimeBudgetForSpawningSplineMeshComponentsPerFrame == g_defaultPerFrame &&
        g_cdo->MaxTimeBudgetForSpawningSplineMeshComponents == g_defaultMax)
    {
        return;
    }

    g_cdo->TimeBudgetForSpawningSplineMeshComponentsPerFrame = g_defaultPerFrame;
    g_cdo->MaxTimeBudgetForSpawningSplineMeshComponents      = g_defaultMax;

    LOG_DEBUG("Spline: rail/belt budget restored (perFrame=%.6f max=%.6f)",
              g_defaultPerFrame, g_defaultMax);
}

bool IsSplineBudgetCaptured() { return g_cdo != nullptr; }

float GetSplinePerFrameBudget()   { return g_cdo ? g_cdo->TimeBudgetForSpawningSplineMeshComponentsPerFrame : 0.0f; }
float GetSplineMaxBudget()        { return g_cdo ? g_cdo->MaxTimeBudgetForSpawningSplineMeshComponents : 0.0f; }
float GetSplineRampStartSeconds() { return g_cdo ? g_cdo->StartTimeSpendGeneratingSplineMeshComponentsSeconds : 0.0f; }
float GetSplineRampEndSeconds()   { return g_cdo ? g_cdo->EndTimeSpendGeneratingSplineMeshComponentsSeconds : 0.0f; }

float GetDefaultSplinePerFrameBudget() { return g_defaultPerFrame; }
float GetDefaultSplineMaxBudget()      { return g_defaultMax; }
