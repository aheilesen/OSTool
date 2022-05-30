#include <Windows.h>

#define NUM_SWITCHES 30
#define SCALE_FACTOR 10

void drawLogicalDash(HDC hdc, HWND hwnd, int x_client, int y_client)
{
	static POINT pt[30] = { 4,8, 6,8, 9,8, 11,8,
		15,8, 17,8, 19,8, 21,8, 23,8, 25,8,
		29,4, 31,4, 33,4, 35,4, 37,4,
		29,6, 31,6, 33,6, 35,6, 37,6,
		41,4, 43,4, 46,4, 49,4, 51,4,
		41,6, 43,6, 46,6, 49,6, 51,6
	};

	PAINTSTRUCT ps;
	HPEN hpen;

	int i;
	int radius = x_client / (56 * 6);		// Rounded rectangle corner radius

	hdc = BeginPaint(hwnd, &ps);

	SetMapMode(hdc, MM_ISOTROPIC);
	SetViewportOrgEx(hdc, x_client / 4, y_client / 4, NULL);
	SetWindowExtEx(hdc, 56 * SCALE_FACTOR, 10 * SCALE_FACTOR, NULL);
	SetViewportExtEx(hdc, x_client / 2, y_client / 2, NULL);

	SelectObject(hdc, (HPEN)CreatePen(PS_SOLID, 1, RGB(180, 180, 180)));

	for (i = 0; i < NUM_SWITCHES; i++) {
		RoundRect(hdc,
			pt[i].x * SCALE_FACTOR + 2,
			pt[i].y * SCALE_FACTOR * 2 + 2,
			(pt[i].x + 2) * SCALE_FACTOR - 2,
			(pt[i].y + 2) * SCALE_FACTOR * 2 - 2,
			radius, radius);
	}

	hpen = (HPEN)SelectObject(hdc, GetStockObject(BLACK_PEN));
	EndPaint(hwnd, &ps);
}