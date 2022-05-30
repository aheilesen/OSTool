#ifndef OSTOOL_H_
#define OSTOOL_H_

#include "parse_vss.h"
#include "parse_switch.h"
#include "banner.h"
#include "list_view.h"
#include "ost_data.h"
#include "ost_shared.h"
#include "cab_view.h"

// Main window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);

// Dialog box procedure
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int createChildWindows(HINSTANCE h_instance, HWND hwnd_parent,
                       HWND* p_hwnd_banner, HWND* p_hwnd_list_view,
                       HWND* p_hwnd_cab_view, P_STATE_DATA p_state_data);

HFONT createVSSFont(HWND hwnd, char* font_name, int height);
int loadNovumFont(HINSTANCE h_instance, char* res_name, char* res_type);

void centerDialog(HWND hDlg);

BOOL getFileInfo(HWND hwnd, OPENFILENAMEA* pOpenFile, char* pFilePath, int pathLength);
void freeMemory(Variant** pVariant, LL** pSwitchList);
int getHighlightPos(LL* sw_list, int index);
static void notifyConflicts(LL* sw_list);
static void notifyPanel(LL* sw_list, unsigned panel);

// Bitmaps
int loadBitmaps(HINSTANCE hInstance, P_SW_BITMAP p_sw_bitmap, int num_bitmaps);
void destroyBitmaps(P_SW_BITMAP p_sw_bitmap, int num_bitmaps);
int createMemoryDCs(HDC hdc, P_SW_BITMAP p_sw_bitmap, int num_dcs);
void deleteMemoryDCs(P_SW_BITMAP p_sw_bitmap, int num_dcs);
int selectBitmaps(P_SW_BITMAP p_sw_bitmap, int num_bitmaps);
int getSwPanel(struct variant* var_list, int num_var);
void drawTitle(HDC hdc, P_SW_BITMAP p_bitmap_truck,
	       P_SW_BITMAP p_bitmap_title);
void clearSrcBitmapPos(int* p_src_bitmap_pos);
int getSrcBitmapPos(LL* sw_list, int* p_src_bitmap_pos);

#endif