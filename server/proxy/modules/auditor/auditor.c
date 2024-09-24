/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server Session Capture Module
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
#include <errno.h>
#include "auditor.h"
#include "auditor_kb.h"
#include "auditor_mouse.h"
#include "auditor_channel.h"

#define TAG MODULE_TAG("auditor")

#define PLUGIN_NAME "auditor"
#define PLUGIN_DESC "proxy data/event auditor plugin"

UINT32 g_auditor_enable = 1;

static proxyPluginsManager* g_plugins_manager = NULL;


static BOOL auditor_plugin_unload(void)
{
	return TRUE;
}

static proxyPlugin auditor_plugin = {
	PLUGIN_NAME,                        /* name */
	PLUGIN_DESC,                        /* description */
	auditor_plugin_unload,              /* PluginUnload */
	NULL,                               /* ClientPreConnect */
	NULL, 								/* ClientPostConnect */
	NULL,                               /* ClientLoginFailure */
	NULL,    							/* ClientEndPaint */
	NULL, 								/* ServerPostConnect */
	NULL,                               /* ServerChannelsInit */
	NULL,                               /* ServerChannelsFree */
	NULL,         						/* Session End */
	NULL,                               /* KeyboardEvent */
};

BOOL proxy_module_entry_point(proxyPluginsManager* plugins_manager)
{
	g_plugins_manager = plugins_manager;

	tlog_init("auditor.log", 1024 * 1024, 8, 0, 0);
	auditor_plugin.MouseEvent = auditor_mouse_event_handler;
	auditor_plugin.KeyboardEvent = auditor_keyboard_event_handler;
	auditor_plugin.ServerChannelsInit = auditor_server_channels_init;
	auditor_plugin.ClientAuditorData = auditor_client_channel_handler;
	auditor_plugin.ServerAuditorData = auditor_server_channel_handler;
	return plugins_manager->RegisterPlugin(&auditor_plugin);
}

BOOL auditor_set_plugin_data(proxyData* pData, void* data)
{
	return g_plugins_manager->SetPluginData(PLUGIN_NAME, pData, data);
}

void* auditor_get_plugin_data(proxyData* pData)
{
	return g_plugins_manager->GetPluginData(PLUGIN_NAME, pData);
}

void auditor_text_event_produce(jms_auditor_event_type event_type, char *sid, char *text)
{
	jms_auditor_event *event;
	jms_auditor_text_data *event_data;
	char *event_sid;
	char *event_text;

	printf("produce text event type:%d sid:%s text:%s", event_type, sid, text);
	if(event_type == AUDITOR_EVENT_TYPE_CLIPBOARD_UPLOAD || event_type == AUDITOR_EVENT_TYPE_CLIPBOARD_DOWNLOAD)
		tlog(TLOG_INFO, sid, 0, "[clipboard] copy UNICODETEXT: %s", text);
	else
		tlog(TLOG_INFO, sid, 0, "[keyboard] copy UNICODETEXT: %s", text);

	event = malloc(sizeof(jms_auditor_event));
	if(!event)
		return;
	memset(event, 0, sizeof(jms_auditor_event));

	event_sid = malloc(strlen(sid) + 1);
	if(!event_sid)
		return;	
	memset(event_sid, 0, strlen(sid) + 1);
	strcpy(event_sid, sid);

	event_text = malloc(strlen(text) + 1);
	if(!event_text)
		return;	
	memset(event_text, 0, strlen(text) + 1);
	strcpy(event_text, text);

	event_data = malloc(sizeof(jms_auditor_text_data));
	if(!event)
		return;
	memset(event_data, 0, sizeof(jms_auditor_text_data));
	event_data->text = event_text;
	event_data->text_len = strlen(event_text);

	event->sid = event_sid;
	event->user_name = NULL;
	event->event_type = event_type;
	event->data_type = AUDITOR_EVENT_DATA_TYPE_TEXT;
	event->event_data = event_data;

	//auditor_event_proc(event);
}

void auditor_file_event_produce(jms_auditor_event_type event_type, char *sid, char *file_name,
								UINT64 file_size, jms_auditor_point file_pos, char *file_path, jms_auditor_io_status status)
{
	jms_auditor_event *event;
	jms_auditor_file_data *event_data;
	char *event_sid;
	char *event_file_name;

	printf("produce file event type:%d sid:%s file name:%s file path:%s\n", event_type, sid, file_name, file_path);
	if(event_type == AUDITOR_EVENT_TYPE_CLIPBOARD_UPLOAD || event_type == AUDITOR_EVENT_TYPE_CLIPBOARD_DOWNLOAD)
		tlog(TLOG_INFO, sid, 0, "[clipboard] copy UNICODETEXT: %s", file_name);
	else
		tlog(TLOG_INFO, sid, 0, "[rdpdr] copy UNICODETEXT: %s", file_name);

	if(!sid) {
		printf("sid is NULL\n");
		return;
	}

	if(!file_name) {
		printf("file_name is NULL\n");
		return;
	}	

	if(!file_path) {
		printf("file_path is NULL\n");
		return;
	}	

	event = malloc(sizeof(jms_auditor_event));
	if(!event)
		return;
	memset(event, 0, sizeof(jms_auditor_event));

	event_sid = malloc(strlen(sid) + 1);
	if(!event_sid)
		return;	
	memset(event_sid, 0, strlen(sid) + 1);
	strcpy(event_sid, sid);

	event_file_name = malloc(strlen(file_name) + 1);
	if(!event_file_name)
		return;	
	memset(event_file_name, 0, strlen(file_name) + 1);
	strcpy(event_file_name, file_name);

	event_data = malloc(sizeof(jms_auditor_file_data));
	if(!event)
		return;
	memset(event_data, 0, sizeof(jms_auditor_file_data));
	event_data->file_name = file_name;
	event_data->file_size = file_size;
	event_data->file_pos = file_pos;
	event_data->backup_path = file_path;

	event->sid = event_sid;
	event->user_name = NULL;
	event->event_type = event_type;
	event->data_type = AUDITOR_EVENT_DATA_TYPE_FILE;
	event->event_data = event_data;

	//auditor_event_proc(event);
}

void auditor_mouse_event_produce(jms_auditor_event_type event_type, char *sid, jms_auditor_mouse_op op_code, jms_auditor_point pos)
{
	jms_auditor_event *event;
	jms_auditor_mouse_data *event_data;
	char *event_sid;

	printf("produce mouse event type:%d sid:%s op:%d", event_type, sid, op_code);

	event = malloc(sizeof(jms_auditor_event));
	if(!event)
		return;
	memset(event, 0, sizeof(jms_auditor_event));

	event_sid = malloc(strlen(sid) + 1);
	if(!event_sid)
		return;	
	memset(event_sid, 0, strlen(sid) + 1);
	strcpy(event_sid, sid);

	event_data = malloc(sizeof(jms_auditor_mouse_data));
	if(!event)
		return;
	memset(event_data, 0, sizeof(jms_auditor_mouse_data));
	event_data->op_code = op_code;
	event_data->pos = pos;

	event->sid = event_sid;
	event->user_name = NULL;
	event->event_type = event_type;
	event->data_type = AUDITOR_EVENT_DATA_TYPE_MOUSE;
	event->event_data = event_data;

	//auditor_event_proc(event);
}