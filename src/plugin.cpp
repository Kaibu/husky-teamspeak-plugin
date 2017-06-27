/*
 * Husky Teamspeak Plugin
 *
 * Author: Kaibu
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include <sstream>

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 22

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

std::string uint64_to_string(uint64 value) {
	std::ostringstream os;
	os << value;
	return os.str();
}

const wchar_t * char_to_wc(const char *c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

void log(const char* message, LogLevel level = LogLevel_DEVEL)
{
	ts3Functions.logMessage(message, level, "ReallifeRPG Support",0);
}

void openPanelviaUID(uint64 schid /* serverConnectionHandlerID */, uint64 clientID) {
	char* uid;
	char* base = "https://support.realliferpg.de/register/uid?uid=";
	char url[100];
	if (ts3Functions.getClientVariableAsString(schid, clientID, CLIENT_UNIQUE_IDENTIFIER, &uid) == ERROR_ok) {
		strcpy(url, base);
		strcat(url, uid);
		ShellExecute(0, 0, char_to_wc(url), 0, 0, SW_SHOW);
	}
}

void copyUIDtoClipboard(uint64 schid /* serverConnectionHandlerID */, uint64 clientID) {
	char* uid;
	if (ts3Functions.getClientVariableAsString(schid, clientID, CLIENT_UNIQUE_IDENTIFIER, &uid) == ERROR_ok) {
		const size_t len = strlen(uid) + 1;
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
		memcpy(GlobalLock(hMem), uid, len);
		GlobalUnlock(hMem);
		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
	}
}

void openPanel() {
	ShellExecute(0, 0, L"https://support.realliferpg.de/", 0, 0, SW_SHOW);
}

const char* ts3plugin_name() {
	return "ReallifeRPG Support";
}

const char* ts3plugin_version() {
    return "1.0";
}

int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

const char* ts3plugin_author() {
    return "github.com/Kaibu";
}

const char* ts3plugin_description() {
    return "ReallifeRPG Support Plugin.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

    return 0;
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

const char* ts3plugin_infoTitle() {
	return "ReallifeRPG Support";
}

void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	char* uid;

	switch(type) {
		case PLUGIN_CLIENT:
			if(ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_UNIQUE_IDENTIFIER, &uid) != ERROR_ok) {
				data = NULL;
				return;
			}
			break;
		default:
			data = NULL;  /* Ignore */
			return;
	}

	*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
	snprintf(*data, INFODATA_BUFSIZE, "Die UID des Clients ist: [I]%s[/I]", uid);
	ts3Functions.freeMemory(uid);
}

void ts3plugin_freeMemory(void* data) {
	free(data);
}

int ts3plugin_requestAutoload() {
	return 0;
}

static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

enum {
	MENU_ID_CLIENT_1 = 1,
	MENU_ID_CLIENT_2,
	MENU_ID_GLOBAL_1,
};

void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	BEGIN_CREATE_MENUS(3);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT,  MENU_ID_CLIENT_1,  "Profil",  "client_open.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT,  MENU_ID_CLIENT_2,  "UID Kopieren",  "client_copy.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL,  MENU_ID_GLOBAL_1,  "Panel",  "plugin.png");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "plugin.png");
}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	switch(type) {
		case PLUGIN_MENU_TYPE_GLOBAL:
			switch(menuItemID) {
				case MENU_ID_GLOBAL_1:
					openPanel();
					break;
				default:
					break;
			}
			break;
		case PLUGIN_MENU_TYPE_CLIENT:
			switch(menuItemID) {
				case MENU_ID_CLIENT_1:
					openPanelviaUID(serverConnectionHandlerID, selectedItemID);
					break;
				case MENU_ID_CLIENT_2:
					copyUIDtoClipboard(serverConnectionHandlerID, selectedItemID);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}