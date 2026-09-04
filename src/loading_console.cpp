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

// Upper bound on the session override. Two frames' worth at 60 fps is already
// far past anything defensible; the point is to catch a typo, not to express a
// policy.
static constexpr float kMaxSliceMs = 33.0f;

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
                        IsSpawnGatePatched() ? "removed" : "in force",
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
                        "  spawn time slice   : %.2f ms  (game ships %.2f ms)",
                        GetSpawnSliceSeconds() * 1000.0,
                        GetDefaultSpawnSliceSeconds() * 1000.0);
        console->Printf(sink, PluginConsoleLineKind::Output,
                        "  failed-spawn retry : %.2f s   (game value, not changed)",
                        GetRetrySeconds());
    }
    else
    {
        console->Write(sink, PluginConsoleLineKind::Error,
                       "  spawn time slice   : UMassSimulationSettings CDO not reachable");
    }

    const bool overridden = (GetSliceOverrideMs() > 0.0f) || GetKeepGameCap();
    if (overridden)
    {
        console->Write(sink, PluginConsoleLineKind::Notice,
                       "  session overrides active -- not saved, gone on reload or restart");
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

        float ms = kNoSliceOverride;
        if (!StrEqualI(argv[2], "default"))
        {
            ms = static_cast<float>(std::atof(argv[2]));
            if (ms <= 0.0f || ms > kMaxSliceMs)
            {
                console->Printf(sink, PluginConsoleLineKind::Error,
                                "slice must be between 0 and %.0f ms, or 'default'", kMaxSliceMs);
                return;
            }
        }

        SetSliceOverrideMs(ms);

        console->Printf(sink, PluginConsoleLineKind::Output,
                        "spawn time slice now %.2f ms", GetSpawnSliceSeconds() * 1000.0);
        console->Write(sink, PluginConsoleLineKind::Notice,
                       "session only -- not saved to the ini");
        return;
    }

    if (StrEqualI(argv[1], "cap"))
    {
        if (argc < 3 || (!StrEqualI(argv[2], "keep") && !StrEqualI(argv[2], "remove")))
        {
            console->Write(sink, PluginConsoleLineKind::Error,
                           "usage: betterloading cap <keep|remove>");
            console->Write(sink, PluginConsoleLineKind::Notice,
                           "  keep   = leave the game's one-per-frame cap in force (stock behaviour)");
            console->Write(sink, PluginConsoleLineKind::Notice,
                           "  remove = take it out (what the plugin does by default)");
            return;
        }

        SetKeepGameCap(StrEqualI(argv[2], "keep"));

        console->Printf(sink, PluginConsoleLineKind::Output,
                        "per-frame building spawn cap is now %s",
                        IsSpawnGatePatched() ? "removed" : "in force");
        console->Write(sink, PluginConsoleLineKind::Notice,
                       "session only -- not saved to the ini");
        return;
    }

    console->Printf(sink, PluginConsoleLineKind::Error, "unknown subcommand '%s'", argv[1]);
    console->Write(sink, PluginConsoleLineKind::Notice,
                   "betterloading [status] | slice <ms|default> | cap <keep|remove>");
}

void RegisterLoadingConsoleCommand(IPluginSelf* self)
{
    if (!self || !self->hooks || !self->hooks->Console)
        return;

    PluginConsoleCommandDesc desc = {};
    desc.name       = kCommandName;
    desc.aliases    = nullptr;
    desc.usage      = "betterloading [status] | slice <ms|default> | cap <keep|remove>";
    desc.help       = "Inspect building spawn-in behaviour, and override it for this session.";
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
