#include "auditor_kb.h"

int ctrl_flag = 0;
int shift_flag = 0;

BOOL auditor_keyboard_event_handler(proxyData* pdata, void* param)
{
	proxyKeyboardEventInfo* event_data = (proxyKeyboardEventInfo*)param;
	if (event_data == NULL)
		return FALSE;

	printf("input: %04X  flag: %04x --\n", event_data->rdp_scan_code, event_data->flags);
	if (event_data->rdp_scan_code == RDP_SCANCODE_KEY_A || event_data->rdp_scan_code == RDP_SCANCODE_LMENU ||
		event_data->rdp_scan_code == RDP_SCANCODE_DECIMAL || event_data->rdp_scan_code == RDP_SCANCODE_LCONTROL)
	{
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			//printf("input: %04X KBD_FLAGS_RELEASE\n", event_data->rdp_scan_code);
			tlog(TLOG_INFO, "input: %04X KBD_FLAGS_RELEASE\n", event_data->rdp_scan_code);
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			printf("input: %04X KBD_FLAGS_DOWN\n", event_data->rdp_scan_code);
		}
	}
	return TRUE;
}