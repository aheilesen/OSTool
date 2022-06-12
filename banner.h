#ifndef BANNER_H_
#define BANNER_H_

#include <Windows.h>
#include "ost_data.h"
#include "ost_shared.h"

// These are included so the parse function pointer for the
// struct spec variable can be set in banner.c
#include "parse_order.h"
#include "parse_vss.h"

LRESULT CALLBACK bannerProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);
void drawTitle(HDC hdc, P_SW_BITMAP p_bitmap_truck,
               P_SW_BITMAP p_bitmap_title);
int checkInput(const char* input, size_t length);
void drawArrowButton(HDC hdc_btn, HDC hdc_mem,
                     const struct button_state* p_state, BOOL f_enabled);
void drawEditRect(HWND hwnd, HDC hdc, HDC hdc_mem, LPRECT p_rect,
                  BOOL f_in_edit);
int getNumFromEdit(HWND hwnd_edit, char* buf_vss, unsigned buf_size);
int genVssURL(const char* src, char* dest, int dest_size);
int genOrderURL(const char* src, char* dest, int dest_size);
LRESULT CALLBACK vssEditProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam);

#endif