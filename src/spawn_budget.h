#pragma once

// UMassSimulationSettings CDO -- the spawn time slice.
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
// when there is actually something waiting to spawn. It is charged per spawner
// subsystem rather than globally -- neither UCrMassActorSpawnerSubsystem nor
// UCrMassBuildingSpawnerSubsystem overrides Initialize, so each registers its
// own PrePhysics callback and each gets the whole slice.

// Look up the CDO and record the engine's own values. Idempotent; returns false
// until the CDO is reachable.
bool EnsureBudgetCaptured();

// Write the spawn time slice, in seconds. No-op when the CDO is not captured or
// when the value is already what is being asked for.
void ApplySpawnSlice(double seconds);

// Put the engine's own value back.
void RestoreBudgetDefaults();

bool   IsBudgetCaptured();
double GetSpawnSliceSeconds();        // live CDO value
double GetDefaultSpawnSliceSeconds(); // what the engine shipped with

// Read-only. The game ships 5.0 s here against stock Unreal's 0.5, which is
// worth seeing when diagnosing a slow load -- but it only affects spawns that
// return Failed, which this spawner should not produce (it sets
// SpawnCollisionHandlingOverride = AlwaysSpawn), and
// UMassRepresentationSubsystem::Initialize caches it per world anyway.
// Reported, never written.
float GetRetrySeconds();
