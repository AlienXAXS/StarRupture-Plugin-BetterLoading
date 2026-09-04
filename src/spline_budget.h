#pragma once

// UCrSplineBasedBuildingsSpawnerSubsystemSettings -- the rails/belts throttle.
//
// Spline-based buildings (drone rails and anything else built along a spline)
// do NOT come through UMassSimulationSettings::DesiredActorSpawningTimeSlicePerTick.
// They have their own developer settings with their own per-frame budget, which
// is why removing the Mass per-frame cap and raising the Mass slice does nothing
// for them, and why rails are the last thing to finish appearing after a load.
//
//   TimeBudgetForSpawningSplineMeshComponentsPerFrame   0x38
//   StartTimeSpendGeneratingSplineMeshComponentsSeconds 0x3C
//   EndTimeSpendGeneratingSplineMeshComponentsSeconds   0x40
//   MaxTimeBudgetForSpawningSplineMeshComponents        0x44
//
// The shipped values are not known here -- they are read off the CDO at runtime
// and logged, which is the point: the log from a real run is what tells us what
// to aim at. Until then this scales rather than sets, so it is correct whatever
// the units and whatever the game ships.
//
// The Start/End pair looks like a ramp -- budget climbing toward Max as
// generation time accumulates -- but that reading is unverified, so neither is
// touched. Only the two budgets are scaled.

bool EnsureSplineBudgetCaptured();

// Scale the two budgets by `multiplier`. 1.0 restores the game's own values.
void ApplySplineBudget(float multiplier);

void RestoreSplineBudgetDefaults();

bool IsSplineBudgetCaptured();

// Live values, for `betterloading status`.
float GetSplinePerFrameBudget();
float GetSplineMaxBudget();
float GetSplineRampStartSeconds();
float GetSplineRampEndSeconds();

// What the game shipped with.
float GetDefaultSplinePerFrameBudget();
float GetDefaultSplineMaxBudget();
