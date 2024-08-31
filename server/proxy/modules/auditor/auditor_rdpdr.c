#include "auditor.h"
#include "auditor_channel.h"
#include "auditor_clip.h"

void auditor_rdpdr_client_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);
void auditor_rdpdr_server_event_handler(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);

__attribute__((constructor)) static int auditor_rdpdr_init()
{
	auditor_channel_event_reg(AUDITOR_CLIENT, AUDITOR_EVENT_RDPDR, auditor_rdpdr_client_event_handler);
	auditor_channel_event_reg(AUDITOR_SERVER, AUDITOR_EVENT_RDPDR, auditor_rdpdr_server_event_handler);

	return 0;
}



void auditor_rdpdr_client_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	wStream* s = NULL;
	UINT16 component;
	UINT16 packetId;
	UINT32 deviceId;
	UINT32 status;
	UINT error = ERROR_INVALID_DATA;

	if (pEvent->flags & CHANNEL_FLAG_FIRST)
	{
		if (auditor_ctx->rdpdr_stream != NULL) {
			Stream_Free(auditor_ctx->rdpdr_stream, TRUE);
		}
		auditor_ctx->rdpdr_stream = Stream_New(NULL, pEvent->data_len);
		Stream_SetPosition(auditor_ctx->rdpdr_stream, 0);
	}
	s = auditor_ctx->rdpdr_stream;

	Stream_Write(s, pEvent->data, pEvent->data_len);
	if (!(pEvent->flags & CHANNEL_FLAG_LAST)) {
		goto finish;
	}

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

					printf("cliboard_filter_client_Event  PAKID_CORE_DEVICE_IOREQUEST\n" );

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
						{
							if (PathLength < s->length) {
								WCHAR* path2 = (WCHAR*)calloc(PathLength + 1, sizeof(WCHAR));
								memcpy(path2, path, PathLength);
								LPSTR lpFileNameA;
								if (ConvertFromUnicode(CP_UTF8, 0, path2, -1, &lpFileNameA, 0, NULL, NULL) < 1)
									return true;
								free(path2);
								printf("=========IRP_MJ_CREATE path:[%s]\n", lpFileNameA);
								printf("=========IRP_MJ_CREATE CreateDisposition:[0x%x] [0x%x] [0x%x] [0x%x]\n", CreateDisposition, CreateOptions, FileAttributes, DesiredAccess);

								//https://learn.microsoft.com/zh-cn/windows/win32/api/fileapi/nf-fileapi-createfilea
								if ((CreateDisposition == 1 || CreateDisposition == 2) && ((DesiredAccess & 0x00000004) || (DesiredAccess & 0x00000002) ) ) {
									g_createNewFilePath = lpFileNameA;
									g_createNewFileNeed = true;
									g_createNewFileAttributes = FileAttributes;
								}
								free(lpFileNameA);
							}
						}
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
								{
									if (PathLength < s->length) {
										WCHAR* path2 = (WCHAR*)calloc(PathLength + 1, sizeof(WCHAR));
										memcpy(path2, path, PathLength);
										LPSTR lpFileNameA;
										if (ConvertFromUnicode(CP_UTF8, 0, path2, -1, &lpFileNameA, 0, NULL, NULL) < 1)
											return true;
										free(path2);
										printf("=========IRP_MN_QUERY_DIRECTORY path:[%s]\n", lpFileNameA);
										if (strlen(lpFileNameA) > 0) {
											g_newPath = lpFileNameA;
											g_rdpdrpathnew.clear();
										}
										free(lpFileNameA);
									}
								}
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
	UINT8 ShortNameLength;
	LPSTR lpFileNameA;
	UINT error;

	if (pEvent->flags & CHANNEL_FLAG_FIRST)
	{
		if (auditor_ctx->rdpdr_stream != NULL) {
			Stream_Free(auditor_ctx->rdpdr_stream, TRUE);
		}
		auditor_ctx->rdpdr_stream = Stream_New(NULL, pEvent->data_len);
		Stream_SetPosition(auditor_ctx->rdpdr_stream, 0);
	}
	s = auditor_ctx->rdpdr_stream;

	Stream_Write(s, pEvent->data, pEvent->data_len);
	if (!(pEvent->flags & CHANNEL_FLAG_LAST)) {
		goto finish;
	}

	if (Stream_GetRemainingLength(s) < 8)
		return true;

	Stream_Read_UINT16(s, msgRDPDRCTYP); // Component (2 bytes)
	Stream_Read_UINT16(s, msgRDPDRPAKID); // PacketId (2 bytes)
	Stream_Read_UINT32(s, DeviceId);       // DeviceId (4 bytes)
	Stream_Read_UINT32(s, CompletionId);              // CompletionId (4 bytes)
	Stream_Read_UINT32(s, IoStatus);                              // IoStatus (4 bytes)
	printf("---------------------rdpdr_server_Event  CompletionId:[%x] IoStatus:[%x]-----------------\n", CompletionId, IoStatus);


	if (g_createNewFileNeed) {
		g_createNewFileNeed = false;
		if (IoStatus == 0) {
			printf("---------------------rdpdr_server_Event  file [%s] is created-----------------\n", g_createNewFilePath.c_str());
		}
	}

	if (!g_need)
		return true;

	if (IoStatus == STATUS_NO_MORE_FILES ) {
		printf("---------------------rdpdr_server_Event  STATUS_NO_MORE_FILES -----------------\n");

		if (g_rdpdrpath.find(g_newPath) != g_rdpdrpath.end()) {
			printf("---------------------rdpdr_server_Event  g_rdpdrpath find path [%s] -----------------\n", g_newPath.c_str());
			for(list<FileBriefInfo>::iterator itb = g_rdpdrpathnew.begin(); itb != g_rdpdrpathnew.end(); itb++) {
				bool bExist = false;
				for(list<FileBriefInfo>::iterator ita = g_rdpdrpath[g_newPath].begin(); ita != g_rdpdrpath[g_newPath].end(); ita++) {
					if (*ita == *itb) {
						bExist = true;
						break;
					}
				}
				if (!bExist) {
					printf("---------------------rdpdr_server_Event [%s] is upload-----------------\n", itb->m_path.c_str());
				}
			}

		}
		else {
			for(list<FileBriefInfo>::iterator itb = g_rdpdrpathnew.begin(); itb != g_rdpdrpathnew.end(); itb++) {
				printf("---------------------rdpdr_server_Event [%s] is upload (new)-----------------\n", itb->m_path.c_str());
			}
		}
		g_rdpdrpath[g_newPath] = g_rdpdrpathnew;
		return true;
	}


	g_need = false;
	switch (g_FsInformationClass)
	{
		case FileDirectoryInformation:
			Stream_Read_UINT32(s, totalLength); /* Length */
			Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
			Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */
			Stream_Read_UINT32(s,CreationTime); /* CreationTime */
			Stream_Read_UINT32(s,CreationTime2); /* CreationTime */
			Stream_Read_UINT32(s, LastAccessTime); /* LastAccessTime */
			Stream_Read_UINT32(s, LastAccessTime2); /* LastAccessTime */
			Stream_Read_UINT32(s,LastWriteTime); /* LastWriteTime */
			Stream_Read_UINT32(s,LastWriteTime2); /* LastWriteTime */
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
			Stream_Read_UINT32(s,CreationTime); /* CreationTime */
			Stream_Read_UINT32(s,CreationTime2); /* CreationTime */
			Stream_Read_UINT32(s, LastAccessTime); /* LastAccessTime */
			Stream_Read_UINT32(s, LastAccessTime2); /* LastAccessTime */
			Stream_Read_UINT32(s,LastWriteTime); /* LastWriteTime */
			Stream_Read_UINT32(s,LastWriteTime2); /* LastWriteTime */
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
			if (totalLength == 0) {
				printf("=========total length is 0, maybe MJ_Close\n");
				return true;
			}
			Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
			Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */
			Stream_Read_UINT32(s, CreationTime); /* CreationTime */
			Stream_Read_UINT32(s, CreationTime2); /* CreationTime */
			Stream_Read_UINT32(s, LastAccessTime); /* LastAccessTime */
			Stream_Read_UINT32(s, LastAccessTime2); /* LastAccessTime */
			Stream_Read_UINT32(s, LastWriteTime); /* LastWriteTime */
			Stream_Read_UINT32(s, LastWriteTime2); /* LastWriteTime */
			Stream_Read_UINT32(s, ChangeTime); /* ChangeTime */
			Stream_Read_UINT32(s, ChangeTime2); /* ChangeTime */
			Stream_Read_UINT32(s, EndOfFile);           /* EndOfFile */
			Stream_Read_UINT32(s, EndOfFile2);          /* EndOfFile */
			Stream_Read_UINT32(s, AllocationSize);     /* AllocationSize */
			Stream_Read_UINT32(s, AllocationSize2);    /* AllocationSize */
			Stream_Read_UINT32(s, FileAttributes); /* FileAttributes */
			Stream_Read_UINT32(s, Length);                   /* FileNameLength */
			Stream_Read_UINT32(s, EaSize);                                /* EaSize */
			Stream_Read_UINT8(s, ShortNameLength);
			Stream_Seek(s, 24);
			//Stream_Zero(output, 24); /* ShortName */
			path = (WCHAR*)Stream_Pointer(s);
			{
				if (Length < s->length) {
					WCHAR* path2 = (WCHAR*)calloc(Length + 1, sizeof(WCHAR));
					memcpy(path2, path, Length);

					if (ConvertFromUnicode(CP_UTF8, 0, path2, -1, &lpFileNameA, 0, NULL, NULL) < 1)
						return true;
					free(path2);
					printf("=========FileBothDirectoryInformation path:[%s]\n", lpFileNameA);
					printf("=========FileBothDirectoryInformation FileAttributes:[0x%x]\n", FileAttributes);
					if (FileAttributes | 0x00000010) {
						FileBriefInfo fileBriefInfo;
						fileBriefInfo.m_path = lpFileNameA;
						fileBriefInfo.m_isDir = 1;
						g_rdpdrpathnew.push_back(fileBriefInfo);;
					}else {
						FileBriefInfo fileBriefInfo;
						fileBriefInfo.m_path = lpFileNameA;
						fileBriefInfo.m_isDir = 0;
						g_rdpdrpathnew.push_back(fileBriefInfo);
					}
					free(lpFileNameA);
				}
			}
			break;

		case FileNamesInformation:
			Stream_Read_UINT32(s, totalLength); /* Length */
			Stream_Read_UINT32(s, NextEntryOffset);                     /* NextEntryOffset */
			Stream_Read_UINT32(s, FileIndex);                     /* FileIndex */
			Stream_Read_UINT32(s, Length);                   /* FileNameLength */
			path = (WCHAR*)Stream_Pointer(s);
			if (ConvertFromUnicode(CP_UTF8, 0, path, -1, &lpFileNameA, 0, NULL, NULL) < 1)
				return NULL;
			printf("=========filename:[%s]\n", lpFileNameA);
			break;

		default:
			break;
	}
}
