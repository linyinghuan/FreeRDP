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



void cliprdr_free_format_list(CLIPRDR_FORMAT_LIST* formatList)
{
	UINT index = 0;

	if (formatList == NULL)
		return;

	if (formatList->formats)
	{
		for (index = 0; index < formatList->numFormats; index++)
		{
			free(formatList->formats[index].formatName);
		}

		free(formatList->formats);
		formatList->formats = NULL;
		formatList->numFormats = 0;
	}
}


UINT cliprdr_read_format_list(wStream* s, CLIPRDR_FORMAT_LIST* formatList, BOOL useLongFormatNames)
{
	UINT32 index;
	BOOL asciiNames;
	int formatNameLength;
	char* szFormatName;
	WCHAR* wszFormatName;
	wStream sub1, sub2;
	CLIPRDR_FORMAT* formats = NULL;
	UINT error = ERROR_INTERNAL_ERROR;

	asciiNames = (formatList->msgFlags & CB_ASCII_NAMES) ? TRUE : FALSE;

	index = 0;
	/* empty format list */
	formatList->formats = NULL;
	formatList->numFormats = 0;

	Stream_StaticInit(&sub1, Stream_Pointer(s), formatList->dataLen);
	if (!Stream_SafeSeek(s, formatList->dataLen))
		return ERROR_INVALID_DATA;

	if (!formatList->dataLen)
	{
	}
	else if (!useLongFormatNames)
	{
		const size_t cap = Stream_Capacity(&sub1);
		formatList->numFormats = (cap / 36);

		if ((formatList->numFormats * 36) != cap)
		{
			WLog_ERR(TAG, "Invalid short format list length: %" PRIuz "", cap);
			return ERROR_INTERNAL_ERROR;
		}

		if (formatList->numFormats)
			formats = (CLIPRDR_FORMAT*)calloc(formatList->numFormats, sizeof(CLIPRDR_FORMAT));

		if (!formats)
		{
			WLog_ERR(TAG, "calloc failed!");
			return CHANNEL_RC_NO_MEMORY;
		}

		formatList->formats = formats;

		while (Stream_GetRemainingLength(&sub1) >= 4)
		{
			Stream_Read_UINT32(&sub1, formats[index].formatId); /* formatId (4 bytes) */

			formats[index].formatName = NULL;

			/* According to MS-RDPECLIP 2.2.3.1.1.1 formatName is "a 32-byte block containing
			 * the *null-terminated* name assigned to the Clipboard Format: (32 ASCII 8 characters
			 * or 16 Unicode characters)"
			 * However, both Windows RDSH and mstsc violate this specs as seen in the following
			 * example of a transferred short format name string: [R.i.c.h. .T.e.x.t. .F.o.r.m.a.t.]
			 * These are 16 unicode charaters - *without* terminating null !
			 */

			szFormatName = (char*)Stream_Pointer(&sub1);
			wszFormatName = (WCHAR*)Stream_Pointer(&sub1);
			if (!Stream_SafeSeek(&sub1, 32))
				goto error_out;
			if (asciiNames)
			{
				if (szFormatName[0])
				{
					/* ensure null termination */
					formats[index].formatName = (char*)malloc(32 + 1);
					if (!formats[index].formatName)
					{
						WLog_ERR(TAG, "malloc failed!");
						error = CHANNEL_RC_NO_MEMORY;
						goto error_out;
					}
					CopyMemory(formats[index].formatName, szFormatName, 32);
					formats[index].formatName[32] = '\0';
				}
			}
			else
			{
				if (wszFormatName[0])
				{
					/* ConvertFromUnicode always returns a null-terminated
					 * string on success, even if the source string isn't.
					 */
					if (ConvertFromUnicode(CP_UTF8, 0, wszFormatName, 16,
					                       &(formats[index].formatName), 0, NULL, NULL) < 1)
					{
						WLog_ERR(TAG, "failed to convert short clipboard format name");
						error = ERROR_INTERNAL_ERROR;
						goto error_out;
					}
				}
			}

			index++;
		}
	}
	else
	{
		sub2 = sub1;
		while (Stream_GetRemainingLength(&sub1) > 0)
		{
			size_t rest;
			if (!Stream_SafeSeek(&sub1, 4)) /* formatId (4 bytes) */
				goto error_out;

			wszFormatName = (WCHAR*)Stream_Pointer(&sub1);
			rest = Stream_GetRemainingLength(&sub1);
			formatNameLength = _wcsnlen(wszFormatName, rest / sizeof(WCHAR));

			if (!Stream_SafeSeek(&sub1, (formatNameLength + 1) * sizeof(WCHAR)))
				goto error_out;
			formatList->numFormats++;
		}

		if (formatList->numFormats)
			formats = (CLIPRDR_FORMAT*)calloc(formatList->numFormats, sizeof(CLIPRDR_FORMAT));

		if (!formats)
		{
			WLog_ERR(TAG, "calloc failed!");
			return CHANNEL_RC_NO_MEMORY;
		}

		formatList->formats = formats;

		while (Stream_GetRemainingLength(&sub2) >= 4)
		{
			size_t rest;
			Stream_Read_UINT32(&sub2, formats[index].formatId); /* formatId (4 bytes) */

			formats[index].formatName = NULL;

			wszFormatName = (WCHAR*)Stream_Pointer(&sub2);
			rest = Stream_GetRemainingLength(&sub2);
			formatNameLength = _wcsnlen(wszFormatName, rest / sizeof(WCHAR));
			if (!Stream_SafeSeek(&sub2, (formatNameLength + 1) * sizeof(WCHAR)))
				goto error_out;

			if (formatNameLength)
			{
				if (ConvertFromUnicode(CP_UTF8, 0, wszFormatName, formatNameLength,
				                       &(formats[index].formatName), 0, NULL, NULL) < 1)
				{
					WLog_ERR(TAG, "failed to convert long clipboard format name");
					error = ERROR_INTERNAL_ERROR;
					goto error_out;
				}
			}

			index++;
		}
	}

	return CHANNEL_RC_OK;

error_out:
	cliprdr_free_format_list(formatList);
	return error;
}



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



CLIPRDR_FORMAT_LIST formatListServer = { 0 };
static BOOL cliboard_filter_server_Event(proxyData* data, void* context) {
	auto pev = static_cast<proxyChannelDataEventInfo*>(context);
	UINT64 server_channel_id;

	if ( 0 == strncmp(pev->channel_name, "cliprdr", strlen("cliprdr") )) {

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

		printf("cliboard_filter_server_Event Type: %d - Flags:%#x - len:%d\n", msgType, msgFlags, dataLen);
		if (msgType == CB_FORMAT_LIST) {
			UINT error = CHANNEL_RC_OK;

			formatListServer.msgType = CB_FORMAT_LIST;
			formatListServer.msgFlags = msgFlags;
			formatListServer.dataLen = dataLen;

			if ((error = cliprdr_read_format_list(s, &formatListServer, data->ps->cliprdr->useLongFormatNames/*cliprdr->useLongFormatNames*/)) == CHANNEL_RC_OK) {

				std::cout<<"format list number" << formatListServer.numFormats <<std::endl;
				for (int i = 0; i< formatListServer.numFormats; i++) {

					printf("-----format id:%#x\t %s\n", formatListServer.formats[i].formatId, formatListServer.formats[i].formatName);
				}

			}
		}
		else if (msgType == CB_FORMAT_DATA_REQUEST) {

			if (Stream_GetRemainingLength(s) >= 4) {
				UINT32 formatID;
				Stream_Read_UINT32(s, formatID);
				serverformatID = formatID;
				std::cout << "C++ demo plugn: CB_FORMAT_DATA_REQUEST..." << serverformatID << std::endl;

				printf("format id:%#x\n", formatID);
			}
		}
		else if (msgType == CB_FORMAT_DATA_RESPONSE) {

			int iFoundIndex = -1;
			for (int i = 0; i< formatListServer.numFormats; i++) {
				if (clientformatID == formatListServer.formats[i].formatId) {
					iFoundIndex = i;
					printf("-----found format id:%#x\t %s\n", formatListServer.formats[i].formatId, formatListServer.formats[i].formatName);
					break;
				}}

			if (clientformatID == CF_UNICODETEXT ) {
				std::wcout << s->pointer <<std::endl;


				LPSTR lpCopyA;
				if (ConvertFromUnicode(CP_UTF8, 0, (LPCWSTR)s->pointer, -1, &lpCopyA, 0, NULL, NULL) < 1)
					return NULL;
				std::cout << "cliboard_filter_server_Event C++ demo plugin: CF_UNICODETEXT:" << lpCopyA<< std::endl;

			}
			else if (clientformatID == CB_FORMAT_TEXTURILIST  || ( iFoundIndex != -1 && 0 == strncmp(formatListServer.formats[iFoundIndex].formatName, "FileGroupDescriptorW", strlen("FileGroupDescriptorW") ))/*clientformatID == 0xc07d*/) {
				FILEDESCRIPTORW* file_descriptor_array;
				UINT32 file_descriptor_count;
				auto result = cliprdr_parse_file_list(s->pointer, dataLen, &file_descriptor_array, &file_descriptor_count);

				std::cout << "cliboard_filter_server_Event C++ demo plugin: CB_FORMAT_TEXTURILIST..." << result<< std::endl;
				if (result == 0 && file_descriptor_count > 0) {
					LPSTR lpFileNameA;
					std::cout << "cliboard_filter_server_Event C++ demo plugin: CB_FORMAT_TEXTURILIST:" << std::endl;

					for (int i = 0; i< file_descriptor_count; i++) {
						if (ConvertFromUnicode(CP_UTF8, 0, (file_descriptor_array+i)->cFileName, -1, &lpFileNameA, 0, NULL, NULL) < 1)
							return NULL;
						std::cout << "" << lpFileNameA<< std::endl;
					}

					std::cout << "end"<< std::endl;

				}
			}
			std::cout << "C++ demo plugin: CB_FORMAT_DATA_RESPONSE..." << std::endl;
		}

		data_in = NULL;
	}

	// Always return TRUE to allow data to pass through the filter
	return TRUE;
}


CLIPRDR_FORMAT_LIST formatListClient = { 0 };
static BOOL cliboard_filter_client_Event(proxyData* data, void* context) {
	auto pev = static_cast<proxyChannelDataEventInfo*>(context);
	UINT64 server_channel_id;

	//if (pev->channel_id == 1006)
	if ( 0 == strncmp(pev->channel_name, "cliprdr", strlen("cliprdr") )) {

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

		printf("cliboard_filter_client_Event Type: %d - Flags:%#x - len:%d\n", msgType, msgFlags, dataLen);
		if (Stream_GetRemainingLength(s) < dataLen)
			return true;

		if (msgType == CB_FORMAT_LIST) {
			UINT error = CHANNEL_RC_OK;

			formatListClient.msgType = CB_FORMAT_LIST;
			formatListClient.msgFlags = msgFlags;
			formatListClient.dataLen = dataLen;

			if ((error = cliprdr_read_format_list(s, &formatListClient, data->ps->cliprdr->useLongFormatNames/*cliprdr->useLongFormatNames*/)) == CHANNEL_RC_OK) {

				std::cout<<"format list number" << formatListClient.numFormats <<std::endl;
				for (int i = 0; i< formatListClient.numFormats; i++) {
					printf("-----format id:%#x\t %s\n", formatListClient.formats[i].formatId, formatListClient.formats[i].formatName);

				}

			}

		}
		else if (msgType == CB_FORMAT_DATA_REQUEST) {

			if (Stream_GetRemainingLength(s) >= 4) {
				UINT32 formatID;
				Stream_Read_UINT32(s, formatID);
				clientformatID = formatID;
				std::cout << "C++ demo plugin: CB_FORMAT_DATA_REQUEST..." << clientformatID << std::endl;

				printf("format id:%#x\n", formatID);
			}
		}
		else if (msgType == CB_FORMAT_DATA_RESPONSE) {

			int iFoundIndex = -1;
			for (int i = 0; i< formatListClient.numFormats; i++) {
				if (serverformatID == formatListClient.formats[i].formatId) {
					iFoundIndex = i;
					printf("-----found format id:%#x\t %s\n", formatListClient.formats[i].formatId, formatListClient.formats[i].formatName);
					break;
				}
			}


			if (serverformatID == CF_UNICODETEXT ) {
				std::wcout << s->pointer <<std::endl;

				LPSTR lpCopyA;
				if (ConvertFromUnicode(CP_UTF8, 0, (LPCWSTR)(s->pointer), -1, &lpCopyA, 0, NULL, NULL) < 1)
					return NULL;
				std::cout << "cliboard_filter_client_Event C++ demo plugin: CF_UNICODETEXT:" << lpCopyA<< std::endl;

			}
			else if (serverformatID == CB_FORMAT_TEXTURILIST || ( iFoundIndex != -1 && 0 == strncmp(formatListClient.formats[iFoundIndex].formatName, "FileGroupDescriptorW", strlen("FileGroupDescriptorW"))) ){
				FILEDESCRIPTORW* file_descriptor_array;
				UINT32 file_descriptor_count;
				auto result = cliprdr_parse_file_list(s->pointer, dataLen, &file_descriptor_array, &file_descriptor_count);

				std::cout << "cliboard_filter_client_Event C++ demo plugin: CB_FORMAT_TEXTURILIST..." << result<< std::endl;
				if (result == 0 && file_descriptor_count > 0) {
					LPSTR lpFileNameA;
					std::cout << "cliboard_filter_client_Event C++ demo plugin: CB_FORMAT_TEXTURILIST:" << std::endl;

					for (int i = 0; i< file_descriptor_count; i++) {
						if (ConvertFromUnicode(CP_UTF8, 0, (file_descriptor_array+i)->cFileName, -1, &lpFileNameA, 0, NULL, NULL) < 1)
							return NULL;
						std::cout << "" << lpFileNameA<< std::endl;
					}

					std::cout << "end"<< std::endl;

				}
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

	//demo_plugin.MouseEvent = demo_filter_mouse_event;
	//demo_plugin.KeyboardEvent = demo_filter_keyboard_event;
	demo_plugin.ServerChannelsInit = demo_server_channels_init;
	demo_plugin.ClientChannelData = cliboard_filter_client_Event;
	demo_plugin.ServerChannelData = cliboard_filter_server_Event;
	return plugins_manager->RegisterPlugin(&demo_plugin);
}
