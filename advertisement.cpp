#include <stdio.h>
#include <regex>
#include "advertisement.h"
#include "colors/colors.h"
#include "iserver.h"
#include <fstream>
#include <vector>
#include <iostream>
#include "utils/module.h"
#include "KeyValues.h"

IFileSystem* filesystem = NULL;
CMemory fn;

int tickTime;
int countAdv;
int updateTime;

struct BlockAdv {
	int dest;
	std::string text;
};

std::vector< BlockAdv > advs;

AdvertisementPlugin g_AdvertisementPlugin;
IServerGameDLL *server = NULL;

// void UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
void (*UTIL_ClientPrintAll)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) = nullptr;

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);

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

	KeyValues* kv = new KeyValues("advertisement");
	kv->LoadFromFile(filesystem, "addons/advertisement/advertisement.ini");

	updateTime = kv->GetInt("update time");

	const KeyValues *listAdv = kv->FindKey("list");
	if (listAdv) {
		for ( KeyValues *pKey = listAdv->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
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

	return true;
}

void AdvertisementPlugin::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	if (GetGameGlobals()->tickcount >= tickTime) {
		auto&adv = advs[countAdv];

		UTIL_ClientPrintAll(adv.dest, adv.text.c_str(), nullptr, nullptr, nullptr, nullptr);

		countAdv = (countAdv+1)%advs.size();

		//Hardcode to tick
		tickTime += updateTime*64;
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
	return "1.0";
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
