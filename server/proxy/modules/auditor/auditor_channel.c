#include "auditor.h"
#include "auditor_channel.h"

static AUDITOR_EVENT_CB auditor_client_cb_tbl[AUDITOR_EVENT_MAX];
static AUDITOR_EVENT_CB auditor_server_cb_tbl[AUDITOR_EVENT_MAX];

BOOL auditor_server_channels_init(proxyData* pdata)
{
	AUDITOR_CTX_DATA *auditor_ctx = NULL;

	if(FALSE == pdata->config->AuditorEnable) {
		g_auditor_enable = 0;
		return TRUE;
	}

	printf("session %s server channel init\n", pdata->session_id);

	if(NULL == auditor_get_plugin_data(pdata)) {
		auditor_ctx = malloc(sizeof(AUDITOR_CTX_DATA));
		memset(auditor_ctx, 0, sizeof(AUDITOR_CTX_DATA));

		auditor_ctx->file_map = hash_table_init(1024*1024);

		auditor_set_plugin_data(pdata, auditor_ctx);
	}

	if(TRUE == pdata->config->AuditorDumpFileEnable) {
		sprintf(auditor_ctx->dump_file_path, "%s%s/", pdata->config->AuditorDumpFilePath, pdata->ps->uuid);
		printf("session file path:%s\n", auditor_ctx->dump_file_path);

		mkdir(auditor_ctx->dump_file_path,0777);
	}	

	return TRUE;
}

BOOL auditor_client_channel_handler(proxyData* pdata, void* context)
{
	AUDITOR_CTX_DATA *auditor_ctx = NULL;

	if(0 == g_auditor_enable)
		return TRUE;

	proxyChannelDataEventInfo* pEvent = (proxyChannelDataEventInfo *)context;

	auditor_ctx = auditor_get_plugin_data(pdata);
	if(NULL == auditor_ctx) {
		printf("client channel data without auditor ctx\n");
		return TRUE;
	}

	if ( 0 == strncmp(pEvent->channel_name, "cliprdr", strlen("cliprdr") )) {
		if(auditor_client_cb_tbl[AUDITOR_EVENT_CLIPB])
			auditor_client_cb_tbl[AUDITOR_EVENT_CLIPB](pdata, pEvent, auditor_ctx);
	}

	if ( 0 == strncmp(pEvent->channel_name, "rdpdr", strlen("rdpdr") )) {
		if(auditor_client_cb_tbl[AUDITOR_EVENT_RDPDR])
			auditor_client_cb_tbl[AUDITOR_EVENT_RDPDR](pdata, pEvent, auditor_ctx);
	}

	return TRUE;
}

BOOL auditor_server_channel_handler(proxyData* pdata, void* context)
{
	AUDITOR_CTX_DATA *auditor_ctx = NULL;
	proxyChannelDataEventInfo* pEvent = (proxyChannelDataEventInfo *)context;

	if(0 == g_auditor_enable)
		return TRUE;

	auditor_ctx = auditor_get_plugin_data(pdata);
	if(NULL == auditor_ctx) {
		printf("server channel data without auditor ctx\n");
		return TRUE;
	}

	if ( 0 == strncmp(pEvent->channel_name, "cliprdr", strlen("cliprdr") )) {
		if(auditor_server_cb_tbl[AUDITOR_EVENT_CLIPB])
			auditor_server_cb_tbl[AUDITOR_EVENT_CLIPB](pdata, pEvent, auditor_ctx);
	}

	if ( 0 == strncmp(pEvent->channel_name, "rdpdr", strlen("rdpdr") )) {
		if(auditor_server_cb_tbl[AUDITOR_EVENT_RDPDR])
			auditor_server_cb_tbl[AUDITOR_EVENT_RDPDR](pdata, pEvent, auditor_ctx);
	}

	return TRUE;
}

BOOL auditor_channel_event_reg(UINT32 mode, UINT32 type, AUDITOR_EVENT_CB callback)
{
	BOOL rtn = TRUE;

	if(!callback) {
		printf("auditor_channel_event_reg: callback is NULL\n");
		rtn = FALSE;
	}

	if(type <= 0 || type >= AUDITOR_EVENT_MAX) {
		rtn = FALSE;		
		printf("auditor_channel_event_reg: type(%d) error\n", type);
	}

	if(mode == AUDITOR_CLIENT) {
		auditor_client_cb_tbl[type] = callback;
	} else if(mode == AUDITOR_SERVER) {
		auditor_server_cb_tbl[type] = callback;
	} else {
		printf("auditor_channel_event_reg: mode(%d) error\n", mode);
		rtn = FALSE;		
	}

	return rtn;
}