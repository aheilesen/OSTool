#ifndef LIST_VIEW_H_
#define LIST_VIEW_H_

#include <Windows.h>
#include "ost_data.h"
#include "andrewll.h"

LRESULT CALLBACK listViewProc(HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam);
HWND createListBox(HWND hwnd_parent);
void printListBoxHeader(HDC hdc, HFONT h_font);
int populateListBox(HWND hwnd, LL* p_sw_list);
void getSWDesc(char* desc, int desc_size, int pn);
void setLbItemFlags(HWND hwnd_list, P_STATE_DATA p_data);
#endif