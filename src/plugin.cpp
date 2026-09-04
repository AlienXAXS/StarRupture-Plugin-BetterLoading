#include "plugin.h"
#include "loading_apply.h"
#include "loading_config.h"
#include "loading_console.h"
#include "plugin_helpers.h"
#include "spawn_budget.h"
#include "spawn_gate.h"
#include "Engine_classes.hpp"
#include <cstring>

// Global plugin self pointer -- stable for the plugin's lifetime. Set in
// OnPluginLoadHooks (which runs first) so the LOG_* macros work there too.
static IPluginSelf* g_self = nullptr;

IPluginSelf* GetSelf() { return g_self; }

#ifndef MODLOADER_BUILD_TAG
#define MODLOADER_BUILD_TAG "dev"
#endif

#if defined(MODLOADER_SERVER_BUILD)
#define PLUGIN_TARGET_THIS PLUGIN_TARGET_SERVER
#else
#define PLUGIN_TARGET_THIS PLUGIN_TARGET_CLIENT
#endif

static PluginInfo s_pluginInfo = {
	"BetterLoading",
	MODLOADER_BUILD_TAG,
	"AlienX",
	"Removes the one-building-per-frame cap on Mass actor spawning so bases stop trickling in after a load.",
	PLUGIN_INTERFACE_VERSION,
	PLUGIN_TARGET_THIS
};

// The CDO is not necessarily reachable when PluginInit runs, so the budget half
// of the state is re-applied whenever the engine gives us another chance. Both
// halves are idempotent.
static void OnEngineInit()
{
	LOG_DEBUG("OnEngineInit: applying state");
	ApplyStateFromConfig();
}

static void OnWorldBeginPlay(SDK::UWorld* /*world*/, const char* worldName)
{
	LOG_DEBUG("OnWorldBeginPlay: world=%s -- reapplying state", worldName ? worldName : "(null)");
	ApplyStateFromConfig();
}

static void OnConfigChanged(const char* section, const char* key, const char* newValue)
{
	LOG_DEBUG("OnConfigChanged: [%s] %s = %s",
		section  ? section  : "(null)",
		key      ? key      : "(null)",
		newValue ? newValue : "(null)");

	// Every key this plugin owns feeds the same apply step, so there is nothing
	// to dispatch on -- and nothing to forget when a key is added.
	ApplyStateFromConfig();
}

extern "C" {

	__declspec(dllexport) PluginInfo* GetPluginInfo()
	{
		return &s_pluginInfo;
	}

	// Runs after GetPluginInfo and before PluginInit, and is the only context in
	// which the loader lets a plugin pattern scan. Resolve here, install from
	// PluginInit -- self->hooks is null for the duration of this event, and the
	// loader frees this DLL if anything misses.
	__declspec(dllexport) void OnPluginLoadHooks(IPluginSelf* self, IPluginHookScanner* scanner)
	{
		g_self = self;
		ResolveSpawnGate(self, scanner);
	}

	__declspec(dllexport) bool PluginInit(IPluginSelf* self)
	{
		g_self = self;

		LOG_INFO("BetterLoading initialising...");

		LoadingConfig::Config::Initialize(self);

		// Everything below is registered even when the plugin is switched off, so
		// that switching it back on -- from the config UI or the console -- takes
		// effect without a reload. ApplyStateFromConfig restores the game's own
		// behaviour when Enabled is false, so registering alone changes nothing.
		if (!LoadingConfig::Config::IsEnabled())
			LOG_WARN("Disabled in config -- loading behaviour left untouched");

		if (self->hooks && self->hooks->Engine)
			self->hooks->Engine->RegisterOnInit(&OnEngineInit);

		if (self->hooks && self->hooks->World)
			self->hooks->World->RegisterOnAnyWorldBeginPlay(&OnWorldBeginPlay);

		// Client only -- null on server builds, where the console command is the
		// way in instead.
		if (self->hooks && self->hooks->UI)
			self->hooks->UI->RegisterOnConfigChanged(self, &OnConfigChanged);

		RegisterLoadingConsoleCommand(self);

		// The gate patch needs nothing from the engine and lands right now; the
		// CDO budget is picked up here if the engine is already up (hot reload)
		// and otherwise on the first of OnEngineInit / OnWorldBeginPlay.
		ApplyStateFromConfig();

		LOG_INFO("BetterLoading initialised");
		return true;
	}

	__declspec(dllexport) void PluginShutdown()
	{
		LOG_INFO("BetterLoading shutting down -- restoring original behaviour");

		RestoreOriginalState();

		if (g_self)
		{
			UnregisterLoadingConsoleCommand(g_self);

			if (g_self->hooks && g_self->hooks->Engine)
				g_self->hooks->Engine->UnregisterOnInit(&OnEngineInit);

			if (g_self->hooks && g_self->hooks->World)
				g_self->hooks->World->UnregisterOnAnyWorldBeginPlay(&OnWorldBeginPlay);

			if (g_self->hooks && g_self->hooks->UI)
				g_self->hooks->UI->UnregisterOnConfigChanged(g_self, &OnConfigChanged);
		}

		g_self = nullptr;
	}

} // extern "C"
