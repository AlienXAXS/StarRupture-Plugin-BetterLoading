# BetterLoading

A [StarRupture ModLoader](https://github.com/AlienXAXS/StarRupture-ModLoader) plugin that stops your base
trickling in one building at a time after a level load.

## The problem

Load a save with a large base and the buildings around you pop in over tens of seconds, a couple per second,
long after the loading screen has gone. It looks like the save is still being read. It isn't — the save JSON is
fully decompressed and parsed before the world is even handed over.

What is actually happening is a hard cap in the game's own building spawner:

```asm
; UCrMassBuildingSpawnerSubsystem::SpawnActor
mov  rax, cs:GFrameCounter
cmp  [rcx+0F0h], rax          ; this->LastSpawnFrame
jnz  short carry_on
mov  al, 5                    ; ESpawnRequestStatus::RetryPending
```

**One building actor spawned per frame.** Not a time budget — a frame counter.

Every building is a Mass entity whose visual actor is spawned on demand by
`UCrMassBuildingRepresentationSubsystem`, which points its `ActorSpawnerSubsystem` at
`UCrMassBuildingSpawnerSubsystem`. A pool hit returns before ever reaching `SpawnActor`, which is why walking
around mid-session feels fine — but after a load the pool is empty and every visible building has to queue
through that cap. At 60 fps that is 60 buildings a second; while the game is still hitching through a load it
is closer to 30.

It also costs more than it saves. `ProcessSpawnRequest` rewrites any status that is neither `Succeeded` nor
`Failed` to `RetryPending` and leaves the handle in the queue, so once the frame's one spawn is used up the
remaining time budget is spent re-scanning requests that cannot succeed — and `GetNextRequestToSpawn` is an
O(N) walk of the whole handle array per pick.

## What this plugin does

**1. Removes the per-frame cap.** Two bytes: `75 07` (`jnz`) becomes `EB 07` (`jmp`), so the early
`RetryPending` return is skipped. The engine's own wall-clock budget in
`UMassActorSpawnerSubsystem::ProcessPendingSpawningRequest` is then the only throttle — which is the mechanism
Mass was written around, and it still bounds the per-frame work.

**2. Raises that budget.** `UMassSimulationSettings::DesiredActorSpawningTimeSlicePerTick` ships at 0.0015 s
(1.5 ms). Removing the cap without touching this leaves you spawning whatever fits in 1.5 ms, so the two go
together. The default here is 4 ms.

The budget costs nothing while idle: `ProcessPendingSpawningRequest` breaks out immediately when the request
queue is empty, so a raised slice is only ever spent when something is actually waiting to spawn.

Both changes are reverted on plugin shutdown or reload.

## Configuration

`<game_dir>/Plugins/config/BetterLoading.ini`, generated on first run.

| Section | Key | Default | Meaning |
|---|---|---|---|
| `General` | `Enabled` | `true` | Master switch. |
| `Spawning` | `RemovePerFrameLimit` | `true` | The byte patch. This is the main fix. |
| `Spawning` | `SpawnTimeSliceMs` | `4.0` | Milliseconds per frame for Mass actor spawning. Game ships 1.5. `0` = leave the game's value alone. |
| `Advanced` | `FailedSpawnRetrySeconds` | `0` | Retry delay for a spawn that genuinely failed. Game ships 5.0 (ten times stock Unreal). `0` = leave alone. |

Setting either numeric knob to `0` is how you isolate one fix from the other while testing.

`FailedSpawnRetrySeconds` only reaches worlds loaded *after* the change — `UMassRepresentationSubsystem::Initialize`
caches it into `RetryTimeInterval` when the world's subsystem is created. It is off by default because it only
matters for spawns that return `Failed`, which the building spawner should not produce: it sets
`SpawnCollisionHandlingOverride = AlwaysSpawn`.

On client builds the config UI applies changes live. Both numeric knobs and the cap patch also take effect
immediately from the console.

## Console

Registered in both console front-ends, which on a dedicated server is the only way to reach any of this.

```
betterloading                      # same as: betterloading status
betterloading slice 8              # set the spawn time slice, in milliseconds
betterloading slice default        # back to the game's 1.5 ms
betterloading limit off            # remove the one-per-frame cap (the default)
betterloading limit on             # put the game's cap back, for an A/B test
```

`status` prints the resolved gate address, whether the patch is applied, and the live vs. shipped values of
both CDO fields.

## Tuning

Start with the defaults. If load-in is still slower than you want, raise `SpawnTimeSliceMs` — 8 to 10 ms is
aggressive but the cost is only paid while there is a spawn backlog.

If removing the cap turns the trickle into a *hitch* rather than making it fast, that is worth knowing and
worth reporting: the likely culprit is `UCrBuildingStabilitySubsystem::AddPendingSpawner` being non-linear in
the number of spawners, which would be the real reason the cap exists. Back the time slice down rather than
putting the cap back, and the two together give you a dial between "slow and smooth" and "fast and choppy".

## Compatibility

The AOB was verified to match **exactly once** in both shipping binaries, and the target function is
byte-identical between them apart from RIP displacements:

- `StarRuptureGameSteam-Win64-Shipping.exe` (client)
- `StarRuptureServerEOS-Win64-Shipping.exe` (dedicated server)

The scan runs in `OnPluginLoadHooks`, so if a game update moves the function the loader refuses the plugin and
tells you why in the hook-failure window (or `hookfailures` in the console) rather than patching a byte it
cannot identify. The resolve also re-checks that the two bytes at the patch site really are `75 07` before
recording the address.

## Requirements

- Visual Studio 2022 (MSVC v143, C++20)
- [StarRupture-Game-SDK](https://github.com/AlienXAXS/StarRupture-Game-SDK)
- [StarRupture-Plugin-SDK](https://github.com/AlienXAXS/StarRupture-Plugin-SDK) (interface v63)
- StarRupture with ModLoader installed

Both SDKs are consumed from **sibling checkouts**:

```
GitHub/
├── StarRupture-Game-SDK/
├── StarRupture-Plugin-SDK/
└── StarRupture-Plugin-BetterLoading/   <- this repo
```

Adjust [`Shared.props`](Shared.props) if your layout differs, or override with
`/p:GameSDKRoot=... /p:PluginSDKInclude=...`.

## Building

```bat
msbuild BetterLoading.sln /p:Configuration="Client Release" /p:Platform=x64
```

Output lands at `build/<Configuration>/Plugins/BetterLoading.dll`.

| Configuration | Target | Define |
|---|---|---|
| `Client Debug` / `Client Release` | Game client | `MODLOADER_CLIENT_BUILD` |
| `Server Debug` / `Server Release` | Dedicated server | `MODLOADER_SERVER_BUILD` |

## Layout

| Path | Role |
|---|---|
| [`src/plugin.cpp`](src/plugin.cpp) | `GetPluginInfo` / `OnPluginLoadHooks` / `PluginInit` / `PluginShutdown` |
| [`src/spawn_gate.cpp`](src/spawn_gate.cpp) | The AOB and the two-byte patch, with the disassembly it is derived from |
| [`src/spawn_budget.cpp`](src/spawn_budget.cpp) | `UMassSimulationSettings` CDO knobs |
| [`src/loading_apply.cpp`](src/loading_apply.cpp) | Single place that turns config into applied state |
| [`src/loading_config.cpp`](src/loading_config.cpp) | Config schema and typed accessors |
| [`src/loading_console.cpp`](src/loading_console.cpp) | The `betterloading` command |
