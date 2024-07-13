/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server Demo C++ Module
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

#include <iostream>

#include "modules_api.h"

#define TAG MODULE_TAG("demo")

static constexpr char plugin_name[] = "demo2";
static constexpr char plugin_desc[] = "this is a test plugin2";

static proxyPluginsManager* g_plugins_manager = NULL;

static BOOL demo_filter_keyboard_event(proxyData* pdata, void* param)
{
	auto event_data = static_cast<proxyKeyboardEventInfo*>(param);
	if (event_data == NULL)
		return FALSE;

	/*
	if (event_data->rdp_scan_code == RDP_SCANCODE_KEY_B)
	{

		std::cout << "C++ demo plugin: aborting connection" << std::endl;
		g_plugins_manager->AbortConnect(pdata);
	}
	*/
			printf("input: %04X --\n", event_data->rdp_scan_code);
	if (event_data->rdp_scan_code == RDP_SCANCODE_KEY_A || event_data->rdp_scan_code == RDP_SCANCODE_LMENU ||
		event_data->rdp_scan_code == RDP_SCANCODE_DECIMAL || event_data->rdp_scan_code == RDP_SCANCODE_LCONTROL)
	{
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			printf("input: %04X KBD_FLAGS_RELEASE\n", event_data->rdp_scan_code);
			//std::cout << "C++ demo plugin: input" << std::hex << event_data->rdp_scan_code << ": KBD_FLAGS_RELEASE" << std::endl;
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			printf("input: %04X KBD_FLAGS_DOWN\n", event_data->rdp_scan_code);
			//std::cout << "C++ demo plugin: input" << std::hex << event_data->rdp_scan_code << ": KBD_FLAGS_DOWN" << std::endl;
		}
		/* user typed 'B', that means bye :) */
		std::cout << "C++ demo plugin: aborting connection" << std::endl;
	}


	return TRUE;
}

static BOOL demo_filter_mouse_event(proxyData* pdata, void* param)
{
	auto event_data = static_cast<proxyMouseEventInfo*>(param);
	if (event_data == NULL)
		return FALSE;

	if (event_data->flags & PTR_FLAGS_DOWN) {
		printf("mouse: %04X\n", event_data->flags);


		if (event_data->flags & PTR_FLAGS_DOWN)
			printf("PTR_FLAGS_DOWN ");
		if (event_data->flags & PTR_FLAGS_MOVE)
			printf("PTR_FLAGS_MOVE ");
		if (event_data->flags & PTR_FLAGS_BUTTON1)
			printf("left ");
		if (event_data->flags & PTR_FLAGS_BUTTON2)
			printf("right ");
		if (event_data->flags & PTR_FLAGS_BUTTON3)
			printf("middle ");

		if (event_data->flags & PTR_FLAGS_WHEEL)
			printf("wheel ");
		if (event_data->flags & PTR_FLAGS_HWHEEL)
			printf("hwheel ");

		printf("\n");
		std::cout << "C++ demo plugin mouse: input:" << event_data->flags << "--"  << event_data->x << "--" <<event_data->y <<std::endl;

	}




	return TRUE;
}


static BOOL demo_plugin_unload()
{
	std::cout << "C++ demo plugin: unloading..." << std::endl;
	return TRUE;
}

static proxyPlugin demo_plugin = {
	plugin_name,                /* name */
	plugin_desc,                /* description */
	demo_plugin_unload,         /* PluginUnload */
	NULL,                       /* ClientPreConnect */
	NULL,                       /* ClientLoginFailure */
	NULL,                       /* ServerPostConnect */
	NULL,                       /* ServerChannelsInit */
	NULL,                       /* ServerChannelsFree */
	NULL,			/* KeyboardEvent */
	NULL,                       /* MouseEvent */
	NULL,                       /* ClientChannelData */
	NULL,                       /* ServerChannelData */
};

UINT32 clientformatID = 0;
UINT32 serverformatID = 0;

wStream* server_data_in = NULL;
wStream* client_data_in = NULL;

static BOOL cliboard_filter_server_Event(proxyData* data, void* context) {
	auto pev = static_cast<proxyChannelDataEventInfo*>(context);
	UINT64 server_channel_id;

	if (pev->channel_id == 1006) {

		if (pev->flags & CHANNEL_FLAG_FIRST)
		{
			if (server_data_in == NULL) {
				server_data_in = Stream_New(NULL, pev->data_len);
			}
			Stream_SetPosition(server_data_in, 0);
		}

		wStream* data_in = server_data_in;
		if (!Stream_EnsureRemainingCapacity(data_in, pev->data_len))
		{
			return true;
		}

		Stream_Write(data_in, pev->data, pev->data_len);
		if (!(pev->flags & CHANNEL_FLAG_LAST)) {
			return true;
		}
		Stream_SetPosition(data_in, 0);
		auto s = (wStream*)data_in;

		UINT16 msgType;
		UINT16 msgFlags;
		UINT32 dataLen;
		UINT error;

		if (Stream_GetRemainingLength(s) < 8)
			return true;

		Stream_Read_UINT16(s, msgType);
		Stream_Read_UINT16(s, msgFlags);
		Stream_Read_UINT32(s, dataLen);

		if (Stream_GetRemainingLength(s) < dataLen)
			return true;

		if (msgType == CB_FORMAT_DATA_REQUEST) {

			if (Stream_GetRemainingLength(s) >= 4) {
				UINT32 formatID;
				Stream_Read_UINT32(s, formatID);
				serverformatID = formatID;
				std::cout << "C++ demo plugin: CB_FORMAT_DATA_REQUEST..." << serverformatID << std::endl;
			}
		}
		else if (msgType == CB_FORMAT_DATA_RESPONSE) {
			if (clientformatID == CF_UNICODETEXT ) {
				std::wcout << s->pointer <<std::endl;
			}
			else if (clientformatID == CB_FORMAT_TEXTURILIST ) {
				FILEDESCRIPTORW* file_descriptor_array;
				UINT32 file_descriptor_count;
				auto result = cliprdr_parse_file_list(s->pointer, dataLen, &file_descriptor_array, &file_descriptor_count);

				if (result == 0 && file_descriptor_count > 0) {
					LPSTR lpFileNameA;
					if (ConvertFromUnicode(CP_UTF8, 0, file_descriptor_array->cFileName, -1, &lpFileNameA, 0, NULL, NULL) < 1)
						return NULL;
					std::cout << "cliboard_filter_server_Event C++ demo plugin: CB_FORMAT_TEXTURILIST..." << lpFileNameA<< std::endl;

				}

				std::cout << "cliboard_filter_server_Event C++ demo plugin: CB_FORMAT_TEXTURILIST..." << result<< std::endl;
			}
			std::cout << "C++ demo plugin: CB_FORMAT_DATA_RESPONSE..." << std::endl;
		}

		data_in = NULL;
	}

	// Always return TRUE to allow data to pass through the filter
	return TRUE;
}


static BOOL cliboard_filter_client_Event(proxyData* data, void* context) {
	auto pev = static_cast<proxyChannelDataEventInfo*>(context);
	UINT64 server_channel_id;

	if (pev->channel_id == 1006) {

		if (pev->flags & CHANNEL_FLAG_FIRST)
		{
			if (client_data_in == NULL) {
				client_data_in = Stream_New(NULL, pev->data_len);
			}
			Stream_SetPosition(client_data_in, 0);
		}

		wStream* data_in = client_data_in;
		if (!Stream_EnsureRemainingCapacity(data_in, pev->data_len))
		{
			return true;
		}

		Stream_Write(data_in, pev->data, pev->data_len);
		if (!(pev->flags & CHANNEL_FLAG_LAST)) {
			return true;
		}
		Stream_SetPosition(data_in, 0);
		auto s = (wStream*)data_in;

		UINT16 msgType;
		UINT16 msgFlags;
		UINT32 dataLen;
		UINT error;

		if (Stream_GetRemainingLength(s) < 8)
			return true;

		Stream_Read_UINT16(s, msgType);
		Stream_Read_UINT16(s, msgFlags);
		Stream_Read_UINT32(s, dataLen);

		if (Stream_GetRemainingLength(s) < dataLen)
			return true;

		if (msgType == CB_FORMAT_DATA_REQUEST) {

			if (Stream_GetRemainingLength(s) >= 4) {
				UINT32 formatID;
				Stream_Read_UINT32(s, formatID);
				clientformatID = formatID;
				std::cout << "C++ demo plugin: CB_FORMAT_DATA_REQUEST..." << clientformatID << std::endl;
			}
		}
		else if (msgType == CB_FORMAT_DATA_RESPONSE) {
			if (serverformatID == CF_UNICODETEXT ) {
				std::wcout << s->pointer <<std::endl;
			}
			else if (serverformatID == CB_FORMAT_TEXTURILIST ) {
				FILEDESCRIPTORW* file_descriptor_array;
				UINT32 file_descriptor_count;
				auto result = cliprdr_parse_file_list(s->pointer, dataLen, &file_descriptor_array, &file_descriptor_count);

				if (result == 0 && file_descriptor_count > 0) {
					LPSTR lpFileNameA;
					if (ConvertFromUnicode(CP_UTF8, 0, file_descriptor_array->cFileName, -1, &lpFileNameA, 0, NULL, NULL) < 1)
						return NULL;
					std::cout << "cliboard_filter_client_Event C++ demo plugin: CB_FORMAT_TEXTURILIST..." << lpFileNameA<< std::endl;

				}
				std::cout << "cliboard_filter_client_Event C++ demo plugin: CB_FORMAT_TEXTURILIST..." << result<< std::endl;
			}
			std::cout << "C++ demo plugin: CB_FORMAT_DATA_RESPONSE..." << std::endl;
		}

		data_in = NULL;
	}

	// Always return TRUE to allow data to pass through the filter
	return TRUE;
}



static BOOL demo_server_channels_init(proxyData* pdata)
{
	WLog_INFO(TAG, "called");
	return TRUE;
}
BOOL proxy_module_entry_point(proxyPluginsManager* plugins_manager)
{
	g_plugins_manager = plugins_manager;

	demo_plugin.MouseEvent = demo_filter_mouse_event;
	demo_plugin.KeyboardEvent = demo_filter_keyboard_event;
	demo_plugin.ServerChannelsInit = demo_server_channels_init;
	demo_plugin.ClientChannelData = cliboard_filter_client_Event;
	demo_plugin.ServerChannelData = cliboard_filter_server_Event;
	return plugins_manager->RegisterPlugin(&demo_plugin);
}
