#ifndef AUDITOR_H
#define AUDITOR_H

#include <errno.h>
#include "modules_api.h"
#include "tlog.h"

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

} AUDITOR_CTX_DATA;

#define TAG MODULE_TAG("auditor")

#endif