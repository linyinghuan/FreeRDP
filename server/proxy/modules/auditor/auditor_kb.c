#include "auditor_kb.h"

int ctrl_flag = 0;
int shift_flag = 0;
int num_lock = 0;
int alt_flag = 0;
int caps_flag = 0;

#define IS_EXT_CODE(code) ((code) & 0x100)
#define IS_NUM_CODE(code) (((code) >= 0x47) && ((code) <= 0x53))
#define IS_AZ_CODE(code) ((((code) >= 0x10) && ((code) <= 0x19)) || (((code) >= 0x1E) && ((code) <= 0x26)) || (((code) >= 0x2C) && ((code) <= 0x32)))

typedef struct {
	UINT16 code;
	char *key;
	char *ext_key;
} CODE_MAP;

CODE_MAP code_map_info[] = {
{RDP_SCANCODE_ESCAPE, "ESC", NULL},
{RDP_SCANCODE_KEY_1, "1", "!"},
{RDP_SCANCODE_KEY_2, "2", "@"},
{RDP_SCANCODE_KEY_3, "3", "#"},
{RDP_SCANCODE_KEY_4, "4", "$"},
{RDP_SCANCODE_KEY_5, "5", "%"},
{RDP_SCANCODE_KEY_6, "6", "^"},
{RDP_SCANCODE_KEY_7, "7", "&"},
{RDP_SCANCODE_KEY_8, "8", "*"},
{RDP_SCANCODE_KEY_9, "9", "("},
{RDP_SCANCODE_KEY_0, "0", ")"},
{RDP_SCANCODE_OEM_MINUS, "-", "_"},
{RDP_SCANCODE_OEM_PLUS, "+", "="},
{RDP_SCANCODE_BACKSPACE, "BACKSPACE", NULL},
{RDP_SCANCODE_TAB, "TAB", NULL},
{RDP_SCANCODE_KEY_Q, "q", "Q"},
{RDP_SCANCODE_KEY_W, "w", "W"},
{RDP_SCANCODE_KEY_E, "e", "E"},

{RDP_SCANCODE_KEY_R, "r", "R"},
{RDP_SCANCODE_KEY_T, "t", "T"},
{RDP_SCANCODE_KEY_Y, "y", "Y"},
{RDP_SCANCODE_KEY_I, "i", "I"},
{RDP_SCANCODE_KEY_O, "o", "O"},
{RDP_SCANCODE_KEY_P, "p", "P"},
{RDP_SCANCODE_OEM_4, "[", "{"},
{RDP_SCANCODE_OEM_6, "]", "}"},
{RDP_SCANCODE_RETURN, "ENTER", NULL},
{RDP_SCANCODE_KEY_A, "a", "A"},
{RDP_SCANCODE_KEY_S, "s", "S"},
{RDP_SCANCODE_KEY_D, "d", "D"},
{RDP_SCANCODE_KEY_F, "f", "F"},
{RDP_SCANCODE_KEY_G, "g", "G"},
{RDP_SCANCODE_KEY_H, "h", "H"},
{RDP_SCANCODE_KEY_J, "j", "J"},
{RDP_SCANCODE_KEY_K, "k", "K"},
{RDP_SCANCODE_KEY_L, "l", "L"},
{RDP_SCANCODE_OEM_1, ";", ":"},
{RDP_SCANCODE_OEM_7, "'", "\""},

{RDP_SCANCODE_KEY_Z, "z", "Z"},
{RDP_SCANCODE_KEY_X, "x", "X"},
{RDP_SCANCODE_KEY_C, "c", "C"},
{RDP_SCANCODE_KEY_V, "v", "V"},
{RDP_SCANCODE_KEY_B, "b", "B"},
{RDP_SCANCODE_KEY_N, "n", "N"},
{RDP_SCANCODE_KEY_M, "m", "M"},
{RDP_SCANCODE_OEM_COMMA, ",", "<"},
{RDP_SCANCODE_OEM_PERIOD, ".", ">"},
{RDP_SCANCODE_OEM_2, "/", "?"},

{RDP_SCANCODE_SPACE, " ", NULL},
{RDP_SCANCODE_F1, "F1", NULL},
{RDP_SCANCODE_F2, "F2", NULL},
{RDP_SCANCODE_F3, "F3", NULL},
{RDP_SCANCODE_F4, "F4", NULL},
{RDP_SCANCODE_F5, "F5", NULL},
{RDP_SCANCODE_F6, "F6", NULL},
{RDP_SCANCODE_F7, "F7", NULL},
{RDP_SCANCODE_F8, "F8", NULL},
{RDP_SCANCODE_F9, "F9", NULL},
{RDP_SCANCODE_F10, "F10", NULL},
{RDP_SCANCODE_F11, "F11", NULL},
{RDP_SCANCODE_F12, "F12", NULL},

{RDP_SCANCODE_SCROLLLOCK, "SCROLLLOCK", "PAUSE"},
{RDP_SCANCODE_MULTIPLY, "*", "PRINTSCREEN"},
{RDP_SCANCODE_NUMPAD7, "7", "HOME"},
{RDP_SCANCODE_NUMPAD8, "8", "UP"},
{RDP_SCANCODE_NUMPAD9, "9", "PAGE UP"},
{RDP_SCANCODE_SUBTRACT, "-", NULL},
{RDP_SCANCODE_NUMPAD4, "4", "LEFT"},
{RDP_SCANCODE_NUMPAD5, "5", NULL},
{RDP_SCANCODE_NUMPAD6, "6", "RIGHT"},
{RDP_SCANCODE_ADD, "+", NULL},
{RDP_SCANCODE_NUMPAD1, "1", "END"},
{RDP_SCANCODE_NUMPAD2, "2", "DOWN"},
{RDP_SCANCODE_NUMPAD3, "3", "PAGE DOWN"},
{RDP_SCANCODE_NUMPAD0, "0", "INSERT"},
{RDP_SCANCODE_DECIMAL, ".", "DELETE"},
};

BOOL auditor_keyboard_event_handler(proxyData* pData, void* param, AUDITOR_CTX_DATA *auditor_ctx)
{
	static CODE_MAP *code_map_table[256] = {0};
	static int code_map_tbl_init = 0;
	int i = 0;
	CODE_MAP *pCodeMap = NULL;
	BOOL extend_code = FALSE;

	if(0 == code_map_tbl_init) {
		for(i = 0; i < sizeof(code_map_info)/sizeof(CODE_MAP); i++) {
			code_map_table[RDP_SCANCODE_CODE(code_map_info[i].code)] = &code_map_info[i];
		}

		code_map_tbl_init = 1;
	}

	proxyKeyboardEventInfo* event_data = (proxyKeyboardEventInfo*)param;
	if (event_data == NULL)
		return FALSE;

	if  (event_data->rdp_scan_code == RDP_SCANCODE_LCONTROL || event_data->rdp_scan_code == RDP_SCANCODE_RCONTROL) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			auditor_ctx->ctrl_flag = 0;
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			auditor_ctx->ctrl_flag = 1;
		}		
	}

	if  (event_data->rdp_scan_code == RDP_SCANCODE_LMENU || event_data->rdp_scan_code == RDP_SCANCODE_RMENU) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			auditor_ctx->alt_flag = 0;
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			auditor_ctx->alt_flag = 1;
		}		
	}	

	if  (event_data->rdp_scan_code == RDP_SCANCODE_LSHIFT || event_data->rdp_scan_code == RDP_SCANCODE_RSHIFT) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			auditor_ctx->shift_flag = 0;
		}
		if (event_data->flags & KBD_FLAGS_DOWN) {
			auditor_ctx->shift_flag = 1;
		}		
	}

	if  (event_data->rdp_scan_code == RDP_SCANCODE_CAPSLOCK) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			auditor_ctx->caps_flag = (0 == auditor_ctx->shift_flag);
		}	
	}

	if  (event_data->rdp_scan_code == RDP_SCANCODE_NUMLOCK) {
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			auditor_ctx->num_lock = (0 == auditor_ctx->num_lock);
		}	
	}


	if (event_data->flags & KBD_FLAGS_RELEASE) {
		char buf[1024] = {0};

		pCodeMap = code_map_table[RDP_SCANCODE_CODE(event_data->rdp_scan_code)];

		if(pCodeMap){
			//printf("input: %04X KBD_FLAGS_RELEASE\n", event_data->rdp_scan_code);
			if(IS_NUM_CODE(RDP_SCANCODE_CODE(event_data->rdp_scan_code))) {
				if(auditor_ctx->num_lock == 1 || IS_EXT_CODE(RDP_SCANCODE_CODE(event_data->rdp_scan_code)))
					extend_code = TRUE;
			} else if(IS_AZ_CODE(RDP_SCANCODE_CODE(event_data->rdp_scan_code))) {
				if(auditor_ctx->caps_flag == 1 || auditor_ctx->shift_flag == 1)
					extend_code = TRUE;
			} else {
				if (auditor_ctx->shift_flag)
					extend_code = TRUE;
			}

			if(extend_code && pCodeMap->ext_key)
				sprintf(buf,"input: %s%s%s\n", auditor_ctx->ctrl_flag?"ctrl + ":" ", auditor_ctx->alt_flag?"alt + ":" ", pCodeMap->ext_key);
			else
				sprintf(buf,"input: %s%s%s\n", auditor_ctx->ctrl_flag?"ctrl + ":" ", auditor_ctx->alt_flag?"alt + ":" ", pCodeMap->key);
			printf("%s", buf);
			//tlog(TLOG_INFO, pData->session_id, 0, "[keyboard] input: %04X\n", event_data->rdp_scan_code);			
		}
	}

	//printf("input: %04X  flag: %04x --\n", event_data->rdp_scan_code, event_data->flags);
	if (event_data->rdp_scan_code == RDP_SCANCODE_KEY_A || event_data->rdp_scan_code == RDP_SCANCODE_LMENU ||
		event_data->rdp_scan_code == RDP_SCANCODE_DECIMAL || event_data->rdp_scan_code == RDP_SCANCODE_LCONTROL)
	{
		if (event_data->flags & KBD_FLAGS_RELEASE) {
			//printf("input: %04X KBD_FLAGS_RELEASE\n", event_data->rdp_scan_code);
			//tlog(TLOG_INFO, "[keyboard] input: %04X (ctrl=%d shift=%d caps=%d)\n", event_data->rdp_scan_code, ctrl_flag, shift_flag, caps_flag);
		}
	}
	return TRUE;
}