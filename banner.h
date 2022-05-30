#ifndef BANNER_H_
#define BANNER_H_

#include <Windows.h>
#include "ost_data.h"
#include "ost_shared.h"

LRESULT CALLBACK bannerProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);
void drawTitle(HDC hdc, P_SW_BITMAP p_bitmap_truck,
               P_SW_BITMAP p_bitmap_title);
int checkInput(const char* input, size_t length);
void drawArrowButton(HDC hdc_btn, HDC hdc_mem,
                     const struct button_state* p_state, BOOL f_enabled);
void drawEditRect(HWND hwnd, HDC hdc, HDC hdc_mem, LPRECT p_rect,
                  BOOL f_in_edit);
int getVssFromEdit(HWND hwnd_edit, char* buf_vss, unsigned buf_size);
int genVssURL(char* dest, const char* src, int dest_size);
LRESULT CALLBACK vssEditProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam);

#endif