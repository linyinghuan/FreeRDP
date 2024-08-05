#include "auditor.h"
#include "auditor_channel.h"

static AUDITOR_EVENT_CB auditor_client_cb_tbl[AUDITOR_EVENT_MAX];
static AUDITOR_EVENT_CB auditor_server_cb_tbl[AUDITOR_EVENT_MAX];

BOOL auditor_server_channels_init(proxyData* pdata)
{
	printf(TAG, "auditor plugin called\n");
	return TRUE;
}

BOOL auditor_client_channel_handler(proxyData* data, void* context)
{
	proxyChannelDataEventInfo* pEvent = (proxyChannelDataEventInfo *)context;

	if ( 0 == strncmp(pEvent->channel_name, "cliprdr", strlen("cliprdr") )) {
		if(auditor_client_cb_tbl[AUDITOR_EVENT_CLIPB])
			auditor_client_cb_tbl[AUDITOR_EVENT_CLIPB](data, pEvent);
	}

	return TRUE;
}

BOOL auditor_server_channel_handler(proxyData* data, void* context)
{
	proxyChannelDataEventInfo* pEvent = (proxyChannelDataEventInfo *)context;

	if ( 0 == strncmp(pEvent->channel_name, "cliprdr", strlen("cliprdr") )) {
		if(auditor_server_cb_tbl[AUDITOR_EVENT_CLIPB])
			auditor_server_cb_tbl[AUDITOR_EVENT_CLIPB](data, pEvent);
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