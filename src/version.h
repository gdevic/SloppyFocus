#pragma once

// Single source of truth for the app version. Bump these on each release.
// Both .rc (VERSIONINFO) and .cpp (About box) include this header.

#define APP_NAME            "SloppyFocus"

#define APP_VERSION_MAJOR   1
#define APP_VERSION_MINOR   0
#define APP_VERSION_BUILD   0

#define APP_VERSION_STR     "1.0.0"
#define APP_VERSION_STR4    "1.0.0.0"

#define APP_COPYRIGHT       "Copyright (c) 2026 Baltazar Studios, LLC. CC BY-NC-SA 4.0."
#define APP_RELEASES_URL    "https://github.com/gdevic/SloppyFocus/releases"

// Widen-string helper so .cpp can reuse the narrow string macros above as
// wide-character literals: WIDEN(APP_NAME) -> L"SloppyFocus".
#define WIDEN_(s) L ## s
#define WIDEN(s)  WIDEN_(s)
