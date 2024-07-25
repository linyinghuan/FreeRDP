/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server Session Capture Module
 *
 * Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include "auditor.h"
#include "auditor_kb.h"
#include "auditor_mouse.h"
#include "auditor_channel.h"

#define TAG MODULE_TAG("auditor")

#define PLUGIN_NAME "auditor"
#define PLUGIN_DESC "proxy data/event auditor plugin"


static proxyPluginsManager* g_plugins_manager = NULL;


static BOOL auditor_plugin_unload(void)
{
	return TRUE;
}

static proxyPlugin auditor_plugin = {
	PLUGIN_NAME,                        /* name */
	PLUGIN_DESC,                        /* description */
	auditor_plugin_unload,              /* PluginUnload */
	NULL,                               /* ClientPreConnect */
	NULL, 								/* ClientPostConnect */
	NULL,                               /* ClientLoginFailure */
	NULL,    							/* ClientEndPaint */
	NULL, 								/* ServerPostConnect */
	NULL,                               /* ServerChannelsInit */
	NULL,                               /* ServerChannelsFree */
	NULL,         						/* Session End */
	NULL,                               /* KeyboardEvent */
	NULL,                               /* MouseEvent */
	NULL,                               /* ClientChannelData */
	NULL,                               /* ServerChannelData */
};

BOOL proxy_module_entry_point(proxyPluginsManager* plugins_manager)
{
	g_plugins_manager = plugins_manager;

	//auditor_plugin.MouseEvent = auditor_mouse_event_handler;
	//auditor_plugin.KeyboardEvent = auditor_keyboard_event_handler;
	auditor_plugin.ServerChannelsInit = auditor_server_channels_init;
	auditor_plugin.ClientChannelData = auditor_client_channel_handler;
	auditor_plugin.ServerChannelData = auditor_server_channel_handler;
	return plugins_manager->RegisterPlugin(&auditor_plugin);
}
