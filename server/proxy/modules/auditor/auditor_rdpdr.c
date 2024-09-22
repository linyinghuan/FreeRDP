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

void auditor_rdpdr_client_event_handler(proxyData* pData, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx)
{
	wStream* s = NULL;
	UINT16 component;
	UINT16 packetId;
	jms_auditor_point file_pos = {0};

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

							if (PathLength > 0 && PathLength < s->length) {
								WCHAR* path2 = (WCHAR*)calloc(PathLength + 1, sizeof(WCHAR));
								memcpy(path2, path+1, PathLength-1);
								LPSTR lpFileNameA;
								if (ConvertFromUnicode(CP_UTF8, 0, path2, -1, &lpFileNameA, 0, NULL, NULL) < 1)
									return;
								free(path2);

								//printf("================= create file %s %lx %lx\n", lpFileNameA, CreateDisposition, DesiredAccess);

								//https://learn.microsoft.com/zh-cn/windows/win32/api/fileapi/nf-fileapi-createfilea

								if((CreateDisposition == 1 || CreateDisposition == 2) && ((DesiredAccess & 0x00000004) || (DesiredAccess & 0x00000002))) {
									if(auditor_ctx->g_createNewFilePath)
										free(auditor_ctx->g_createNewFilePath);
									auditor_ctx->g_createNewFilePath = lpFileNameA;
									auditor_ctx->g_createNewFileAttributes = FileAttributes;
									auditor_ctx->g_createNewFileNeed = true;	
									printf("----------------- request create file path:[%s]\n", auditor_ctx->g_createNewFilePath);								
								} else if((DesiredAccess & SYNCHRONIZE) && (DesiredAccess & STANDARD_RIGHTS_WRITE) &&
											 ((DesiredAccess&FILE_WRITE_DATA) || (DesiredAccess&FILE_WRITE_EA))){
									if(auditor_ctx->g_writeFilePath)
										free(auditor_ctx->g_writeFilePath);
									auditor_ctx->g_writeFilePath = lpFileNameA;
									auditor_ctx->g_writeFileNeed = true;
									printf("----------------- request write file path:[%s]\n", auditor_ctx->g_writeFilePath);
								} else if((DesiredAccess & SYNCHRONIZE) && (DesiredAccess & STANDARD_RIGHTS_READ) &&
											 ((DesiredAccess&FILE_READ_DATA) || (DesiredAccess&FILE_READ_EA))){
									auditor_ctx->g_readFileDatasNeed = false;
									if(auditor_ctx->g_readFilePath)
										free(auditor_ctx->g_readFilePath);
									auditor_ctx->g_readFilePath = lpFileNameA;
									auditor_ctx->g_readFileNeed = true;
									auditor_ctx->g_readFileMaxCompId = 32;
									if(auditor_ctx->g_readFileOffset)
										free(auditor_ctx->g_readFileOffset);
									auditor_ctx->g_readFileOffset = malloc(auditor_ctx->g_readFileMaxCompId*sizeof(UINT64));
									memset(auditor_ctx->g_readFileOffset, 0xff, auditor_ctx->g_readFileMaxCompId*sizeof(UINT64));
									printf("----------------- request read file path:[%s]\n", auditor_ctx->g_readFilePath);
								}

								//free(lpFileNameA);
							}
						}
					} else if (MajorFunction == IRP_MJ_READ) {
						UINT32 length;
						UINT64 offset;

						Stream_Read_UINT32(s, length);
						Stream_Read_UINT64(s, offset);
						printf("================= read file %d %s %d %ld\n", auditor_ctx->g_readFileNeed, auditor_ctx->g_readFilePath, CompletionId, offset);
						if(CompletionId < auditor_ctx->g_readFileMaxCompId) {
							*(auditor_ctx->g_readFileOffset + CompletionId) = offset;
						} else {
							auditor_ctx->g_readFileMaxCompId += 32;
							auditor_ctx->g_readFileOffset = realloc(auditor_ctx->g_readFileOffset, auditor_ctx->g_readFileMaxCompId*sizeof(UINT64));
							*(auditor_ctx->g_readFileOffset + CompletionId) = offset;
						}

						if(auditor_ctx->g_readFileNeed == true) {
							char file_path[1024] = {0};
							FILE* fp = NULL;
							sprintf(file_path, "%s%s", auditor_ctx->dump_file_path, auditor_ctx->g_readFilePath);

							fp = fopen(file_path,"w");
							if(fp)
								fclose(fp);

							printf("++++++++++++++ download file path:[%s]\n", auditor_ctx->g_readFilePath);
							tlog(TLOG_INFO, pData->session_id, 0, "[filesystem] download file: %s\n", auditor_ctx->g_readFilePath);
							auditor_file_event_produce(AUDITOR_EVENT_TYPE_FILESYS_DOWNLOAD, pData->ps->uuid, auditor_ctx->g_readFilePath, 0, file_pos, pData->config->AuditorDumpFilePath);							
							auditor_ctx->g_readFileNeed = false;
							auditor_ctx->g_readFileDatasNeed = true;
						}
						
					} else if (MajorFunction == IRP_MJ_WRITE) {
						UINT32 length;
						UINT64 offset;
						char file_path[1024] = {0};
						FILE* fp = NULL;

						printf("================= write file %d %s\n", auditor_ctx->g_writeFileNeed, auditor_ctx->g_writeFilePath);
						sprintf(file_path, "%s%s", auditor_ctx->dump_file_path, auditor_ctx->g_writeFilePath);
						if(auditor_ctx->g_writeFileNeed == true) {
							printf("++++++++++++++ upload file path:[%s]\n", auditor_ctx->g_writeFilePath);
							auditor_ctx->g_writeFileNeed = false;
							auditor_file_event_produce(AUDITOR_EVENT_TYPE_FILESYS_UPLOAD, pData->ps->uuid, auditor_ctx->g_writeFilePath, 0, file_pos, pData->config->AuditorDumpFilePath);	
							fp = fopen(file_path,"w");
							if(fp)
								fclose(fp);							
						}

						Stream_Read_UINT32(s, length);
						Stream_Read_UINT64(s, offset);
						Stream_Seek(s, 20);

						printf("++++++++++++++ write file data:[%s] offset[%ld], len[%d]\n", file_path, offset, length);
						fp = fopen(file_path,"r+");

						fseek(fp, offset, SEEK_SET);
						if(fp) {
							fwrite(s->pointer, length, 1, fp);
							fclose(fp);				
						}
					} else if (MajorFunction == IRP_MJ_SET_INFORMATION) {
						UINT32 FsInformationClass;
						UINT32 Length;
						UINT8 ReplaceIfExists;
						UINT8 RootDirectory;
						UINT32 FileNameLength;
						const WCHAR* path;

						Stream_Read_UINT32(s, FsInformationClass);
						Stream_Read_UINT32(s, Length);
						Stream_Seek(s, 24);
						if(FsInformationClass == FileRenameInformation) {
							Stream_Read_UINT8(s, ReplaceIfExists);
							Stream_Read_UINT8(s, RootDirectory);
							Stream_Read_UINT32(s, FileNameLength);					

							path = (const WCHAR*)Stream_Pointer(s);
							if (FileNameLength < s->length) {
								WCHAR* path2 = (WCHAR*)calloc(FileNameLength + 1, sizeof(WCHAR));
								memcpy(path2, path, FileNameLength);
								LPSTR lpFileNameA;
								if (ConvertFromUnicode(CP_UTF8, 0, path2, -1, &lpFileNameA, 0, NULL, NULL) < 1)
									return;
								free(path2);

								printf("++++++++++++++ rename file new name %s\n", lpFileNameA);
								auditor_file_event_produce(AUDITOR_EVENT_TYPE_FILESYS_UPLOAD, pData->ps->uuid, lpFileNameA, 0, file_pos, pData->config->AuditorDumpFilePath);						
							}	
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
	jms_auditor_point file_pos = {0};

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
	//printf("---------------------rdpdr_server_Event msgRDPDRCTYP[%lx] msgRDPDRPAKID[%lx] CompletionId:[%x] IoStatus:[%x]-----------------\n", msgRDPDRCTYP, msgRDPDRPAKID, CompletionId, IoStatus);

	if (auditor_ctx->g_createNewFileNeed) {
		auditor_ctx->g_createNewFileNeed = false;
		if (IoStatus == 0) {
			printf("++++++++++++++ create file path:[%s]\n", auditor_ctx->g_createNewFilePath);

			auditor_file_event_produce(AUDITOR_EVENT_TYPE_FILESYS_UPLOAD, pData->ps->uuid, auditor_ctx->g_createNewFilePath, 0, file_pos, pData->config->AuditorDumpFilePath);
		}
	}

	if(msgRDPDRCTYP == 0x4472 && msgRDPDRPAKID == PAKID_CORE_DEVICE_IOCOMPLETION) {
		if(auditor_ctx->g_readFileDatasNeed == true && 
			*(auditor_ctx->g_readFileOffset + CompletionId) != (-1L)) {
			UINT32 length;
			char file_path[1024] = {0};
			FILE* fp = NULL;

			Stream_Read_UINT32(s, length);

			sprintf(file_path, "%s%s", auditor_ctx->dump_file_path, auditor_ctx->g_readFilePath);
			printf("++++++++++++++ read file data:[%s] offset[%ld], len[%d] status[%d]\n", file_path, *(auditor_ctx->g_readFileOffset + CompletionId), length, IoStatus);
			fp = fopen(file_path,"r+");

			fseek(fp, *(auditor_ctx->g_readFileOffset + CompletionId), SEEK_SET);
			if(fp) {
				fwrite(s->pointer, length, 1, fp);
				fclose(fp);				
			}
			*(auditor_ctx->g_readFileOffset + CompletionId) = (-1L);
			//auditor_ctx->g_readFileDatasNeed = false;
		}		
	}

	return;
}
