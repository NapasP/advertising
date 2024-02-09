#include <stdio.h>
#include <regex>
#include "advertisement.h"
#include <iserver.h>
#include <fstream>
#include <vector>
#include <iostream>
#include "utils/module.h"
#include "colors/colors.h"
#include <KeyValues.h>
#include "ctimer.h"

IFileSystem* filesystem = NULL;
CMemory fn;

int countAdv;

float g_flUniversalTime;
float g_flLastTickedTime;
bool g_bHasTicked;

struct BlockAdv {
	int dest;
	std::string text;
};

std::vector< BlockAdv > advs;

AdvertisementPlugin g_AdvertisementPlugin;
IServerGameDLL *server = NULL;

class GameSessionConfiguration_t { };

void (*UTIL_ClientPrintAll)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) = nullptr;

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);

CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

PLUGIN_EXPOSE(AdvertisementPlugin, g_AdvertisementPlugin);
bool AdvertisementPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &AdvertisementPlugin::Hook_GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &AdvertisementPlugin::Hook_StartupServer, true);

	KeyValues* kv = new KeyValues("advertisement");
	kv->LoadFromFile(filesystem, "addons/advertisement/advertisement.ini");

	new CTimer(kv->GetInt("update time"), true, true, []() {
		auto&adv = advs[countAdv];

		UTIL_ClientPrintAll(adv.dest, adv.text.c_str(), nullptr, nullptr, nullptr, nullptr);

		countAdv = (countAdv+1)%advs.size();
	});

	KeyValues* listAdv = kv->FindKey("list");
	if (listAdv) {
		for ( KeyValues* pKey = listAdv->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
		{
			std::string text(pKey->GetString("text"));

			for (int i = 0; i < std::size(colors_hex); i++) {
				text = ReplaceAll(text, colors_text[i], colors_hex[i]);
			}

			advs.push_back({pKey->GetInt("dest"), std::move(text)});
		}
	}

	delete kv;

	CModule libserver(server);

	//Only linux
	UTIL_ClientPrintAll = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 D7 41 56 49 89 F6 41 55 41 89 FD").RCast< decltype(UTIL_ClientPrintAll) >();
	if (!UTIL_ClientPrintAll)
	{
		V_strncpy(error, "Failed to find function to get UTIL_ClientPrintAll", maxlen);
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);

		return false;
	}

	return true;
}

bool AdvertisementPlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &AdvertisementPlugin::Hook_GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &AdvertisementPlugin::Hook_StartupServer, true);

	RemoveTimers();

	return true;
}

void AdvertisementPlugin::Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*)
{
	if(g_bHasTicked)
		RemoveMapTimers();

	g_bHasTicked = false;
}

void AdvertisementPlugin::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	if (simulating && g_bHasTicked)
	{
		g_flUniversalTime += GetGameGlobals()->curtime - g_flLastTickedTime;
	}
	else
	{
		g_flUniversalTime += GetGameGlobals()->interval_per_tick;
	}

	g_flLastTickedTime = GetGameGlobals()->curtime;
	g_bHasTicked = true;

	for (int i = g_timers.Tail(); i != g_timers.InvalidIndex();)
	{
		auto timer = g_timers[i];

		int prevIndex = i;
		i = g_timers.Previous(i);

		if (timer->m_flLastExecute == -1)
			timer->m_flLastExecute = g_flUniversalTime;

		// Timer execute
		if (timer->m_flLastExecute + timer->m_flTime <= g_flUniversalTime)
		{
			timer->Execute();

			if (!timer->m_bRepeat)
			{
				delete timer;
				g_timers.Remove(prevIndex);
			}
			else
				timer->m_flLastExecute = g_flUniversalTime;
		}
	}
}

bool AdvertisementPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool AdvertisementPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *AdvertisementPlugin::GetLicense()
{
	return "Free";
}

const char *AdvertisementPlugin::GetVersion()
{
	return "1.2";
}

const char *AdvertisementPlugin::GetDate()
{
	return __DATE__;
}

const char *AdvertisementPlugin::GetLogTag()
{
	return "Adv";
}

const char *AdvertisementPlugin::GetAuthor()
{
	return "Napas";
}

const char *AdvertisementPlugin::GetDescription()
{
	return "Show advertisement after time";
}

const char *AdvertisementPlugin::GetName()
{
	return "Advertisement";
}

const char *AdvertisementPlugin::GetURL()
{
	return "https://napas-project.com";
}
