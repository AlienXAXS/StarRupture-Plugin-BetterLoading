#pragma once

#include <plugin_interface.h>

namespace LoadingConfig
{
    // 0 for either of the two numeric knobs means "leave the engine's own value
    // alone", which is how you isolate one fix from the other when testing.
    static const ConfigEntry CONFIG_ENTRIES[] = {
        {
            "General", "Enabled", ConfigValueType::Boolean, "true",
            "Enable or disable BetterLoading entirely.",
            0.0f, 1.0f
        },
        {
            "Spawning", "RemovePerFrameLimit", ConfigValueType::Boolean, "true",
            "Remove the game's hard cap of one building actor spawned per frame. "
            "This is the main fix: after a level load the actor pool is empty, so "
            "every building around you has to come through that cap one frame at a time.",
            0.0f, 1.0f
        },
        {
            "Spawning", "SpawnTimeSliceMs", ConfigValueType::Float, "4.0",
            "Milliseconds per frame the engine may spend spawning Mass actors. "
            "The game ships 1.5. Only spent when something is actually waiting to "
            "spawn, so raising it costs nothing once the world is loaded. "
            "0 = leave the game's own value alone.",
            0.0f, 33.0f
        },
        {
            "Advanced", "FailedSpawnRetrySeconds", ConfigValueType::Float, "0",
            "Seconds before a Mass actor whose spawn genuinely failed is retried. "
            "The game ships 5.0, which is ten times stock Unreal. Only affects "
            "worlds loaded after the change. 0 = leave the game's own value alone.",
            0.0f, 30.0f
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

        static bool  IsEnabled()                  { return s_self ? s_self->config->ReadBool (s_self, "General",  "Enabled",                 true)  : true;  }
        static bool  ReadRemovePerFrameLimit()    { return s_self ? s_self->config->ReadBool (s_self, "Spawning", "RemovePerFrameLimit",     true)  : true;  }
        static float ReadSpawnTimeSliceMs()       { return s_self ? s_self->config->ReadFloat(s_self, "Spawning", "SpawnTimeSliceMs",        4.0f)  : 4.0f;  }
        static float ReadFailedSpawnRetrySeconds(){ return s_self ? s_self->config->ReadFloat(s_self, "Advanced", "FailedSpawnRetrySeconds", 0.0f)  : 0.0f;  }

        static bool WriteSpawnTimeSliceMs(float ms)
        {
            return s_self && s_self->config->WriteFloat(s_self, "Spawning", "SpawnTimeSliceMs", ms);
        }

        static bool WriteRemovePerFrameLimit(bool on)
        {
            return s_self && s_self->config->WriteBool(s_self, "Spawning", "RemovePerFrameLimit", on);
        }

    private:
        static IPluginSelf* s_self;
    };
}
