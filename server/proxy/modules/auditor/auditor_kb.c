#include "auditor_kb.h"

int ctrl_flag = 0;
int shift_flag = 0;
int caps_flag = 0;

BOOL auditor_keyboard_event_handler(proxyData* pdata, void* param)
{
	proxyKeyboardEventInfo* event_data = (proxyKeyboardEventInfo*)param;
	if (event_data == NULL)
		return FALSE;

	if  (event_data->rdp_scan_code == RDP_SCANCODE_LCONTROL || event_data->rdp_scan_code == RDP_SCANCODE_RCONTROL) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			ctrl_flag = 0;
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			ctrl_flag = 1;
		}		
	}

	if  (event_data->rdp_scan_code == RDP_SCANCODE_LSHIFT || event_data->rdp_scan_code == RDP_SCANCODE_RSHIFT) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			shift_flag = 0;
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			shift_flag = 1;
		}		
	}

	if  (event_data->rdp_scan_code == RDP_SCANCODE_CAPSLOCK) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			shift_flag = (0 == shift_flag);
		}	
	}

	printf("input: %04X  flag: %04x --\n", event_data->rdp_scan_code, event_data->flags);
	if (event_data->rdp_scan_code == RDP_SCANCODE_KEY_A || event_data->rdp_scan_code == RDP_SCANCODE_LMENU ||
		event_data->rdp_scan_code == RDP_SCANCODE_DECIMAL || event_data->rdp_scan_code == RDP_SCANCODE_LCONTROL)
	{
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			//printf("input: %04X KBD_FLAGS_RELEASE\n", event_data->rdp_scan_code);
			tlog(TLOG_INFO, "[keyboard] input: %04X (ctrl=%d shift=%d caps=%d)\n", event_data->rdp_scan_code, ctrl_flag, shift_flag, caps_flag);
		}
	}
	return TRUE;
}