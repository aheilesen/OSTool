#ifndef OST_DATA_H_
#define OST_DATA_H_

#include <Windows.h>
#include "andrewll.h"

#define LINE_LENGTH       500
#define MAX_VARIANTS      1500
#define VAR_DESC_LENGTH   60
#define FAM_DESC_LENGTH   30
#define SYMBOL_LENGTH     8
#define IDVAR6_LENGTH     6

#define SW_NAME_LENGTH    20

#define BUF_LENGTH        100

// COLORS
#define VOLVO_BLUE (RGB(24,40,113))
#define ORANGE     (RGB(255,136,26))
#define LT_GRAY    (RGB(240,240,240))
#define GRAY       (RGB(184,184,184))
#define DARK_GRAY  (RGB(96,96,96))
#define CTA_BLUE   (RGB(0,79,188))
#define BLACK      (RGB(0,0,0))
#define WHITE      (RGB(255,255,255))

typedef struct variant {
	char idvar6[IDVAR6_LENGTH + 1];
	char symbol[SYMBOL_LENGTH + 1];
	char fam_desc[FAM_DESC_LENGTH + 1];
	char var_desc[VAR_DESC_LENGTH + 1];
} Variant;

typedef struct sw_link {
	int loc;
	int pn;
	char* vars;
	int qty;
} SW_link;

typedef struct vss_num_dlg {
	int num_1;
	int num_2;
} VSS_Num_Dlg;

typedef struct _tag_BITMAP {
	HDC hdc_mem;
	HBITMAP h_bitmap;
	char* str;
} SW_BITMAP, * P_SW_BITMAP;

typedef struct _tag_STATE_DATA {
	LL* p_sw_list;
	P_SW_BITMAP p_bitmaps;
	int num_bitmaps;
	int src_bitmap_pos[14];
	HFONT h_font_title;
	HFONT h_font_text;
} STATE_DATA, * P_STATE_DATA;

struct spec {
	char url[200];
	char num[14];
	struct variant *(*parse)(char* buf, int* num_var);
};

#endif