#ifndef AUDITOR_H
#define AUDITOR_H

#include <errno.h>
#include "modules_api.h"
#include "pf_jumpserver.h"
#include "tlog.h"

extern UINT32 g_auditor_enable;

typedef struct {
	char *m_path;
	int m_isDir;
	UINT64 size;
} AUDITOR_RDPDR_PATH;

typedef struct auditor_rdpdr_path_list_node {
	struct auditor_rdpdr_path_list_node *next;

	AUDITOR_RDPDR_PATH *path;
} AUDITOR_RDPDR_PATH_LIST_NODE;

typedef struct {
	AUDITOR_RDPDR_PATH_LIST_NODE *node;
} AUDITOR_RDPDR_PATH_LIST_HEAD;

typedef struct auditor_rdpdr_path_table_node {
	struct auditor_rdpdr_path_table_node *next;

	char *path_key;
	AUDITOR_RDPDR_PATH_LIST_NODE *path_list;
} AUDITOR_RDPDR_PATH_TABLE_NODE;

typedef struct  {
	AUDITOR_RDPDR_PATH_TABLE_NODE *node;
} AUDITOR_RDPDR_PATH_TABLE_HEAD;

typedef struct {
	char dump_file_path[512];

	/* keyboard data */
	int ctrl_flag;
	int shift_flag;
	int num_lock;
	int alt_flag;
	int caps_flag;

	/* clipboard data */
	UINT32 formatID;
	CLIPRDR_FORMAT_LIST formatList;
	wStream* clip_stream;
	wStream* clip_client_stream;

	/* rdpdr data */
	wStream* rdpdr_client_stream;
	wStream* rdpdr_server_stream;
	UINT32 g_FsInformationClass;
	bool g_need;
	bool g_createNewFileNeed;
	char *g_createNewFilePath;
	bool g_writeFileNeed;
	char *g_writeFilePath;
	bool g_readFileNeed;
	char *g_readFilePath;
	bool g_readFileDatasNeed;
	UINT32 g_readFileMaxCompId;
	UINT64 *g_readFileOffset;	
	UINT32 g_createNewFileAttributes;
	char *g_newPath;
	AUDITOR_RDPDR_PATH_TABLE_HEAD g_rdpdrpath;
	AUDITOR_RDPDR_PATH_LIST_HEAD  g_rdpdrpath_list;

} AUDITOR_CTX_DATA;

BOOL auditor_set_plugin_data(proxyData* pData, void* data);
void* auditor_get_plugin_data(proxyData* pData);

void auditor_text_event_produce(jms_auditor_event_type event_type, char *sid, char *text);
void auditor_file_event_produce(jms_auditor_event_type event_type, char *sid, char *file_name,
								UINT64 file_size, jms_auditor_point file_pos, char *file_path);
void auditor_mouse_event_produce(jms_auditor_event_type event_type, char *sid, jms_auditor_mouse_op op_code, jms_auditor_point pos);

#define TAG MODULE_TAG("auditor")

#endif