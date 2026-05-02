#pragma once

#include <windows.h>
#include <string>

namespace settings {

inline constexpr wchar_t kAppName[]      = L"SloppyFocus";
inline constexpr wchar_t kRunKey[]       = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr wchar_t kAppKey[]       = L"Software\\SloppyFocus";
inline constexpr wchar_t kDisableExit[]  = L"DisableOnExit";

inline std::wstring exe_path()
{
    wchar_t buf[MAX_PATH] = {};
    DWORD const n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::wstring(buf, (n > 0U) ? n : 0U);
}

inline bool autostart_get()
{
    HKEY hk{};
    if ((RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &hk)) != ERROR_SUCCESS)
        return false;
    DWORD type = 0U;
    DWORD sz   = 0U;
    LONG const r = RegQueryValueExW(hk, kAppName, nullptr, &type, nullptr, &sz);
    RegCloseKey(hk);
    return ((r == ERROR_SUCCESS) && ((type == REG_SZ) || (type == REG_EXPAND_SZ)));
}

inline bool autostart_set(bool on)
{
    HKEY hk{};
    if ((RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &hk)) != ERROR_SUCCESS)
        return false;
    LONG r = ERROR_SUCCESS;
    if (on)
    {
        std::wstring const path = (L"\"" + exe_path()) + L"\"";
        DWORD const bytes = DWORD((path.size() + 1U) * sizeof(wchar_t));
        r = RegSetValueExW(hk, kAppName, 0U, REG_SZ,
                           reinterpret_cast<BYTE const*>(path.c_str()), bytes);
    }
    else
    {
        r = RegDeleteValueW(hk, kAppName);
        if (r == ERROR_FILE_NOT_FOUND) r = ERROR_SUCCESS;
    }
    RegCloseKey(hk);
    return (r == ERROR_SUCCESS);
}

inline bool disable_on_exit_get()
{
    HKEY hk{};
    if ((RegOpenKeyExW(HKEY_CURRENT_USER, kAppKey, 0, KEY_QUERY_VALUE, &hk)) != ERROR_SUCCESS)
        return false;
    DWORD val  = 0U;
    DWORD sz   = sizeof(val);
    DWORD type = 0U;
    LONG const r = RegQueryValueExW(hk, kDisableExit, nullptr, &type,
                                    reinterpret_cast<BYTE*>(&val), &sz);
    RegCloseKey(hk);
    return ((r == ERROR_SUCCESS) && (type == REG_DWORD) && (val != 0U));
}

inline bool disable_on_exit_set(bool on)
{
    HKEY hk{};
    DWORD disp = 0U;
    if ((RegCreateKeyExW(HKEY_CURRENT_USER, kAppKey, 0, nullptr, 0,
                         KEY_SET_VALUE, nullptr, &hk, &disp)) != ERROR_SUCCESS)
        return false;
    DWORD const val = (on) ? 1U : 0U;
    LONG const r = RegSetValueExW(hk, kDisableExit, 0U, REG_DWORD,
                                  reinterpret_cast<BYTE const*>(&val), sizeof(val));
    RegCloseKey(hk);
    return (r == ERROR_SUCCESS);
}

} // namespace settings
