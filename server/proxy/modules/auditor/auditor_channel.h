#ifndef AUDITOR_CHANNEL_H
#define AUDITOR_CHANNEL_H

#include "auditor.h"

#define AUDITOR_CLIENT 0
#define AUDITOR_SERVER 1

typedef enum{
	AUDITOR_EVENT_CLIPB = 1,
	AUDITOR_EVENT_FILESYS,
	AUDITOR_EVENT_MAX,
} AUDITOR_EVENT_TYPE;

typedef VOID(*AUDITOR_EVENT_CB)(proxyData* pdata, proxyChannelDataEventInfo* pEvent, AUDITOR_CTX_DATA *auditor_ctx);

BOOL auditor_server_channels_init(proxyData* pdata);
BOOL auditor_client_channel_handler(proxyData* data, void* context);
BOOL auditor_server_channel_handler(proxyData* data, void* context);
BOOL auditor_channel_event_reg(UINT32 mode, UINT32 type, AUDITOR_EVENT_CB callback);
#endif