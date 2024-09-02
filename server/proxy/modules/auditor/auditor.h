#ifndef AUDITOR_H
#define AUDITOR_H

#include <errno.h>
#include "modules_api.h"
#include "tlog.h"

typedef struct {
	char *m_path;
	int m_isDir;
} AUDITOR_RDPDR_PATH;

typedef struct auditor_rdpdr_path_list_node {
	struct auditor_rdpdr_path_list_node *next;

	AUDITOR_RDPDR_PATH *path;
} AUDITOR_RDPDR_PATH_LIST_NODE;

typedef struct auditor_rdpdr_path_table_node {
	struct auditor_rdpdr_path_table_node *next;

	char *path_key;
	AUDITOR_RDPDR_PATH_LIST_NODE *path_list;
} AUDITOR_RDPDR_PATH_TABLE_NODE;

typedef struct {
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

	/* rdpdr data */
	wStream* rdpdr_stream;
	UINT32 g_FsInformationClass;
	bool g_need;
	bool g_createNewFileNeed;
	char *g_createNewFilePath;
	UINT32 g_createNewFileAttributes;
	char *g_newPath;
	AUDITOR_RDPDR_PATH_TABLE_NODE *g_rdpdrpath;
	AUDITOR_RDPDR_PATH_LIST_NODE *g_rdpdrpath_list;

} AUDITOR_CTX_DATA;

BOOL auditor_set_plugin_data(proxyData* pData, void* data);
void* auditor_get_plugin_data(proxyData* pData);

#define TAG MODULE_TAG("auditor")

#endif