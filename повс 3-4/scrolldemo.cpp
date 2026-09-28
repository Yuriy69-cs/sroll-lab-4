#include <windows.h>
#include <stdlib.h>

#define VIRTUAL_W   3000
#define VIRTUAL_H   3000
#define MAX_RECTS   300
#define LINE_STEP   50

struct RectItem
{
    RECT     rc;
    COLORREF color;
    int      number;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"ScrollWndClass";

    if (!RegisterClass(&wc))
        return 0;

    HWND hWnd = CreateWindowEx(
        0,
        L"ScrollWndClass",
        L"Прокрутка виртуальной области рисования",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInst, NULL);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, nCmdShow);
  
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static RectItem rects[MAX_RECTS];
    static int      rectCount = 0;
    static int      scrollX = 0;
    static int      scrollY = 0;

    switch (msg)
    {
    case WM_CREATE:
    {
        srand((unsigned)GetTickCount());

        rectCount = MAX_RECTS;
        for (int i = 0; i < rectCount; i++)
        {
            int w = 40 + rand() % 140;
            int h = 30 + rand() % 120;
            int x = rand() % (VIRTUAL_W - w);
            int y = rand() % (VIRTUAL_H - h);

            rects[i].rc.left = x;
            rects[i].rc.top = y;
            rects[i].rc.right = x + w;
            rects[i].rc.bottom = y + h;

            if (rand() % 2)
                rects[i].color = RGB(240, 240, 130);
            else
                rects[i].color = RGB(220, 220, 245);

            rects[i].number = rand() % 999 + 1;
        }

        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = VIRTUAL_W - 1;
        si.nPage = 100;
        si.nPos = 0;
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

        si.nMax = VIRTUAL_H - 1;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

        return 0;
    }

    case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE;

        si.nPage = rc.right;
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

        si.nPage = rc.bottom;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

        return 0;
    }

    case WM_VSCROLL:
    case WM_HSCROLL:
    {
        int nBar = (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ;

        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, nBar, &si);

        int oldPos = si.nPos;

        switch (LOWORD(wParam))
        {
        case SB_TOP:            si.nPos = si.nMin;         break;
        case SB_BOTTOM:         si.nPos = si.nMax;         break;
        case SB_LINEUP:         si.nPos -= LINE_STEP;      break;
        case SB_LINEDOWN:       si.nPos += LINE_STEP;      break;
        case SB_PAGEUP:         si.nPos -= (int)si.nPage;  break;
        case SB_PAGEDOWN:       si.nPos += (int)si.nPage;  break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:  si.nPos = si.nTrackPos;    break;
        }

        if (si.nPos < si.nMin) si.nPos = si.nMin;
        int maxPos = si.nMax - (int)si.nPage + 1;
        if (maxPos < si.nMin) maxPos = si.nMin;
        if (si.nPos > maxPos) si.nPos = maxPos;

        if (si.nPos != oldPos)
        {
            int dx = 0, dy = 0;
            if (nBar == SB_HORZ) { dx = oldPos - si.nPos; scrollX = si.nPos; }
            else { dy = oldPos - si.nPos; scrollY = si.nPos; }

            SetScrollPos(hWnd, nBar, si.nPos, TRUE);

            ScrollWindowEx(hWnd, dx, dy, NULL, NULL, NULL, NULL,
                SW_INVALIDATE | SW_ERASE);
        }
        return 0;
    }

    case WM_KEYDOWN:
    {
        SCROLLINFO siH, siV;
        siH.cbSize = sizeof(siH); siH.fMask = SIF_ALL;
        siV.cbSize = sizeof(siV); siV.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_HORZ, &siH);
        GetScrollInfo(hWnd, SB_VERT, &siV);

        int nBar = 0;
        int delta = 0;

        switch (wParam)
        {
        case VK_LEFT:   nBar = SB_HORZ; delta = -LINE_STEP; break;
        case VK_RIGHT:  nBar = SB_HORZ; delta = LINE_STEP; break;
        case VK_UP:     nBar = SB_VERT; delta = -LINE_STEP; break;
        case VK_DOWN:   nBar = SB_VERT; delta = LINE_STEP; break;
        case VK_PRIOR:  nBar = SB_VERT; delta = -(int)siV.nPage; break;
        case VK_NEXT:   nBar = SB_VERT; delta = (int)siV.nPage; break;
        case VK_HOME:   nBar = SB_VERT; delta = -siV.nPos; break;
        case VK_END:
            nBar = SB_VERT;
            delta = siV.nMax - (int)siV.nPage - siV.nPos + 1;
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        SCROLLINFO* psi = (nBar == SB_HORZ) ? &siH : &siV;

        int newPos = psi->nPos + delta;
        if (newPos < psi->nMin) newPos = psi->nMin;
        int maxPos = psi->nMax - (int)psi->nPage + 1;
        if (maxPos < psi->nMin) maxPos = psi->nMin;
        if (newPos > maxPos) newPos = maxPos;

        int shift = psi->nPos - newPos;
        if (shift != 0)
        {
            if (nBar == SB_HORZ) scrollX = newPos;
            else                 scrollY = newPos;

            SetScrollPos(hWnd, nBar, newPos, TRUE);

            ScrollWindowEx(hWnd,
                (nBar == SB_HORZ) ? shift : 0,
                (nBar == SB_VERT) ? shift : 0,
                NULL, NULL, NULL, NULL,
                SW_INVALIDATE | SW_ERASE);
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);

        RECT rcClip = ps.rcPaint;

        SetBkMode(hDC, TRANSPARENT);
        SetTextAlign(hDC, TA_CENTER | TA_BASELINE);

        for (int i = 0; i < rectCount; i++)
        {
            RECT rcScreen;
            rcScreen.left = rects[i].rc.left - scrollX;
            rcScreen.top = rects[i].rc.top - scrollY;
            rcScreen.right = rects[i].rc.right - scrollX;
            rcScreen.bottom = rects[i].rc.bottom - scrollY;

            RECT rcInter;
            if (!IntersectRect(&rcInter, &rcScreen, &rcClip))
                continue;

            HBRUSH hBrsh = ::CreateSolidBrush(rects[i].color);
            ::FillRect(hDC, &rcScreen, hBrsh);
            ::DeleteObject(hBrsh);

            HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            HPEN oPen = (HPEN)::SelectObject(hDC, hPen);
            HGDIOBJ oBrsh = ::SelectObject(hDC, GetStockObject(NULL_BRUSH));

            ::Rectangle(hDC, rcScreen.left, rcScreen.top,
                rcScreen.right, rcScreen.bottom);

            ::SelectObject(hDC, oPen);
            ::SelectObject(hDC, oBrsh);
            ::DeleteObject(hPen);

            char szNum[16];
            wsprintfA(szNum, "%d", rects[i].number);

            int cx = (rcScreen.left + rcScreen.right) / 2;
            int cy = (rcScreen.top + rcScreen.bottom) / 2;

            TextOutA(hDC, cx, cy, szNum, lstrlenA(szNum));
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}