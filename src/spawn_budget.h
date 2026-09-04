#pragma once

// UMassSimulationSettings CDO knobs.
//
// Removing the per-frame cap in spawn_gate only converts into throughput as far
// as the engine's own wall-clock budget allows, so the two belong together:
// UMassActorSpawnerSubsystem::OnPrePhysicsPhaseStarted reads
// DesiredActorSpawningTimeSlicePerTick straight off this CDO every frame and
// passes it to ProcessPendingSpawningRequest. Writing the field therefore takes
// effect on the next frame, permanently, with no hook.
//
// The budget costs nothing while idle: ProcessPendingSpawningRequest breaks out
// immediately when the request queue is empty, so a raised slice is only spent
// when there is actually something waiting to spawn.

// Look up the CDO and record the engine's own defaults. Idempotent; returns
// false until the CDO is reachable.
bool EnsureBudgetCaptured();

// Push the configured values into the CDO. No-op when the CDO is not captured.
void ApplyBudgetFromConfig();

// Put the engine's defaults back.
void RestoreBudgetDefaults();

bool   IsBudgetCaptured();
double GetSpawnSliceSeconds();        // live CDO value
double GetDefaultSpawnSliceSeconds(); // what the engine shipped with
float  GetRetrySeconds();
float  GetDefaultRetrySeconds();
