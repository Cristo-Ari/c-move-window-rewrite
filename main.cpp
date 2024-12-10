#include <windows.h>

WNDPROC originalWndProc = nullptr;
POINT screenCenter;
POINT mouseOffset;
bool isDragging = false;

LRESULT CALLBACK MirroredWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCHITTEST: {
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);

        if (pt.x >= windowRect.left && pt.x < windowRect.left + 10) {
            if (pt.y >= windowRect.top && pt.y < windowRect.top + 10) return HTTOPLEFT;
            if (pt.y <= windowRect.bottom && pt.y > windowRect.bottom - 10) return HTBOTTOMLEFT;
        }
        if (pt.x <= windowRect.right && pt.x > windowRect.right - 10) {
            if (pt.y >= windowRect.top && pt.y < windowRect.top + 10) return HTTOPRIGHT;
            if (pt.y <= windowRect.bottom && pt.y > windowRect.bottom - 10) return HTBOTTOMRIGHT;
        }

        if (pt.x >= windowRect.left + 10 && pt.x <= windowRect.right - 10) {
            if (pt.y <= windowRect.top + 10) return HTTOP;
            if (pt.y >= windowRect.bottom - 10) return HTBOTTOM;
        }

        if (pt.y >= windowRect.top + 10 && pt.y <= windowRect.bottom - 10) {
            if (pt.x <= windowRect.left + 10) return HTLEFT;
            if (pt.x >= windowRect.right - 10) return HTRIGHT;
        }

        return HTCAPTION;
    }

    case WM_NCLBUTTONDOWN: {
        if (wParam == HTCAPTION || wParam == HTTOP || wParam == HTBOTTOM || wParam == HTLEFT || wParam == HTRIGHT) {
            SetCapture(hwnd);
            mouseOffset.x = LOWORD(lParam);
            mouseOffset.y = HIWORD(lParam);
            isDragging = true;
            return 0;
        }
        break;
    }

    case WM_NCLBUTTONUP: {
        if (isDragging) {
            ReleaseCapture();
            isDragging = false;
            return 0;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        if (isDragging) {
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);

            int mouseX = LOWORD(lParam);
            int mouseY = HIWORD(lParam);

            // Вычисляем зеркальную позицию относительно центра экрана
            int mirroredX = screenCenter.x - (mouseX - screenCenter.x) - (windowRect.right - windowRect.left);
            int mirroredY = screenCenter.y - (mouseY - screenCenter.y) - (windowRect.bottom - windowRect.top);

            // Получаем размеры экрана
            RECT screenRect;
            GetWindowRect(GetDesktopWindow(), &screenRect);

            // Ограничиваем зеркальную позицию так, чтобы окно не выходило за пределы экрана
            if (mirroredX < screenRect.left) mirroredX = screenRect.left;
            if (mirroredY < screenRect.top) mirroredY = screenRect.top;
            if (mirroredX + (windowRect.right - windowRect.left) > screenRect.right) mirroredX = screenRect.right - (windowRect.right - windowRect.left);
            if (mirroredY + (windowRect.bottom - windowRect.top) > screenRect.bottom) mirroredY = screenRect.bottom - (windowRect.bottom - windowRect.top);

            // Перемещаем окно в зеркальное положение
            SetWindowPos(hwnd, nullptr, mirroredX, mirroredY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return 0;
        }
        break;
    }

    case WM_SYSCOMMAND: {
        if (wParam == SC_CLOSE) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    }

    case WM_CLOSE: {
        DestroyWindow(hwnd);
        break;
    }

    default:
        return CallWindowProc(originalWndProc, hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    RECT screenRect;
    GetWindowRect(GetDesktopWindow(), &screenRect);
    screenCenter.x = (screenRect.right - screenRect.left) / 2;
    screenCenter.y = (screenRect.bottom - screenRect.top) / 2;

    const wchar_t CLASS_NAME[] = L"MirroredWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Mirrored Window",
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        return -1;
    }

    originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MirroredWindowProc)));

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
