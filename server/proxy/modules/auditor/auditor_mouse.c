#include "auditor_mouse.h"

BOOL auditor_mouse_event_handler(proxyData* pData, void* param)
{
	proxyMouseEventInfo* event_data = (proxyMouseEventInfo*)param;
	jms_auditor_point pos =  {0};

	AUDITOR_MOUSE_OP_LEFT_CLICK = 1,
	AUDITOR_MOUSE_OP_RIGHT_CLICK,
	AUDITOR_MOUSE_OP_MID_CLICK,
	AUDITOR_MOUSE_OP_WHEEL_CLICK,

	if (event_data == NULL)
		return FALSE;

	if (event_data->flags & PTR_FLAGS_DOWN) {
		printf("mouse: %04X\n", event_data->flags);
		pos.x = event_data->x;
		pos.y = event_data->y;
		if (event_data->flags & PTR_FLAGS_DOWN)
			printf("PTR_FLAGS_DOWN ");
		if (event_data->flags & PTR_FLAGS_MOVE)
			printf("PTR_FLAGS_MOVE ");
		if (event_data->flags & PTR_FLAGS_BUTTON1) {
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] left click at: %d:%d)\n", event_data->x, event_data->y);
			auditor_mouse_file_event_produce(AUDITOR_EVENT_TYPE_MOUSE, pData->ps->sid, AUDITOR_MOUSE_OP_LEFT_CLICK, pos);
		}

		if (event_data->flags & PTR_FLAGS_BUTTON2) {
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] right click at: %d:%d)\n", event_data->x, event_data->y);
			auditor_mouse_file_event_produce(AUDITOR_EVENT_TYPE_MOUSE, pData->ps->sid, AUDITOR_MOUSE_OP_RIGHT_CLICK, pos);
		}
		if (event_data->flags & PTR_FLAGS_BUTTON3) {
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] mid click at: %d:%d)\n", event_data->x, event_data->y);
			auditor_mouse_file_event_produce(AUDITOR_EVENT_TYPE_MOUSE, pData->ps->sid, AUDITOR_MOUSE_OP_MID_CLICK, pos);
		}
		if (event_data->flags & PTR_FLAGS_WHEEL) {
			tlog(TLOG_INFO, pData->session_id, 0, "[mouse] wheel click at: %d:%d)\n", event_data->x, event_data->y);
			auditor_mouse_file_event_produce(AUDITOR_EVENT_TYPE_MOUSE, pData->ps->sid, AUDITOR_MOUSE_OP_WHEEL_CLICK, pos);
		}
	}
	return TRUE;
}