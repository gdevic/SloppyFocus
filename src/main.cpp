// SloppyFocus — Windows tray utility for focus-follows-mouse without raise.

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <algorithm>

#include "spi.hpp"
#include "settings.hpp"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' "                       \
                        "name='Microsoft.Windows.Common-Controls' "                  \
                        "version='6.0.0.0' processorArchitecture='*' "               \
                        "publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

constexpr UINT     WM_TRAYCALLBACK = (WM_APP + 1U);
constexpr UINT     TRAY_UID        = 1U;
constexpr unsigned DEFAULT_DELAY   = 200U;
constexpr int      MAX_DELAY_TICKS = 20;     // 20 notches × 100 ms = 2000 ms
constexpr unsigned MS_PER_TICK     = 100U;

constexpr wchar_t kClassName[]   = L"SloppyFocus.MsgWnd";
constexpr wchar_t kMutexName[]   = L"Local\\SloppyFocus.singleton";
constexpr wchar_t kShowMsgName[] = L"SloppyFocus.ShowSettings";
constexpr wchar_t kTaskbarMsg[]  = L"TaskbarCreated";

HINSTANCE g_inst          = nullptr;
HWND      g_msg_wnd       = nullptr;
HWND      g_settings_wnd  = nullptr;
UINT      g_show_msg      = 0U;
UINT      g_taskbar_msg   = 0U;
HANDLE    g_singleton     = nullptr;

HICON load_app_icon(int cx, int cy)
{
    HICON h = HICON(LoadImageW(g_inst, MAKEINTRESOURCEW(IDI_TRAYICON),
                               IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR));
    if (h == nullptr) h = LoadIconW(nullptr, IDI_APPLICATION);
    return h;
}

void format_tip(wchar_t* dst)
{
    bool const on = spi::get_tracking().value_or(false);
    wsprintfW(dst, L"SloppyFocus \x2014 Focus follows mouse: %s",
              ((on) ? L"ON" : L"OFF"));
}

// NIF_SHOWTIP is required alongside NIF_TIP under NOTIFYICON_VERSION_4 — the
// new version suppresses the standard hover tooltip by default to let apps
// draw their own popup UI instead.
void tray_add(HWND hwnd)
{
    NOTIFYICONDATAW nid{};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = hwnd;
    nid.uID              = TRAY_UID;
    nid.uFlags           = (NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP);
    nid.uCallbackMessage = WM_TRAYCALLBACK;
    nid.hIcon            = load_app_icon(GetSystemMetrics(SM_CXSMICON),
                                         GetSystemMetrics(SM_CYSMICON));
    format_tip(nid.szTip);
    Shell_NotifyIconW(NIM_ADD, &nid);
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

void tray_update_tip(HWND hwnd)
{
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = hwnd;
    nid.uID    = TRAY_UID;
    nid.uFlags = (NIF_TIP | NIF_SHOWTIP);
    format_tip(nid.szTip);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void tray_remove(HWND hwnd)
{
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = hwnd;
    nid.uID    = TRAY_UID;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void show_tray_menu(HWND hwnd)
{
    POINT pt{};
    GetCursorPos(&pt);
    HMENU const root = LoadMenuW(g_inst, MAKEINTRESOURCEW(IDR_TRAYMENU));
    if (root == nullptr) return;
    HMENU const sub = GetSubMenu(root, 0);

    bool const tracking  = spi::get_tracking().value_or(false);
    bool const autostart = settings::autostart_get();
    CheckMenuItem(sub, IDM_TRAY_TOGGLE,
                  MF_BYCOMMAND | ((tracking)  ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(sub, IDM_TRAY_AUTOSTART,
                  MF_BYCOMMAND | ((autostart) ? MF_CHECKED : MF_UNCHECKED));
    SetMenuDefaultItem(sub, IDM_TRAY_TOGGLE, FALSE);

    // Required so the popup dismisses when the user clicks elsewhere.
    SetForegroundWindow(hwnd);
    TrackPopupMenu(sub, (TPM_RIGHTBUTTON | TPM_BOTTOMALIGN),
                   pt.x, pt.y, 0, hwnd, nullptr);
    PostMessageW(hwnd, WM_NULL, 0, 0);
    DestroyMenu(root);
}

void update_timeout_label(HWND dlg, int ms)
{
    wchar_t buf[16];
    wsprintfW(buf, L"%d", ms);
    SetDlgItemTextW(dlg, IDC_LBL_TIMEOUT, buf);
}

void apply_dialog(HWND dlg)
{
    bool const enabled    = (IsDlgButtonChecked(dlg, IDC_CHK_ENABLED)         == BST_CHECKED);
    bool const raise      = (IsDlgButtonChecked(dlg, IDC_CHK_RAISE)           == BST_CHECKED);
    bool const autostart  = (IsDlgButtonChecked(dlg, IDC_CHK_AUTOSTART)       == BST_CHECKED);
    bool const offOnExit  = (IsDlgButtonChecked(dlg, IDC_CHK_DISABLE_ON_EXIT) == BST_CHECKED);
    LRESULT const pos     = SendDlgItemMessageW(dlg, IDC_TRK_TIMEOUT, TBM_GETPOS, 0, 0);
    unsigned const ms     = (unsigned(std::clamp(int(pos), 0, MAX_DELAY_TICKS)) * MS_PER_TICK);

    spi::set_timeout(ms);
    spi::set_zorder(raise);
    spi::set_tracking(enabled);
    settings::autostart_set(autostart);
    settings::disable_on_exit_set(offOnExit);
    if (g_msg_wnd != nullptr) tray_update_tip(g_msg_wnd);
}

INT_PTR CALLBACK settings_proc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            SendMessageW(dlg, WM_SETICON, ICON_SMALL,
                         LPARAM(load_app_icon(GetSystemMetrics(SM_CXSMICON),
                                              GetSystemMetrics(SM_CYSMICON))));
            SendMessageW(dlg, WM_SETICON, ICON_BIG,
                         LPARAM(load_app_icon(GetSystemMetrics(SM_CXICON),
                                              GetSystemMetrics(SM_CYICON))));

            bool     const tracking = spi::get_tracking().value_or(false);
            bool     const zorder   = spi::get_zorder().value_or(false);
            unsigned const tmout    = spi::get_timeout().value_or(DEFAULT_DELAY);

            CheckDlgButton(dlg, IDC_CHK_ENABLED,
                           (tracking) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(dlg, IDC_CHK_RAISE,
                           (zorder)   ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(dlg, IDC_CHK_AUTOSTART,
                           (settings::autostart_get()) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(dlg, IDC_CHK_DISABLE_ON_EXIT,
                           (settings::disable_on_exit_get()) ? BST_CHECKED : BST_UNCHECKED);

            HWND const trk  = GetDlgItem(dlg, IDC_TRK_TIMEOUT);
            unsigned const tickU = ((tmout + (MS_PER_TICK / 2U)) / MS_PER_TICK);
            int      const tick  = std::clamp(int(tickU), 0, MAX_DELAY_TICKS);
            SendMessageW(trk, TBM_SETRANGE,    TRUE, MAKELPARAM(0, MAX_DELAY_TICKS));
            SendMessageW(trk, TBM_SETTICFREQ,  1,    0);
            SendMessageW(trk, TBM_SETPAGESIZE, 0,    2);
            SendMessageW(trk, TBM_SETPOS,      TRUE, LPARAM(tick));
            update_timeout_label(dlg, (tick * int(MS_PER_TICK)));
            return TRUE;
        }
        case WM_HSCROLL:
        {
            HWND const trk = GetDlgItem(dlg, IDC_TRK_TIMEOUT);
            if (HWND(lp) == trk)
            {
                LRESULT const pos = SendMessageW(trk, TBM_GETPOS, 0, 0);
                update_timeout_label(dlg, (int(pos) * int(MS_PER_TICK)));
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wp))
            {
                case IDOK:
                    apply_dialog(dlg);
                    DestroyWindow(dlg);
                    g_settings_wnd = nullptr;
                    return TRUE;
                case IDCANCEL:
                    DestroyWindow(dlg);
                    g_settings_wnd = nullptr;
                    return TRUE;
                case IDC_BTN_APPLY:
                    apply_dialog(dlg);
                    return TRUE;
                default: break;
            }
            return FALSE;
        }
        case WM_CLOSE:
            DestroyWindow(dlg);
            g_settings_wnd = nullptr;
            return TRUE;
        default: break;
    }
    return FALSE;
}

void show_settings(HWND owner)
{
    if (g_settings_wnd != nullptr)
    {
        SetForegroundWindow(g_settings_wnd);
        return;
    }
    g_settings_wnd = CreateDialogParamW(g_inst, MAKEINTRESOURCEW(IDD_SETTINGS),
                                        owner, settings_proc, 0);
    if (g_settings_wnd != nullptr)
    {
        ShowWindow(g_settings_wnd, SW_SHOW);
        SetForegroundWindow(g_settings_wnd);
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // Re-add the icon if explorer.exe restarts.
    if ((g_taskbar_msg != 0U) && (msg == g_taskbar_msg))
    {
        tray_add(hwnd);
        return 0;
    }
    // Wake-up message from a second invocation.
    if ((g_show_msg != 0U) && (msg == g_show_msg))
    {
        show_settings(hwnd);
        return 0;
    }

    switch (msg)
    {
        case WM_TRAYCALLBACK:
        {
            UINT const event = LOWORD(lp);
            if ((event == WM_LBUTTONUP) || (event == NIN_SELECT))
            {
                bool const cur = spi::get_tracking().value_or(false);
                spi::set_tracking(!cur);
                tray_update_tip(hwnd);
            }
            else if ((event == WM_CONTEXTMENU) || (event == WM_RBUTTONUP))
            {
                show_tray_menu(hwnd);
            }
            return 0;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wp))
            {
                case IDM_TRAY_TOGGLE:
                {
                    bool const cur = spi::get_tracking().value_or(false);
                    spi::set_tracking(!cur);
                    tray_update_tip(hwnd);
                    return 0;
                }
                case IDM_TRAY_SETTINGS:
                    show_settings(hwnd);
                    return 0;
                case IDM_TRAY_AUTOSTART:
                    settings::autostart_set(!settings::autostart_get());
                    return 0;
                case IDM_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    return 0;
                default: break;
            }
            return 0;
        }
        case WM_DESTROY:
            if (settings::disable_on_exit_get())
                spi::set_tracking(false);
            tray_remove(hwnd);
            PostQuitMessage(0);
            return 0;
        default: break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool register_class()
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = g_inst;
    wc.lpszClassName = kClassName;
    wc.hIcon         = load_app_icon(GetSystemMetrics(SM_CXICON),
                                     GetSystemMetrics(SM_CYICON));
    wc.hIconSm       = load_app_icon(GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON));
    return (RegisterClassExW(&wc) != 0);
}

} // namespace

int APIENTRY wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int)
{
    g_inst = inst;

    INITCOMMONCONTROLSEX icc{ sizeof(icc), (ICC_BAR_CLASSES | ICC_STANDARD_CLASSES) };
    InitCommonControlsEx(&icc);

    g_show_msg    = RegisterWindowMessageW(kShowMsgName);
    g_taskbar_msg = RegisterWindowMessageW(kTaskbarMsg);

    g_singleton = CreateMutexW(nullptr, TRUE, kMutexName);
    if ((g_singleton != nullptr) && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        if (g_show_msg != 0U)
            PostMessageW(HWND_BROADCAST, g_show_msg, 0, 0);
        CloseHandle(g_singleton);
        return 0;
    }

    if (!register_class()) return 1;

    g_msg_wnd = CreateWindowExW(0, kClassName, L"SloppyFocus",
                                0, 0, 0, 0, 0,
                                HWND_MESSAGE, nullptr, inst, nullptr);
    if (g_msg_wnd == nullptr) return 1;

    tray_add(g_msg_wnd);

    MSG m{};
    while ((GetMessageW(&m, nullptr, 0, 0)) > 0)
    {
        if ((g_settings_wnd == nullptr) || (IsDialogMessageW(g_settings_wnd, &m) == FALSE))
        {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }

    if (g_singleton != nullptr) CloseHandle(g_singleton);
    return int(m.wParam);
}
