#include "spawn_gate.h"
#include "plugin_helpers.h"
#include <plugin_interface.h>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// UCrMassBuildingSpawnerSubsystem::SpawnActor -- the one-building-per-frame cap
//
// Every building actor visualization goes through this function.
// UCrMassBuildingRepresentationSubsystem::Initialize points its
// ActorSpawnerSubsystem at UCrMassBuildingSpawnerSubsystem, and the stock
// UMassActorSpawnerSubsystem::SpawnOrRetrieveFromPool calls the virtual
// SpawnActor whenever the actor pool has nothing to hand back. The override
// opens with:
//
//     mov  rax, cs:GFrameCounter
//     mov  r15, r8
//     mov  r12, rdx
//     mov  rbp, rcx
//     cmp  [rcx+0F0h], rax          ; this->LastSpawnFrame
//     jnz  short carry_on           ; <- the two bytes we patch
//     mov  al, 5                    ; ESpawnRequestStatus::RetryPending
//
// That is a hard cap of one building actor spawned per frame, independent of
// the wall-clock budget UMassActorSpawnerSubsystem::ProcessPendingSpawningRequest
// already applies once per frame at the PrePhysics phase.
//
// It only bites on a cold load. A pool hit returns Succeeded before ever
// reaching SpawnActor, so re-showing a building you walked away from is
// instant; but after a level load the pool is empty and every visible building
// has to come through here, one per frame. It also costs more than it saves:
// ProcessSpawnRequest rewrites any status that is neither Succeeded(3) nor
// Failed(4) to RetryPending(5) and leaves the handle queued, so the rest of the
// frame's time budget is burnt re-scanning requests that cannot succeed.
//
// Turning the conditional jump into an unconditional one removes the cap and
// leaves the time slice as the only throttle -- which is the mechanism the
// engine was written around, and which still bounds the per-frame work.
//
// The pattern was verified to match exactly once in both shipping binaries,
// and the function is byte-identical between them apart from RIP
// displacements: StarRuptureGameSteam-Win64-Shipping.exe (client) and
// StarRuptureServerEOS-Win64-Shipping.exe (dedicated server).
// ---------------------------------------------------------------------------

static constexpr const char* kHookName = "UCrMassBuildingSpawnerSubsystem::SpawnActor (per-frame gate)";

static constexpr const char* kSpawnGatePattern =
    "48 8B 05 ?? ?? ?? ?? 4D 8B F8 4C 8B E2 48 8B E9 48 39 81 F0 00 00 00 75 07 B0 05";

// Offset of the `jnz` from the start of the match.
static constexpr uintptr_t kJnzOffset = 0x17;

static constexpr uint8_t kOriginalBytes[2] = { 0x75, 0x07 }; // jnz short +7
static constexpr uint8_t kPatchedBytes[2]  = { 0xEB, 0x07 }; // jmp short +7

static uintptr_t g_jnzAddress = 0;
static bool      g_patched    = false;

void ResolveSpawnGate(IPluginSelf* self, IPluginHookScanner* scanner)
{
    if (!self || !scanner)
        return;

    const uintptr_t match = scanner->ResolveRequired(self, kHookName, kSpawnGatePattern);
    if (!match)
        return; // ResolveRequired recorded the miss; the loader refuses the plugin.

    // The pattern already pins these two bytes, so a mismatch here means the
    // offset arithmetic is wrong rather than the game having changed. Say so
    // instead of writing over a byte we have not actually identified.
    const uintptr_t jnz = match + kJnzOffset;

    uint8_t seen[2] = { 0, 0 };
    std::memcpy(seen, reinterpret_cast<const void*>(jnz), sizeof(seen));

    if (seen[0] != kOriginalBytes[0] || seen[1] != kOriginalBytes[1])
    {
        char detail[128];
        std::snprintf(detail, sizeof(detail),
                      "expected 75 07 at match+0x%02X, found %02X %02X",
                      static_cast<unsigned>(kJnzOffset), seen[0], seen[1]);
        scanner->ReportFailure(self, kHookName, detail);
        return;
    }

    g_jnzAddress = jnz;
}

bool ApplySpawnGatePatch()
{
    if (g_patched)
        return true;

    if (!g_jnzAddress)
    {
        LOG_WARN("SpawnGate: address unresolved -- per-frame limit left in place");
        return false;
    }

    IPluginHooks* hooks = GetHooks();
    if (!hooks || !hooks->Memory)
    {
        LOG_ERROR("SpawnGate: memory interface unavailable");
        return false;
    }

    // A single two-byte store over a jcc. SpawnActor only ever runs on the game
    // thread, and the loader's own detour machinery writes 14 bytes into live
    // functions the same way, so this is the smaller of the two risks already
    // being taken.
    if (!hooks->Memory->Patch(g_jnzAddress, kPatchedBytes, sizeof(kPatchedBytes)))
    {
        LOG_ERROR("SpawnGate: patch failed at 0x%llX",
                  static_cast<unsigned long long>(g_jnzAddress));
        return false;
    }

    g_patched = true;
    LOG_INFO("SpawnGate: per-frame building spawn cap removed (0x%llX: 75 07 -> EB 07)",
             static_cast<unsigned long long>(g_jnzAddress));
    return true;
}

void RestoreSpawnGatePatch()
{
    if (!g_patched)
        return;

    IPluginHooks* hooks = GetHooks();
    if (!hooks || !hooks->Memory)
    {
        // Nothing we can do -- leaving the patch in is better than pretending.
        LOG_ERROR("SpawnGate: memory interface gone, cap patch left applied at 0x%llX",
                  static_cast<unsigned long long>(g_jnzAddress));
        return;
    }

    if (!hooks->Memory->Patch(g_jnzAddress, kOriginalBytes, sizeof(kOriginalBytes)))
    {
        LOG_ERROR("SpawnGate: restore failed at 0x%llX",
                  static_cast<unsigned long long>(g_jnzAddress));
        return;
    }

    g_patched = false;
    LOG_INFO("SpawnGate: per-frame building spawn cap restored");
}

bool      IsSpawnGatePatched()  { return g_patched; }
uintptr_t GetSpawnGateAddress() { return g_jnzAddress; }
