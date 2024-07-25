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
			printf("left ");
		if (event_data->flags & PTR_FLAGS_BUTTON2)
			printf("right ");
		if (event_data->flags & PTR_FLAGS_BUTTON3)
			printf("middle ");
		if (event_data->flags & PTR_FLAGS_WHEEL)
			printf("wheel ");
		if (event_data->flags & PTR_FLAGS_HWHEEL)
			printf("hwheel ");
		printf("\n");
	}
	return TRUE;
}