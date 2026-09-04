#include "loading_console.h"
#include "loading_apply.h"
#include "loading_config.h"
#include "spawn_budget.h"
#include "spawn_gate.h"
#include "plugin_helpers.h"
#include <plugin_interface.h>
#include <cstdlib>
#include <cstring>

static constexpr const char* kCommandName = "betterloading";

static bool StrEqualI(const char* a, const char* b)
{
    return a && b && _stricmp(a, b) == 0;
}

static void PrintStatus(IPluginConsole* console, PluginConsoleSink sink)
{
    console->Write(sink, PluginConsoleLineKind::Notice, "BetterLoading");

    console->Printf(sink, PluginConsoleLineKind::Output,
                    "  enabled            : %s",
                    LoadingConfig::Config::IsEnabled() ? "yes" : "no");

    const uintptr_t gate = GetSpawnGateAddress();
    if (gate)
    {
        console->Printf(sink, PluginConsoleLineKind::Output,
                        "  per-frame cap      : %s (gate at 0x%llX)",
                        IsSpawnGatePatched() ? "removed" : "in place",
                        static_cast<unsigned long long>(gate));
    }
    else
    {
        console->Write(sink, PluginConsoleLineKind::Error,
                       "  per-frame cap      : gate address unresolved");
    }

    if (IsBudgetCaptured())
    {
        console->Printf(sink, PluginConsoleLineKind::Output,
                        "  spawn time slice   : %.2f ms  (game default %.2f ms)",
                        GetSpawnSliceSeconds() * 1000.0,
                        GetDefaultSpawnSliceSeconds() * 1000.0);
        console->Printf(sink, PluginConsoleLineKind::Output,
                        "  failed-spawn retry : %.2f s   (game default %.2f s)",
                        GetRetrySeconds(), GetDefaultRetrySeconds());
    }
    else
    {
        console->Write(sink, PluginConsoleLineKind::Error,
                       "  spawn time slice   : UMassSimulationSettings CDO not reachable");
    }
}

static void HandleCommand(const char* const* argv, int argc, PluginConsoleSink sink, void*)
{
    IPluginHooks* hooks = GetHooks();
    if (!hooks || !hooks->Console)
        return;

    IPluginConsole* console = hooks->Console;

    if (argc < 2 || StrEqualI(argv[1], "status"))
    {
        PrintStatus(console, sink);
        return;
    }

    if (StrEqualI(argv[1], "slice"))
    {
        if (argc < 3)
        {
            console->Write(sink, PluginConsoleLineKind::Error,
                           "usage: betterloading slice <milliseconds|default>");
            return;
        }

        // "default" is written to the ini as 0, which ApplyBudgetFromConfig
        // reads as "put the engine's own value back".
        float ms = 0.0f;
        if (!StrEqualI(argv[2], "default"))
        {
            ms = static_cast<float>(std::atof(argv[2]));
            if (ms <= 0.0f || ms > 33.0f)
            {
                console->Write(sink, PluginConsoleLineKind::Error,
                               "slice must be between 0 and 33 ms, or 'default'");
                return;
            }
        }

        LoadingConfig::Config::WriteSpawnTimeSliceMs(ms);
        ApplyStateFromConfig();

        console->Printf(sink, PluginConsoleLineKind::Output,
                        "spawn time slice now %.2f ms", GetSpawnSliceSeconds() * 1000.0);
        return;
    }

    if (StrEqualI(argv[1], "limit"))
    {
        if (argc < 3 || (!StrEqualI(argv[2], "on") && !StrEqualI(argv[2], "off")))
        {
            console->Write(sink, PluginConsoleLineKind::Error,
                           "usage: betterloading limit <on|off>   (on = the game's one-per-frame cap)");
            return;
        }

        // "on" means the game's cap is in force, i.e. our patch is off.
        const bool capOn = StrEqualI(argv[2], "on");
        LoadingConfig::Config::WriteRemovePerFrameLimit(!capOn);
        ApplyStateFromConfig();

        console->Printf(sink, PluginConsoleLineKind::Output,
                        "per-frame building spawn cap is now %s",
                        IsSpawnGatePatched() ? "removed" : "in place");
        return;
    }

    console->Printf(sink, PluginConsoleLineKind::Error, "unknown subcommand '%s'", argv[1]);
    console->Write(sink, PluginConsoleLineKind::Notice,
                   "betterloading [status] | slice <ms|default> | limit <on|off>");
}

void RegisterLoadingConsoleCommand(IPluginSelf* self)
{
    if (!self || !self->hooks || !self->hooks->Console)
        return;

    PluginConsoleCommandDesc desc = {};
    desc.name       = kCommandName;
    desc.aliases    = nullptr;
    desc.usage      = "betterloading [status] | slice <ms|default> | limit <on|off>";
    desc.help       = "Inspect and tune building spawn-in rate after a level load.";
    desc.handler    = &HandleCommand;
    desc.userData   = nullptr;
    // Touches the UMassSimulationSettings CDO and patches game code, so it has
    // to run where the engine expects those to change.
    desc.gameThread = true;

    if (!self->hooks->Console->RegisterCommand(self, &desc))
        LOG_WARN("Console: could not register '%s' -- name already taken?", kCommandName);
    else
        LOG_DEBUG("Console: '%s' registered", kCommandName);
}

void UnregisterLoadingConsoleCommand(IPluginSelf* self)
{
    if (!self || !self->hooks || !self->hooks->Console)
        return;

    self->hooks->Console->UnregisterAllCommands(self);
}
