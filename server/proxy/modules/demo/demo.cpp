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

enum RDPDR_PAKID_CG
{
	PAKID_CORE_SERVER_ANNOUNCE = 0x496E,
	PAKID_CORE_CLIENTID_CONFIRM = 0x4343,
	PAKID_CORE_CLIENT_NAME = 0x434E,
	PAKID_CORE_DEVICELIST_ANNOUNCE = 0x4441,
	PAKID_CORE_DEVICE_REPLY = 0x6472,
	PAKID_CORE_DEVICE_IOREQUEST = 0x4952,
	PAKID_CORE_DEVICE_IOCOMPLETION = 0x4943,
	PAKID_CORE_SERVER_CAPABILITY = 0x5350,
	PAKID_CORE_CLIENT_CAPABILITY = 0x4350,
	PAKID_CORE_DEVICELIST_REMOVE = 0x444D,
	PAKID_CORE_USER_LOGGEDON = 0x554C,
	PAKID_PRN_CACHE_DATA = 0x5043,
	PAKID_PRN_USING_XPS = 0x5543
};

/* DR_DEVICE_IOREQUEST.MajorFunction */
enum IRP_MJ_CG
{
	IRP_MJ_CREATE = 0x00000000,
	IRP_MJ_CLOSE = 0x00000002,
	IRP_MJ_READ = 0x00000003,
	IRP_MJ_WRITE = 0x00000004,
	IRP_MJ_DEVICE_CONTROL = 0x0000000E,
	IRP_MJ_QUERY_VOLUME_INFORMATION = 0x0000000A,
	IRP_MJ_SET_VOLUME_INFORMATION = 0x0000000B,
	IRP_MJ_QUERY_INFORMATION = 0x00000005,
	IRP_MJ_SET_INFORMATION = 0x00000006,
	IRP_MJ_DIRECTORY_CONTROL = 0x0000000C,
	IRP_MJ_LOCK_CONTROL = 0x00000011
};

/* DR_DEVICE_IOREQUEST.MinorFunction */
enum IRP_MN_CG
{
	IRP_MN_QUERY_DIRECTORY = 0x00000001,
	IRP_MN_NOTIFY_CHANGE_DIRECTORY = 0x00000002
};
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
UINT32 g_FsInformationClass;
bool g_need = false;
static BOOL cliboard_filter_server_Event(proxyData* data, void* context) {
	auto pev = static_cast<proxyChannelDataEventInfo*>(context);


	//if (!pev->valid)
	//	return true;

	UINT64 server_channel_id;

	if ( 0 == strncmp(pev->channel_name, "rdpdr", strlen("rdpdr") )) {
		printf("---------------------cliboard_filter_server_Event Type: %s -----------------\n", pev->channel_name);


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


		UINT16 msgRDPDRCTYP;
		UINT16 msgRDPDRPAKID;
		UINT32 DeviceId;
		UINT32 CompletionId;
		UINT32 IoStatus;
		UINT error;

		if (Stream_GetRemainingLength(s) < 8)
			return true;

		if (!g_need)
			return true;

		Stream_Read_UINT16(s, msgRDPDRCTYP); // Component (2 bytes)
		Stream_Read_UINT16(s, msgRDPDRPAKID); // PacketId (2 bytes)
		Stream_Read_UINT32(s, DeviceId);       // DeviceId (4 bytes)
		Stream_Read_UINT32(s, CompletionId);              // CompletionId (4 bytes)
		Stream_Read_UINT32(s, IoStatus);                              // IoStatus (4 bytes)

		UINT32 totalLength;
		UINT32 Length;
		UINT32 NextEntryOffset;;
		UINT32 FileIndex;
		UINT32 CreationTime;
		UINT32 CreationTime2;
		UINT32 LastAccessTime;
		UINT32 LastAccessTime2;
		UINT32 LastWriteTime;
		UINT32 LastWriteTime2;
		UINT32 ChangeTime;
		UINT32 ChangeTime2;
		UINT32 EndOfFile;
		UINT32 EndOfFile2;
		UINT32 AllocationSize;
		UINT32 AllocationSize2;
		UINT32 FileAttributes;
		WCHAR* path;
		UINT32 EaSize;
		UINT32 ShortNameLength;
				LPSTR lpFileNameA;

		g_need = false;
		switch (g_FsInformationClass)
		{
			case FileDirectoryInformation:


				Stream_Read_UINT32(s, totalLength); /* Length */
				Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
				Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */
				Stream_Read_UINT32(s,
				                    CreationTime); /* CreationTime */
				Stream_Read_UINT32(s,CreationTime2); /* CreationTime */
				Stream_Read_UINT32(
				    s, LastAccessTime); /* LastAccessTime */
				Stream_Read_UINT32(
				    s, LastAccessTime2); /* LastAccessTime */
				Stream_Read_UINT32(s,
				                    LastWriteTime); /* LastWriteTime */
				Stream_Read_UINT32(s,
				                    LastWriteTime2); /* LastWriteTime */
				Stream_Read_UINT32(s,ChangeTime); /* ChangeTime */
				Stream_Read_UINT32(s,ChangeTime2); /* ChangeTime */
				Stream_Read_UINT32(s, EndOfFile);           /* EndOfFile */
				Stream_Read_UINT32(s, EndOfFile2);          /* EndOfFile */
				Stream_Read_UINT32(s, AllocationSize);     /* AllocationSize */
				Stream_Read_UINT32(s, AllocationSize2);    /* AllocationSize */
				Stream_Read_UINT32(s, FileAttributes); /* FileAttributes */
				Stream_Read_UINT32(s, Length);                   /* FileNameLength */


				path = (WCHAR*)Stream_Pointer(s);


				if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
					return NULL;

				printf("=========path:[%s]\n", lpFileNameA);
				break;

			case FileFullDirectoryInformation:
				Stream_Read_UINT32(s, totalLength); /* Length */
				Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
				Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */
				Stream_Read_UINT32(s,
						    CreationTime); /* CreationTime */
				Stream_Read_UINT32(s,CreationTime2); /* CreationTime */
				Stream_Read_UINT32(
				    s, LastAccessTime); /* LastAccessTime */
				Stream_Read_UINT32(
				    s, LastAccessTime2); /* LastAccessTime */
				Stream_Read_UINT32(s,
						    LastWriteTime); /* LastWriteTime */
				Stream_Read_UINT32(s,
						    LastWriteTime2); /* LastWriteTime */
				Stream_Read_UINT32(s,ChangeTime); /* ChangeTime */
				Stream_Read_UINT32(s,ChangeTime2); /* ChangeTime */
				Stream_Read_UINT32(s, EndOfFile);           /* EndOfFile */
				Stream_Read_UINT32(s, EndOfFile2);          /* EndOfFile */
				Stream_Read_UINT32(s, AllocationSize);     /* AllocationSize */
				Stream_Read_UINT32(s, AllocationSize2);    /* AllocationSize */
				Stream_Read_UINT32(s, FileAttributes); /* FileAttributes */
				Stream_Read_UINT32(s, Length);                   /* FileNameLength */

				Stream_Read_UINT32(s, EaSize);                                /* EaSize */

				path = (WCHAR*)Stream_Pointer(s);


				if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
					return NULL;

				printf("=========path:[%s]\n", lpFileNameA);
				break;

			case FileBothDirectoryInformation:


				Stream_Read_UINT32(s, totalLength); /* Length */
				Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
				Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */
				Stream_Read_UINT32(s,
						    CreationTime); /* CreationTime */
				Stream_Read_UINT32(s,CreationTime2); /* CreationTime */
				Stream_Read_UINT32(
				    s, LastAccessTime); /* LastAccessTime */
				Stream_Read_UINT32(
				    s, LastAccessTime2); /* LastAccessTime */
				Stream_Read_UINT32(s,
						    LastWriteTime); /* LastWriteTime */
				Stream_Read_UINT32(s,
						    LastWriteTime2); /* LastWriteTime */
				Stream_Read_UINT32(s,ChangeTime); /* ChangeTime */
				Stream_Read_UINT32(s,ChangeTime2); /* ChangeTime */
				Stream_Read_UINT32(s, EndOfFile);           /* EndOfFile */
				Stream_Read_UINT32(s, EndOfFile2);          /* EndOfFile */
				Stream_Read_UINT32(s, AllocationSize);     /* AllocationSize */
				Stream_Read_UINT32(s, AllocationSize2);    /* AllocationSize */
				Stream_Read_UINT32(s, FileAttributes); /* FileAttributes */
				Stream_Read_UINT32(s, Length);                   /* FileNameLength */

				Stream_Read_UINT32(s, EaSize);                                /* EaSize */

				Stream_Read_UINT32(s, ShortNameLength);
				Stream_Seek(s, 24);
				//Stream_Zero(output, 24); /* ShortName */
				path = (WCHAR*)Stream_Pointer(s);


				if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
					return NULL;

				printf("=========path:[%s]\n", lpFileNameA);
				break;


			case FileNamesInformation:

				Stream_Read_UINT32(s, totalLength); /* Length */
				Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
				Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */

				Stream_Read_UINT32(s, Length);                   /* FileNameLength */
				path = (WCHAR*)Stream_Pointer(s);


				if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
					return NULL;

				printf("=========path:[%s]\n", lpFileNameA);
				break;

			default:
				break;
		}
	}
	else if ( 0 == strncmp(pev->channel_name, "cliprdr", strlen("cliprdr") )) {

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
	//if (!pev->valid)
	//	return true;
	UINT64 server_channel_id;

	//if (pev->channel_id == 1006)

	if ( 0 == strncmp(pev->channel_name, "rdpdr", strlen("rdpdr") )) {
		printf("cliboard_filter_client_Event Type: %s -----------------\n", pev->channel_name);


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


		if (Stream_GetRemainingLength(s) < 4)
			return true;

		UINT16 component;
		UINT16 packetId;
		UINT32 deviceId;
		UINT32 status;
		UINT error = ERROR_INVALID_DATA;

		Stream_Read_UINT16(s, component); /* Component (2 bytes) */
		Stream_Read_UINT16(s, packetId);  /* PacketId (2 bytes) */
		if (component == 0x4472 /*RDPDR_CTYP_CORE*/) {

			switch (packetId)
			{
				case PAKID_CORE_SERVER_ANNOUNCE:

				{
					printf("cliboard_filter_client_Event  PAKID_CORE_SERVER_ANNOUNCE\n" );
				}

					break;

				case PAKID_CORE_SERVER_CAPABILITY:

				{
					printf("cliboard_filter_client_Event  PAKID_CORE_SERVER_CAPABILITY\n" );
				}
					break;

				case PAKID_CORE_CLIENTID_CONFIRM:

				{
					printf("cliboard_filter_client_Event  PAKID_CORE_CLIENTID_CONFIRM\n" );
				}
					break;

				case PAKID_CORE_USER_LOGGEDON:

				{
					printf("cliboard_filter_client_Event  PAKID_CORE_USER_LOGGEDON\n" );
				}

					break;

				case PAKID_CORE_DEVICE_REPLY:


				{
					printf("cliboard_filter_client_Event  PAKID_CORE_DEVICE_REPLY\n" );
				}
					break;

				case PAKID_CORE_DEVICE_IOREQUEST:
				{
					printf("cliboard_filter_client_Event  PAKID_CORE_DEVICE_IOREQUEST\n" );
					UINT32 DeviceId;
					UINT32 FileId;
					UINT32 CompletionId;
					UINT32 MajorFunction;
					UINT32 MinorFunction;


					if (Stream_GetRemainingLength(s) < 20)
						return true;

					Stream_Read_UINT32(s, DeviceId); /* DeviceId (4 bytes) */

					Stream_Read_UINT32(s, FileId);        /* FileId (4 bytes) */
					Stream_Read_UINT32(s, CompletionId);  /* CompletionId (4 bytes) */
					Stream_Read_UINT32(s, MajorFunction); /* MajorFunction (4 bytes) */
					Stream_Read_UINT32(s, MinorFunction); /* MinorFunction (4 bytes) */
					printf("cliboard_filter_client_Event MajorFunction: 0x%x -----------------\n", MajorFunction);

					if (MajorFunction == IRP_MJ_CREATE) {
						UINT32 FileId;
						//DRIVE_FILE* file;
						BYTE Information;
						UINT32 FileAttributes;
						UINT32 SharedAccess;
						UINT32 DesiredAccess;
						UINT32 CreateDisposition;
						UINT32 CreateOptions;
						UINT32 PathLength;
						UINT64 allocationSize;
						const WCHAR* path;

						if (Stream_GetRemainingLength(s) < 6 * 4 + 8)
							return true;

						Stream_Read_UINT32(s, DesiredAccess);
						Stream_Read_UINT64(s, allocationSize);
						Stream_Read_UINT32(s, FileAttributes);
						Stream_Read_UINT32(s, SharedAccess);
						Stream_Read_UINT32(s, CreateDisposition);
						Stream_Read_UINT32(s, CreateOptions);
						Stream_Read_UINT32(s, PathLength);

						if (Stream_GetRemainingLength(s) < PathLength)
							return ERROR_INVALID_DATA;

						path = (const WCHAR*)Stream_Pointer(s);

						LPSTR lpFileNameA;
						if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
								return NULL;

						printf("=========path:[%s]\n", lpFileNameA);
					}
					else if (MajorFunction == IRP_MJ_DIRECTORY_CONTROL) {
						printf("cliboard_filter_client_Event  IRP_MJ_DIRECTORY_CONTROL\n" );

						switch (MinorFunction)
						{
						case IRP_MN_QUERY_DIRECTORY:
						{
							g_need = true;
							const WCHAR* path;
							//DRIVE_FILE* file;
							BYTE InitialQuery;
							UINT32 PathLength;
							UINT32 FsInformationClass;

							if (Stream_GetRemainingLength(s) < 32)
								return ERROR_INVALID_DATA;

							Stream_Read_UINT32(s, FsInformationClass);
							Stream_Read_UINT8(s, InitialQuery);
							Stream_Read_UINT32(s, PathLength);
							Stream_Seek(s, 23); /* Padding */
							path = (WCHAR*)Stream_Pointer(s);

							g_FsInformationClass = FsInformationClass;

							LPSTR lpFileNameA;
							if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
								return NULL;

							printf("=========path:[%s]\n", lpFileNameA);
						}
							break;

						case IRP_MN_NOTIFY_CHANGE_DIRECTORY: /* TODO */
							//return irp->Discard(irp);
							break;
						default:
							break;
						}
					}
				}
					break;

				default:
				{
					printf("cliboard_filter_client_Event  default invalid\n" );
				}
					break;
			}



		}

	}
	else if ( 0 == strncmp(pev->channel_name, "cliprdr", strlen("cliprdr") )) {

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
