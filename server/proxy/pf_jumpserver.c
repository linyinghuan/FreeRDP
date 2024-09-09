#include <winpr/library.h>
#include <stdio.h>
#include <winpr/synch.h>
#include <winpr/string.h>

#include "pf_jumpserver.h"

#define TAG PROXY_TAG("jumpserver")

#define jumpserverLib "/usr/lib/libjumpserver.so"
#define init_serice_name "init_service"
#define get_connect_info_name "get_connect_info"
#define create_session_name "create_session"
#define finish_session_name "finish_session"
#define check_kill_task_name "check_kill_task"
#define finish_kill_task_name "finish_kill_task"
#define record_init_name "record_init"
#define record_end_name "record_end"
#define recording_pix_img_name "recording_pix_img"
#define check_expired_name "check_expired"
#define update_active_name "update_active"
#define get_current_time_name "current_time"
#define update_session_failed_name "update_session_failed"

#define audit_kb_event_proc_name "audit_kb_event"
#define audit_mouse_event_proc_name "audit_mouse_event"
#define audit_clipboard_text_event_proc_name "audit_clipboard_text_event"
#define audit_clipboard_file_event_proc_name "audit_clipboard_file_event"
#define audit_filesys_event_proc_name "audit_filesys_event"

static void* jumpserverHandle = NULL;

typedef jmsConnectInfo (*get_connect_info_func)(char* a, char* b);
typedef char* (*create_session_func)(char* a, char* b);
typedef int (*finish_session_func)(char* id);
typedef int (*check_terminate_func)(char* id);
typedef int (*finish_kill_task_func)(char* id);
typedef void (*record_init_func)(char*, int, int);
typedef void (*record_end_func)(char*);
typedef void (*recording_pix_img_func)(char*, uint8_t*, int, int, int, int, int);
typedef int (*check_expired_func)(char*);
typedef void (*update_active_func)(char*);
typedef int64_t (*get_current_time_func)();
typedef void (*update_session_failed_func)(char*,int);

// audit event name 相关
typedef void (*audit_kb_event_func)(char* id, char* txt, int64_t text_len);
typedef void (*audit_mouse_event_func)(char* id, int op, int x, int y);
typedef void (*audit_clipboard_text_event_func)(char* id, int op, char* txt, int64_t text_len );
typedef void (*audit_clipboard_file_event_func)(char* id,int op,char* filename, int64_t file_size, char* path);
typedef void (*audit_filesys_event_func)(char* id, int op,char* filename, int64_t file_size,char* path);

static get_connect_info_func GetConnectInfo = NULL;
static create_session_func CreateSession = NULL;
static finish_session_func FinishSession = NULL;
static check_terminate_func CheckTerminate = NULL;
static finish_kill_task_func FinishKillTask = NULL;
static record_init_func RecordInit = NULL;
static record_end_func RecordEnd = NULL;
static recording_pix_img_func RecordingPixImg = NULL;
static check_expired_func CheckExpired = NULL;
static update_active_func UpdateActive = NULL;
static get_current_time_func GetCurrentTime = NULL;
static update_session_failed_func UpdateSessionFailed = NULL;


// audit func 

static audit_kb_event_func AuditKbEvent = NULL;
static audit_mouse_event_func AuditMouseEvent = NULL;
static audit_clipboard_text_event_func AuditCliTextEvent = NULL;
static audit_clipboard_file_event_func AuditCliFileEvent = NULL;
static audit_filesys_event_func AuditFilesysEvent = NULL;

void init_server(char* path)
{
	jumpserverHandle = LoadLibraryA(jumpserverLib);
	void (*init_service_func)(char* a);
	init_service_func = GetProcAddress(jumpserverHandle, init_serice_name);
	GetConnectInfo = GetProcAddress(jumpserverHandle, get_connect_info_name);
	CreateSession = GetProcAddress(jumpserverHandle, create_session_name);
	FinishSession = GetProcAddress(jumpserverHandle, finish_session_name);
	CheckTerminate = GetProcAddress(jumpserverHandle, check_kill_task_name);
	FinishKillTask = GetProcAddress(jumpserverHandle, finish_kill_task_name);
	RecordInit = GetProcAddress(jumpserverHandle, record_init_name);
	RecordEnd = GetProcAddress(jumpserverHandle, record_end_name);
	RecordingPixImg = GetProcAddress(jumpserverHandle, recording_pix_img_name);
	CheckExpired = GetProcAddress(jumpserverHandle, check_expired_name);
	UpdateActive = GetProcAddress(jumpserverHandle, update_active_name);
	GetCurrentTime = GetProcAddress(jumpserverHandle, get_current_time_name);
	UpdateSessionFailed = GetProcAddress(jumpserverHandle, update_session_failed_name);
	AuditKbEvent = GetProcAddress(jumpserverHandle, audit_kb_event_proc_name);
	AuditMouseEvent = GetProcAddress(jumpserverHandle, audit_mouse_event_proc_name);
	AuditCliTextEvent = GetProcAddress(jumpserverHandle, audit_clipboard_text_event_proc_name);
	AuditCliFileEvent = GetProcAddress(jumpserverHandle, audit_clipboard_file_event_proc_name);
	AuditFilesysEvent = GetProcAddress(jumpserverHandle, audit_filesys_event_proc_name);
	init_service_func(path);
}

jmsConnectInfo get_connect_info(char* token, char* secret)
{
	return GetConnectInfo(token, secret);
}

char* create_session(char* uuid, char* addr)
{
	return CreateSession(uuid, addr);
}

int finish_session(char* uuid)
{
	return FinishSession(uuid);
}

int check_kill_task(char* uuid)
{
	return CheckTerminate(uuid);
}

int finish_kill_task(char* uuid)
{
	return FinishKillTask(uuid);
}

void record_init(char* uuid, int width, int height)
{
	RecordInit(uuid, width, height);
}

void record_end(char* uuid)
{
	RecordEnd(uuid);
}

void recording_pix_img(char* uuid, uint8_t* frame, int size, int width, int height,
                       int target_width, int target_height)
{
	RecordingPixImg(uuid, frame, size, width, height, target_width, target_height);
}

int check_expired(char* uuid)
{
    return CheckExpired(uuid);
}

void update_active(char* uuid)
{
    UpdateActive(uuid);
}

int64_t get_current_time()
{
	return GetCurrentTime();
}

void update_session_failed(char* uuid, int code)
{
	UpdateSessionFailed(uuid, code);
}

void auditor_event_proc(jms_auditor_event *event)
{
	jms_auditor_text_data* text_data;
	jms_auditor_mouse_data* mouse_data;
	jms_auditor_file_data* file_data;
	switch (event->event_type) {
		case AUDITOR_EVENT_TYPE_KB:
			switch (event->data_type) {
				case ANDITOR_EVENT_DATA_TYPE_TEXT:
					text_data = (jms_auditor_text_data*)event->event_data;
					AuditKbEvent(event->sid, text_data->text, text_data->text_len);
					break;
				default:
					printf("Unknown kb event data type: %d\n", event->data_type);
					break;
				}
			break;

		case AUDITOR_EVENT_TYPE_MOUSE:
			switch (event->data_type) {
				case ANDITOR_EVENT_DATA_TYPE_MOUSE:
					mouse_data = (jms_auditor_mouse_data*)event->event_data;
					AuditMouseEvent(event->sid,mouse_data->op_code, mouse_data->pos.x, mouse_data->pos.y);
					break;
				default:
				printf("Unknown mouse event data type: %d\n", event->data_type);
				break;
			}
			break;

		case AUDITOR_EVENT_TYPE_CLIPBOARD_UPLOAD:
		case AUDITOR_EVENT_TYPE_CLIPBOARD_DOWNLOAD:
			switch (event->data_type) {
				case ANDITOR_EVENT_DATA_TYPE_TEXT:
					text_data = (jms_auditor_text_data*)event->event_data;
					AuditCliTextEvent(event->sid,0, text_data->text, text_data->text_len);
					break;
				case ANDITOR_EVENT_DATA_TYPE_FILE:
					file_data = (jms_auditor_file_data*)event->event_data;
					AuditCliFileEvent(event->sid,0, file_data->file_name, file_data->file_size, file_data->backup_path);
					break;
				default:
					printf("Unknown clipboard event data type: %d\n", event->data_type);
					break;
			}
			break;

        case AUDITOR_EVENT_TYPE_FILESYS_UPLOAD:
        case AUDITOR_EVENT_TYPE_FILESYS_DOWNLOAD:
			switch (event->data_type) {
				case ANDITOR_EVENT_DATA_TYPE_FILE:
					file_data = (jms_auditor_file_data*)event->event_data;
					AuditFilesysEvent(event->sid,0, file_data->file_name, file_data->file_size, file_data->backup_path);
					break;
				default:
					printf("Unknown filesys event data type: %d\n", event->data_type);
					break;
			}
			break;
			
        default:
            printf("Unknown event type: %d\n", event->event_type);
            break;
	}
}