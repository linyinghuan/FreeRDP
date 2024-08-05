#include "auditor.h"
#include "auditor_channel.h"
#include "auditor_clip.h"

void auditor_clip_event_handler(UINT mode, proxyData* pdata, proxyChannelDataEventInfo* pEvent);
void auditor_clip_client_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent);
void auditor_clip_server_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent);

__attribute__((constructor)) static int auditor_clip_init()
{
	auditor_channel_event_reg(AUDITOR_CLIENT, AUDITOR_EVENT_CLIPB, auditor_clip_client_event_handler);
	auditor_channel_event_reg(AUDITOR_SERVER, AUDITOR_EVENT_CLIPB, auditor_clip_server_event_handler);
}

static void cliprdr_free_format_list(CLIPRDR_FORMAT_LIST* formatList)
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


static UINT cliprdr_read_format_list(wStream* s, CLIPRDR_FORMAT_LIST* formatList, BOOL useLongFormatNames)
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
			return ERROR_INTERNAL_ERROR;
		}

		if (formatList->numFormats)
			formats = (CLIPRDR_FORMAT*)calloc(formatList->numFormats, sizeof(CLIPRDR_FORMAT));

		if (!formats)
		{
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
						printf("failed to convert short clipboard format name\n");
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
			printf("calloc failed!\n");
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
					printf("failed to convert long clipboard format name\n");
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

void auditor_clip_client_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent)
{
	auditor_clip_event_handler(AUDITOR_CLIENT, pData, pEvent);
}

void auditor_clip_server_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent)
{
	auditor_clip_event_handler(AUDITOR_SERVER, pData, pEvent);
}

void auditor_clip_event_handler(UINT mode, proxyData* pData, proxyChannelDataEventInfo* pEvent)
{
	wStream* s;
	UINT16 msgType;
	UINT16 msgFlags;
	UINT32 dataLen;
	static UINT32 formatID;
	static CLIPRDR_FORMAT_LIST formatList;
	UINT error;

	s = Stream_New(NULL, pEvent->data_len);
	Stream_SetPosition(s, 0);

	Stream_Write(s, pEvent->data, pEvent->data_len);
	if (!(pEvent->flags & CHANNEL_FLAG_LAST)) {
		return;
	}
	Stream_SetPosition(s, 0);
	if (Stream_GetRemainingLength(s) < 8)
		return;
	Stream_Read_UINT16(s, msgType);
	Stream_Read_UINT16(s, msgFlags);
	Stream_Read_UINT32(s, dataLen);

	if (Stream_GetRemainingLength(s) < dataLen)
		return;

	printf("cliboard_filter_server_Event Type: %d - Flags:%#x - len:%d\n", msgType, msgFlags, dataLen);
	if (msgType == CB_FORMAT_LIST) {
		error = CHANNEL_RC_OK;

		formatList.msgType = CB_FORMAT_LIST;
		formatList.msgFlags = msgFlags;
		formatList.dataLen = dataLen;
		if ((error = cliprdr_read_format_list(s, &formatList, pData->ps->cliprdr->useLongFormatNames/*cliprdr->useLongFormatNames*/)) == CHANNEL_RC_OK) {
			for (int i = 0; i< formatList.numFormats; i++) {
				printf("-----format id:%#x\t %s\n", formatList.formats[i].formatId, formatList.formats[i].formatName);
			}

		}
	}
	else if (msgType == CB_FORMAT_DATA_REQUEST) {
		if (Stream_GetRemainingLength(s) >= 4) {
			Stream_Read_UINT32(s, formatID);
			printf("format id:%#x\n", formatID);
		}
	}
	else if (msgType == CB_FORMAT_DATA_RESPONSE) {
		int iFoundIndex = -1;

		for (int i = 0; i< formatList.numFormats; i++) {
			if (formatID == formatList.formats[i].formatId) {
				iFoundIndex = i;
				printf("-----found format id:%#x\t %s\n", formatList.formats[i].formatId, formatList.formats[i].formatName);
				break;
			}}

		if (formatID == CF_UNICODETEXT ) {
			LPSTR lpCopyA;
			if (ConvertFromUnicode(CP_UTF8, 0, (LPCWSTR)s->pointer, -1, &lpCopyA, 0, NULL, NULL) < 1)
				return NULL;
			printf("cliboard_filter_server_Event C++ demo plugin: CF_UNICODETEXT:%s\n", lpCopyA);
			tlog(TLOG_INFO, pData->session_id, 0, "[clipboard] copy UNICODETEXT: %s\n", lpCopyA);

		}
		else if (formatID == CB_FORMAT_TEXTURILIST  || 
			( iFoundIndex != -1 && 0 == strncmp(formatList.formats[iFoundIndex].formatName, "FileGroupDescriptorW", strlen("FileGroupDescriptorW") ))/*clientformatID == 0xc07d*/) {
			FILEDESCRIPTORW* file_descriptor_array;
			UINT32 file_descriptor_count;
			UINT result = cliprdr_parse_file_list(s->pointer, dataLen, &file_descriptor_array, &file_descriptor_count);

			if (result == 0 && file_descriptor_count > 0) {
				LPSTR lpFileNameA;
				printf("cliboard_filter_server_Event C++ demo plugin: CB_FORMAT_TEXTURILIST:\n");

				for (int i = 0; i< file_descriptor_count; i++) {
					if (ConvertFromUnicode(CP_UTF8, 0, (file_descriptor_array+i)->cFileName, -1, &lpFileNameA, 0, NULL, NULL) < 1)
						return NULL;
					printf("%s\n", lpFileNameA);
					tlog(TLOG_INFO, pData->session_id, 0, "[clipboard] copy FILE: %s n", lpFileNameA);
				}
			}
		}
	}
	Stream_Free(s, TRUE);
}