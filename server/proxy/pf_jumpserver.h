#ifndef FREERDP_SERVER_PROXY_JUMPSERVER_H
#define FREERDP_SERVER_PROXY_JUMPSERVER_H

typedef struct jumpserver_connect_info
{
	char* uuid;
	char* ip;
	int port;
	char* username;
	char* password;
	char* domain;
	char* remoteapp;
	char* remoteapp_program;
	char* remoteapp_workdir;
	char* remoteapp_command_line;
	char* errorMsg;
	int enable_connect;
	int enable_upload;
	int enable_download;
	int enable_paste;
	int enable_copy;
	int need_recording;
	int64_t expired_at;
	int64_t expired_session_at;
	int64_t max_idle_seconds;
	int enable_console;
} jmsConnectInfo;

typedef enum {
	AUDITOR_EVENT_TYPE_KB = 1,
	AUDITOR_EVENT_TYPE_MOUSE,
	AUDITOR_EVENT_TYPE_CLIPBOARD_UPLOAD,
	AUDITOR_EVENT_TYPE_CLIPBOARD_DOWNLOAD,
	AUDITOR_EVENT_TYPE_FILESYS_UPLOAD,
	AUDITOR_EVENT_TYPE_FILESYS_DOWNLOAD,
	AUDITOR_EVENT_TYPE_MAX
} jms_auditor_event_type;

typedef enum {
	AUDITOR_EVENT_DATA_TYPE_TEXT = 1,
	AUDITOR_EVENT_DATA_TYPE_MOUSE,
	AUDITOR_EVENT_DATA_TYPE_FILE,
	AUDITOR_EVENT_DATA_TYPE_MAX
} jms_auditor_event_data_type;

typedef struct {
	char*  sid;
	char*  user_name;
	jms_auditor_event_type event_type;
	time_t event_time;
	time_t event_time_abs;
	jms_auditor_event_data_type data_type;
	void*  event_data;
} jms_auditor_event;

typedef struct {
	UINT32 text_len;
	char * text;
} jms_auditor_text_data;

typedef enum {
	AUDITOR_MOUSE_OP_LEFT_CLICK = 1,
	AUDITOR_MOUSE_OP_RIGHT_CLICK,
	AUDITOR_MOUSE_OP_MID_CLICK,
	AUDITOR_MOUSE_OP_WHEEL_CLICK,
	AUDITOR_MOUSE_OP_MAX
} jms_auditor_mouse_op;

typedef struct {
	UINT32 x;
	UINT32 y;
} jms_auditor_point;

typedef struct {
	jms_auditor_mouse_op op_code;
	jms_auditor_point pos;
} jms_auditor_mouse_data;

typedef enum {
	AUDITOR_IO_STATUS_SUCCESS = 0,
	AUDITOR_IO_STATUS_FAIL,
	AUDITOR_IO_STATUS_MAX
} jms_auditor_io_status;

typedef struct {
	char* 	file_name;
	UINT64	file_size;
	jms_auditor_point 	file_pos;
	jms_auditor_io_status  file_io_status;
	char * 	backup_path;
} jms_auditor_file_data;

void init_server(char* path);

jmsConnectInfo get_connect_info(char* token, char* secret);

char* create_session(char* uuid, char* addr);

int finish_session(char* uuid);

int check_kill_task(char* uuid);

int finish_kill_task(char* uuid);

void record_init(char*, int, int);

void record_end(char*);

void recording_pix_img(char*, uint8_t*, int, int, int, int, int);

int check_expired(char* uuid);

void update_active(char* uuid);

void update_session_failed(char*, int);

int64_t get_current_time();

void auditor_event_proc(jms_auditor_event *event);

#endif