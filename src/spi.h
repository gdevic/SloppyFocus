#pragma once

#include <windows.h>
#include <algorithm>
#include <optional>

namespace spi {

// SPIF_UPDATEINIFILE persists to registry. SPIF_SENDCHANGE is intentionally
// omitted: it would broadcast WM_SETTINGCHANGE via SendMessageTimeout to every
// top-level window and stall the UI by ~1 s on busy desktops; the SPI value
// itself takes effect immediately without the broadcast.
inline constexpr UINT kFlags = SPIF_UPDATEINIFILE;
inline constexpr UINT kMaxTimeoutMs = 2000U;

inline bool set_tracking(bool on) noexcept
{
    PVOID const v = PVOID(UINT_PTR((on) ? 1U : 0U));
    return (SystemParametersInfoW(SPI_SETACTIVEWINDOWTRACKING, 0U, v, kFlags) != FALSE);
}

inline bool set_timeout(unsigned ms) noexcept
{
    UINT const clamped = std::clamp(UINT(ms), UINT(0), kMaxTimeoutMs);
    PVOID const v = PVOID(UINT_PTR(clamped));
    return (SystemParametersInfoW(SPI_SETACTIVEWNDTRKTIMEOUT, 0U, v, kFlags) != FALSE);
}

inline bool set_zorder(bool raise) noexcept
{
    PVOID const v = PVOID(UINT_PTR((raise) ? 1U : 0U));
    return (SystemParametersInfoW(SPI_SETACTIVEWNDTRKZORDER, 0U, v, kFlags) != FALSE);
}

inline std::optional<bool> get_tracking() noexcept
{
    BOOL out = FALSE;
    if ((SystemParametersInfoW(SPI_GETACTIVEWINDOWTRACKING, 0U, &out, 0U)) == FALSE)
        return std::nullopt;
    return (out != FALSE);
}

inline std::optional<unsigned> get_timeout() noexcept
{
    DWORD out = 0U;
    if ((SystemParametersInfoW(SPI_GETACTIVEWNDTRKTIMEOUT, 0U, &out, 0U)) == FALSE)
        return std::nullopt;
    return unsigned(out);
}

inline std::optional<bool> get_zorder() noexcept
{
    BOOL out = FALSE;
    if ((SystemParametersInfoW(SPI_GETACTIVEWNDTRKZORDER, 0U, &out, 0U)) == FALSE)
        return std::nullopt;
    return (out != FALSE);
}

} // namespace spi
