#pragma once

#include <plugin_interface.h>

namespace LoadingConfig
{
    // One key, deliberately.
    //
    // The spawn time slice and the cap patch used to be config entries with
    // sliders in the editor. They are not settings -- installing BetterLoading
    // *is* the decision, and a plugin that has to be tuned before it does the
    // thing it is named after has moved its own job onto the user. Worse, a
    // slice value someone typed during one test and left in an ini is a
    // permanent, invisible frame-time cost with nothing on screen to explain
    // it. The loader makes exactly this argument about per-plugin log levels
    // and refuses to persist those either.
    //
    // Both live on as session-only overrides on the `betterloading` console
    // command, which is where a debugging tool belongs: per-run, and visible in
    // the thing that caused it.
    static const ConfigEntry CONFIG_ENTRIES[] = {
        {
            "General", "Enabled", ConfigValueType::Boolean, "true",
            "Enable or disable BetterLoading. When on, the game's one-building-per-frame "
            "spawn cap is removed and the Mass actor spawn budget is raised, with no "
            "further setup.",
            0.0f, 1.0f
        }
    };

    static const ConfigSchema SCHEMA = {
        CONFIG_ENTRIES,
        sizeof(CONFIG_ENTRIES) / sizeof(ConfigEntry)
    };

    class Config
    {
    public:
        static void Initialize(IPluginSelf* self)
        {
            s_self = self;
            if (s_self)
                s_self->config->InitializeFromSchema(s_self, &SCHEMA);
        }

        static bool IsEnabled()
        {
            return s_self ? s_self->config->ReadBool(s_self, "General", "Enabled", true) : true;
        }

    private:
        static IPluginSelf* s_self;
    };
}
