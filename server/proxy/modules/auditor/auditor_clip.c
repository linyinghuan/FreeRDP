#include "auditor.h"
#include "auditor_channel.h"
#include "auditor_clip.h"

void auditor_clip_event_handler(UINT mode, proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);
void auditor_clip_client_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);
void auditor_clip_server_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);

__attribute__((constructor)) static int auditor_clip_init()
{
	auditor_channel_event_reg(AUDITOR_CLIENT, AUDITOR_EVENT_CLIPB, auditor_clip_client_event_handler);
	auditor_channel_event_reg(AUDITOR_SERVER, AUDITOR_EVENT_CLIPB, auditor_clip_server_event_handler);

	return 0;
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

static UINT auditor_parse_file_list(const BYTE* format_data, UINT32 format_data_length,
                             FILEDESCRIPTORW** file_descriptor_array, UINT32* file_descriptor_count)
{
	UINT result = NO_ERROR;
	UINT32 i;
	UINT32 count = 0;
	wStream sbuffer;
	wStream* s = &sbuffer;

	if (!format_data || !file_descriptor_array || !file_descriptor_count)
		return ERROR_BAD_ARGUMENTS;

	Stream_StaticInit(&sbuffer, format_data, format_data_length);
	if (!s)
		return ERROR_NOT_ENOUGH_MEMORY;

	if (Stream_GetRemainingLength(s) < 4)
	{
		WLog_ERR(TAG, "invalid packed file list");

		result = ERROR_INCORRECT_SIZE;
		goto out;
	}

	Stream_Read_UINT32(s, count); /* cItems (4 bytes) */

	if (Stream_GetRemainingLength(s) / CLIPRDR_FILEDESCRIPTOR_SIZE < count)
	{
		WLog_ERR(TAG, "packed file list is too short: expected %" PRIuz ", have %" PRIuz,
		         ((size_t)count) * CLIPRDR_FILEDESCRIPTOR_SIZE, Stream_GetRemainingLength(s));

		result = ERROR_INCORRECT_SIZE;
		goto out;
	}

	*file_descriptor_count = count;
	*file_descriptor_array = calloc(count, sizeof(FILEDESCRIPTORW));
	if (!*file_descriptor_array)
	{
		result = ERROR_NOT_ENOUGH_MEMORY;
		goto out;
	}

	for (i = 0; i < count; i++)
	{
		UINT64 tmp;
		FILEDESCRIPTORW* file = &((*file_descriptor_array)[i]);

		Stream_Read_UINT32(s, file->dwFlags);          /* flags (4 bytes) */
		Stream_Read_UINT32(s, file->clsid.Data1);
		Stream_Read_UINT16(s, file->clsid.Data2);
		Stream_Read_UINT16(s, file->clsid.Data3);
		Stream_Read(s, &file->clsid.Data4, sizeof(file->clsid.Data4));
		Stream_Read_INT32(s, file->sizel.cx);
		Stream_Read_INT32(s, file->sizel.cy);
		Stream_Read_INT32(s, file->pointl.x);
		Stream_Read_INT32(s, file->pointl.y);
		Stream_Read_UINT32(s, file->dwFileAttributes); /* fileAttributes (4 bytes) */
		Stream_Read_UINT64(s, tmp);                    /* ftCreationTime (8 bytes) */
		file->ftCreationTime = uint64_to_filetime(tmp);
		Stream_Read_UINT64(s, tmp); /* ftLastAccessTime (8 bytes) */
		file->ftLastAccessTime = uint64_to_filetime(tmp);
		Stream_Read_UINT64(s, tmp); /* lastWriteTime (8 bytes) */
		file->ftLastWriteTime = uint64_to_filetime(tmp);
		Stream_Read_UINT32(s, file->nFileSizeHigh); /* fileSizeHigh (4 bytes) */
		Stream_Read_UINT32(s, file->nFileSizeLow);  /* fileSizeLow (4 bytes) */
		Stream_Read_UTF16_String(s, file->cFileName,
		                         ARRAYSIZE(file->cFileName)); /* cFileName (520 bytes) */
	}

out:

	return result;
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

void auditor_clip_client_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	auditor_clip_event_handler(AUDITOR_CLIENT, pData, pEvent, auditor_ctx);
}

void auditor_clip_server_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	auditor_clip_event_handler(AUDITOR_SERVER, pData, pEvent, auditor_ctx);
}

void auditor_clip_event_handler(UINT mode, proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	wStream* s = NULL;
	UINT16 msgType;
	UINT16 msgFlags;
	UINT32 dataLen;
	UINT error;


	if (pEvent->flags & CHANNEL_FLAG_FIRST)
	{
		if (auditor_ctx->clip_stream != NULL) {
			Stream_Free(auditor_ctx->clip_stream, TRUE);
		}
		auditor_ctx->clip_stream = Stream_New(NULL, pEvent->data_len);
		Stream_SetPosition(auditor_ctx->clip_stream, 0);
	}
	s = auditor_ctx->clip_stream;

	Stream_Write(s, pEvent->data, pEvent->data_len);
	if (!(pEvent->flags & CHANNEL_FLAG_LAST)) {
		goto finish;
	}
	Stream_SetPosition(s, 0);
	if (Stream_GetRemainingLength(s) < 8)
		goto finish;
	Stream_Read_UINT16(s, msgType);
	Stream_Read_UINT16(s, msgFlags);
	Stream_Read_UINT32(s, dataLen);

	if (Stream_GetRemainingLength(s) < dataLen)
		goto finish;

	printf("cliboard_filter_server_Event Type: %d - Flags:%#x - len:%d\n", msgType, msgFlags, dataLen);
	if (msgType == CB_FORMAT_LIST) {
		error = CHANNEL_RC_OK;

		auditor_ctx->formatList.msgType = CB_FORMAT_LIST;
		auditor_ctx->formatList.msgFlags = msgFlags;
		auditor_ctx->formatList.dataLen = dataLen;
		if ((error = cliprdr_read_format_list(s, &auditor_ctx->formatList, pData->ps->cliprdr->useLongFormatNames/*cliprdr->useLongFormatNames*/)) == CHANNEL_RC_OK) {
			for (int i = 0; i< auditor_ctx->formatList.numFormats; i++) {
				printf("-----format id:%#x\t %s\n", auditor_ctx->formatList.formats[i].formatId, auditor_ctx->formatList.formats[i].formatName);
			}

		}
	}
	else if (msgType == CB_FORMAT_DATA_REQUEST) {
		if (Stream_GetRemainingLength(s) >= 4) {
			Stream_Read_UINT32(s, auditor_ctx->formatID);
			printf("format id:%#x\n", auditor_ctx->formatID);
		}
	}
	else if (msgType == CB_FORMAT_DATA_RESPONSE) {
		int iFoundIndex = -1;

		for (int i = 0; i< auditor_ctx->formatList.numFormats; i++) {
			if (auditor_ctx->formatID == auditor_ctx->formatList.formats[i].formatId) {
				iFoundIndex = i;
				printf("-----found format id:%#x\t %s\n", auditor_ctx->formatList.formats[i].formatId, auditor_ctx->formatList.formats[i].formatName);
				break;
			}}

		if (auditor_ctx->formatID == CF_UNICODETEXT ) {
			LPSTR lpCopyA;
			if (ConvertFromUnicode(CP_UTF8, 0, (LPCWSTR)s->pointer, -1, &lpCopyA, 0, NULL, NULL) < 1)
				goto finish;
			printf("cliboard_filter_server_Event C++ demo plugin: CF_UNICODETEXT:%s\n", lpCopyA);
			tlog(TLOG_INFO, pData->session_id, 0, "[clipboard] copy UNICODETEXT: %s\n", lpCopyA);

		}
		else if (auditor_ctx->formatID == CB_FORMAT_TEXTURILIST  || 
			( iFoundIndex != -1 && 0 == strncmp(auditor_ctx->formatList.formats[iFoundIndex].formatName, "FileGroupDescriptorW", strlen("FileGroupDescriptorW") ))/*clientformatID == 0xc07d*/) {
			FILEDESCRIPTORW* file_descriptor_array;
			UINT32 file_descriptor_count;
			UINT result = auditor_parse_file_list(s->pointer, dataLen, &file_descriptor_array, &file_descriptor_count);

			if (result == 0 && file_descriptor_count > 0) {
				LPSTR lpFileNameA;
				printf("cliboard_filter_server_Event C++ demo plugin: CB_FORMAT_TEXTURILIST:\n");

				for (int i = 0; i< file_descriptor_count; i++) {
					if (ConvertFromUnicode(CP_UTF8, 0, (file_descriptor_array+i)->cFileName, -1, &lpFileNameA, 0, NULL, NULL) < 1)
						goto finish;
					printf("%s\n", lpFileNameA);
					tlog(TLOG_INFO, pData->session_id, 0, "[clipboard] copy FILE: %s n", lpFileNameA);
				}
			}
		}
	}

finish:
	return;
}