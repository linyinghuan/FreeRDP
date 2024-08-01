#include "auditor_mouse.h"

BOOL auditor_mouse_event_handler(proxyData* pdata, void* param)
{
	proxyMouseEventInfo* event_data = (proxyMouseEventInfo*)param;
	if (event_data == NULL)
		return FALSE;

	if (event_data->flags & PTR_FLAGS_DOWN) {
		printf("mouse: %04X\n", event_data->flags);
		if (event_data->flags & PTR_FLAGS_DOWN)
			printf("PTR_FLAGS_DOWN ");
		if (event_data->flags & PTR_FLAGS_MOVE)
			printf("PTR_FLAGS_MOVE ");
		if (event_data->flags & PTR_FLAGS_BUTTON1)
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] left click at: %d:%d)\n", event_data->x, event_data->y);
		if (event_data->flags & PTR_FLAGS_BUTTON2)
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] right click at: %d:%d)\n", event_data->x, event_data->y);
		if (event_data->flags & PTR_FLAGS_BUTTON3)
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] mid click at: %d:%d)\n", event_data->x, event_data->y);
		if (event_data->flags & PTR_FLAGS_WHEEL)
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] wheel click at: %d:%d)\n", event_data->x, event_data->y);
		if (event_data->flags & PTR_FLAGS_HWHEEL)
			tlog(TLOG_INFO, pData->session_id, 0,  "[mouse] hwheel click at: %d:%d)\n", event_data->x, event_data->y);
		printf("\n");
	}
	return TRUE;
}