#include "auditor.h"
#include "auditor_channel.h"
#include "auditor_clip.h"

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

void auditor_rdpdr_client_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);
void auditor_rdpdr_server_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);

__attribute__((constructor)) static int auditor_rdpdr_init()
{
	auditor_channel_event_reg(AUDITOR_CLIENT, AUDITOR_EVENT_RDPDR, auditor_rdpdr_client_event_handler);
	auditor_channel_event_reg(AUDITOR_SERVER, AUDITOR_EVENT_RDPDR, auditor_rdpdr_server_event_handler);

	return 0;
}

UINT32 auditor_rdpdr_add_path_table(AUDITOR_RDPDR_PATH_TABLE_HEAD* table, char *key, AUDITOR_RDPDR_PATH_LIST_NODE* list)
{
	AUDITOR_RDPDR_PATH_TABLE_NODE* node = NULL;
	AUDITOR_RDPDR_PATH_TABLE_NODE* pTmp = NULL;
	AUDITOR_RDPDR_PATH_TABLE_NODE* pNext = table->node;

	while(pNext) {
		pTmp = pNext;
		if(0 ==strcmp(pNext->path_key, key)) {
			//free old list
			pNext->path_list = list;
			return 0;
		}

		pNext = pNext->next;
	}

	node = malloc(sizeof(AUDITOR_RDPDR_PATH_TABLE_NODE));
	memset(node, 0, sizeof(AUDITOR_RDPDR_PATH_TABLE_NODE));
	node->path_key = key;
	node->path_list = list;

	if(pTmp)
		pTmp->next = node;
	else
		table->node = node;

	return 1;
}

AUDITOR_RDPDR_PATH_LIST_NODE* auditor_rdpdr_find_path_table(AUDITOR_RDPDR_PATH_TABLE_NODE* table, char* key)
{
	AUDITOR_RDPDR_PATH_TABLE_NODE* pNext = table;

	while(pNext) {
		if(0 ==strcmp(pNext->path_key, key))
			return pNext->path_list;

		pNext = pNext->next;
	}

	return NULL;
}

UINT32 auditor_rdpdr_add_path_list(AUDITOR_RDPDR_PATH_LIST_HEAD* list, AUDITOR_RDPDR_PATH *path_info)
{
	AUDITOR_RDPDR_PATH_LIST_NODE* node = NULL;
	AUDITOR_RDPDR_PATH_LIST_NODE* pTmp = NULL;
	AUDITOR_RDPDR_PATH_LIST_NODE* pNext = list->node;

	while(pNext) {
		pTmp = pNext;
		if(0 ==strcmp(pNext->path->m_path, path_info->m_path))
			return 0;

		pNext = pNext->next;
	}

	node = malloc(sizeof(AUDITOR_RDPDR_PATH_LIST_NODE));
	memset(node, 0, sizeof(AUDITOR_RDPDR_PATH_LIST_NODE));
	node->path = path_info;

	if(pTmp)
		pTmp->next = node;
	else
		list->node = node;

	return 1;
}

AUDITOR_RDPDR_PATH *auditor_rdpdr_find_path_list(AUDITOR_RDPDR_PATH_LIST_NODE* list, char *path)
{
	AUDITOR_RDPDR_PATH_LIST_NODE* pNext = list;

	while(pNext) {
		if(0 ==strcmp(pNext->path->m_path, path))
			return pNext->path;

		pNext = pNext->next;
	}

	return NULL;
}

void auditor_rdpdr_update_path_table(proxyData* pData, AUDITOR_RDPDR_PATH_TABLE_HEAD* table, char* key, AUDITOR_RDPDR_PATH_LIST_NODE* list, hash_table *file_map)
{
	AUDITOR_RDPDR_PATH_LIST_NODE* pOldList = NULL;
	AUDITOR_RDPDR_PATH_LIST_NODE* pNext = NULL;
	AUDITOR_RDPDR_PATH_LIST_NODE* pOldNext = NULL;
	jms_auditor_point file_pos = {0};

	pOldList =  auditor_rdpdr_find_path_table(table->node, key);
	if(NULL == pOldList) {
		pNext = list;

		while(pNext) {
			//printf("---------------------rdpdr_server_Event [%s] is upload-----------------\n", pNext->path->m_path);
			//tlog(TLOG_INFO, pData->session_id, 0, "[filesystem] upload file: %s\n", pNext->path->m_path);
			//auditor_file_event_produce(AUDITOR_EVENT_TYPE_CLIPBOARD_UPLOAD, pData->ps->uuid, pNext->path->m_path, pNext->path->size, file_pos, pData->config->AuditorDumpFilePath);
			pNext = pNext->next;
		}


	} else {
		pNext = list;
		pOldNext = pOldList;

		while(pNext) {
			if(NULL == auditor_rdpdr_find_path_list(pOldList, pNext->path->m_path)){
				//printf("---------------------rdpdr_server_Event [%s] is upload-----------------\n", pNext->path->m_path);
				//tlog(TLOG_INFO, pData->session_id, 0, "[filesystem] upload file: %s\n", pNext->path->m_path);
				//auditor_file_event_produce(AUDITOR_EVENT_TYPE_CLIPBOARD_UPLOAD, pData->ps->uuid, pNext->path->m_path, pNext->path->size, file_pos, pData->config->AuditorDumpFilePath);
			}
			pNext = pNext->next;
		}
	}
	auditor_rdpdr_add_path_table(table, key, list);
}

void auditor_rdpdr_client_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	wStream* s = NULL;
	UINT16 component;
	UINT16 packetId;

	if (pEvent->flags & CHANNEL_FLAG_FIRST || !auditor_ctx->rdpdr_client_stream)
	{
		if (auditor_ctx->rdpdr_client_stream != NULL) {
			Stream_Free(auditor_ctx->rdpdr_client_stream, TRUE);
		}
		auditor_ctx->rdpdr_client_stream = Stream_New(NULL, pEvent->data_len);
		Stream_SetPosition(auditor_ctx->rdpdr_client_stream, 0);
	}
	s = auditor_ctx->rdpdr_client_stream;
	if (!Stream_EnsureRemainingCapacity(s, pEvent->data_len))
	{
		return;
	}	
	Stream_Write(s, pEvent->data, pEvent->data_len);


	if (!(pEvent->flags & CHANNEL_FLAG_LAST)) {
		return;
	}

	Stream_SetPosition(s, 0);
	if (Stream_GetRemainingLength(s) < 8)
		return;
	Stream_Read_UINT16(s, component); /* Component (2 bytes) */
	Stream_Read_UINT16(s, packetId);  /* PacketId (2 bytes) */
	if (component == 0x4472 /*RDPDR_CTYP_CORE*/) {
		switch (packetId)
		{
			case PAKID_CORE_SERVER_ANNOUNCE:
				break;

			case PAKID_CORE_SERVER_CAPABILITY:
				break;

			case PAKID_CORE_CLIENTID_CONFIRM:
				break;

			case PAKID_CORE_USER_LOGGEDON:
				break;

			case PAKID_CORE_DEVICE_REPLY:
				break;

			case PAKID_CORE_DEVICE_IOREQUEST:
				{
					UINT32 DeviceId;
					UINT32 FileId;
					UINT32 CompletionId;
					UINT32 MajorFunction;
					UINT32 MinorFunction;

					//printf("cliboard_filter_client_Event  PAKID_CORE_DEVICE_IOREQUEST\n" );

					if (Stream_GetRemainingLength(s) < 20)
						return;

					Stream_Read_UINT32(s, DeviceId); /* DeviceId (4 bytes) */
					Stream_Read_UINT32(s, FileId);        /* FileId (4 bytes) */
					Stream_Read_UINT32(s, CompletionId);  /* CompletionId (4 bytes) */
					Stream_Read_UINT32(s, MajorFunction); /* MajorFunction (4 bytes) */
					Stream_Read_UINT32(s, MinorFunction); /* MinorFunction (4 bytes) */
					//printf("cliboard_filter_client_Event MajorFunction: 0x%x -----------------\n", MajorFunction);

					if (MajorFunction == IRP_MJ_CREATE) {
						//UINT32 FileId;
						//DRIVE_FILE* file;
						//BYTE Information;
						UINT32 FileAttributes;
						UINT32 SharedAccess;
						UINT32 DesiredAccess;
						UINT32 CreateDisposition;
						UINT32 CreateOptions;
						UINT32 PathLength;
						UINT64 allocationSize;
						const WCHAR* path;

						if (Stream_GetRemainingLength(s) < 6 * 4 + 8)
							return;

						Stream_Read_UINT32(s, DesiredAccess);
						Stream_Read_UINT64(s, allocationSize);
						Stream_Read_UINT32(s, FileAttributes);
						Stream_Read_UINT32(s, SharedAccess);
						Stream_Read_UINT32(s, CreateDisposition);
						Stream_Read_UINT32(s, CreateOptions);
						Stream_Read_UINT32(s, PathLength);

						if (Stream_GetRemainingLength(s) < PathLength)
							return;

						path = (const WCHAR*)Stream_Pointer(s);
						{
							if (PathLength < s->length) {
								WCHAR* path2 = (WCHAR*)calloc(PathLength + 1, sizeof(WCHAR));
								memcpy(path2, path, PathLength);
								LPSTR lpFileNameA;
								if (ConvertFromUnicode(CP_UTF8, 0, path2, -1, &lpFileNameA, 0, NULL, NULL) < 1)
									return;
								free(path2);

								//https://learn.microsoft.com/zh-cn/windows/win32/api/fileapi/nf-fileapi-createfilea
								
								if((CreateDisposition == 1 || CreateDisposition == 2) && ((DesiredAccess & 0x00000004) || (DesiredAccess & 0x00000002))) {
									printf("++++++++++++++ request create file path:[%s]\n", auditor_ctx->g_createNewFilePath);
									if(auditor_ctx->g_createNewFilePath)
										free(auditor_ctx->g_createNewFilePath);
									auditor_ctx->g_createNewFilePath = lpFileNameA;
									auditor_ctx->g_createNewFileAttributes = FileAttributes;
									auditor_ctx->g_createNewFileNeed = true;									
								} else {
									printf("++++++++++++++ request open file path:[%s]\n", auditor_ctx->g_createNewFilePath);
									if(auditor_ctx->g_openFilePath)
										free(auditor_ctx->g_openFilePath);
									auditor_ctx->g_openFilePath = lpFileNameA;
									auditor_ctx->g_openFileNeed = true;										
								}

								//free(lpFileNameA);
							}
						}
					} else if (MajorFunction == IRP_MJ_READ) {
						printf("================= read file %d %s\n", auditor_ctx->g_openFileNeed, auditor_ctx->g_openFilePath);
						if(auditor_ctx->g_openFileNeed == true) {
							printf("++++++++++++++ download file path:[%s]\n", auditor_ctx->g_openFilePath);
							auditor_ctx->g_openFileNeed = false;
						}
						
					} else if (MajorFunction == IRP_MJ_WRITE) {
						printf("================= write file %d %s\n", auditor_ctx->g_openFileNeed, auditor_ctx->g_openFilePath);
						if(auditor_ctx->g_openFileNeed == true) {
							printf("++++++++++++++ upload file path:[%s]\n", auditor_ctx->g_openFilePath);
							auditor_ctx->g_openFileNeed = false;
						}
					}
				}
				break;
			default:
				break;
		}
	}
}

void auditor_rdpdr_server_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	wStream* s = NULL;
	UINT16 msgRDPDRCTYP;
	UINT16 msgRDPDRPAKID;
	UINT32 DeviceId;
	UINT32 CompletionId;
	UINT32 IoStatus;

	if (pEvent->flags & CHANNEL_FLAG_FIRST || !auditor_ctx->rdpdr_server_stream)
	{
		if (auditor_ctx->rdpdr_server_stream != NULL) {
			Stream_Free(auditor_ctx->rdpdr_server_stream, TRUE);
		}
		auditor_ctx->rdpdr_server_stream = Stream_New(NULL, pEvent->data_len);
		Stream_SetPosition(auditor_ctx->rdpdr_server_stream, 0);
	}
	s = auditor_ctx->rdpdr_server_stream;
	if (!Stream_EnsureRemainingCapacity(s, pEvent->data_len))
	{
		return;
	}	
	Stream_Write(s, pEvent->data, pEvent->data_len);


	if (!(pEvent->flags & CHANNEL_FLAG_LAST)) {
		return;
	}

	Stream_SetPosition(s, 0);
	if (Stream_GetRemainingLength(s) < 8)
		return;

	Stream_Read_UINT16(s, msgRDPDRCTYP); // Component (2 bytes)
	Stream_Read_UINT16(s, msgRDPDRPAKID); // PacketId (2 bytes)
	Stream_Read_UINT32(s, DeviceId);       // DeviceId (4 bytes)
	Stream_Read_UINT32(s, CompletionId);              // CompletionId (4 bytes)
	Stream_Read_UINT32(s, IoStatus);                              // IoStatus (4 bytes)
	//printf("---------------------rdpdr_server_Event  CompletionId:[%x] IoStatus:[%x]-----------------\n", CompletionId, IoStatus);

	if (auditor_ctx->g_createNewFileNeed) {
		auditor_ctx->g_createNewFileNeed = false;
		if (IoStatus == 0) {
			printf("++++++++++++++ create file path:[%s]\n", auditor_ctx->g_createNewFilePath);
		}
	}

	return;
}
