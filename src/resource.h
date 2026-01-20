#ifndef RESOURCE_H
#define RESOURCE_H

// Application info
#define APP_NAME            L"MilleWin CF Extractor"
#define APP_VERSION         L"1.2.1"
#define APP_CLASS_NAME      L"MWCFExtractorClass"

// Icon IDs
#define IDI_APP_ICON        101
#define IDI_TRAY_ACTIVE     102
#define IDI_TRAY_INACTIVE   103

// Dialog IDs
#define IDD_HOTKEY_CONFIG   200

// Control IDs
#define IDC_HOTKEY_CAPTURE  1001
#define IDC_STATIC_CURRENT  1003
#define IDC_STATIC_ERROR    1004
#define IDC_BTN_RETRY       1005
#define IDC_BTN_CANCEL      1006

// Tray menu IDs
#define IDM_TRAY_SAVE_HOTKEY    2001
#define IDM_TRAY_CHANGE_HOTKEY  2002
#define IDM_TRAY_AUTOSTART      2003
#define IDM_TRAY_DESKTOP_ICON   2004
#define IDM_TRAY_CHECK_UPDATES  2005
#define IDM_TRAY_ABOUT          2006
#define IDM_TRAY_EXIT           2007

// Legacy (per compatibilitÃ )
#define IDM_TRAY_CONFIGURE  IDM_TRAY_CHANGE_HOTKEY

// Timer IDs
#define IDT_MSGBOX_CLOSE    3001
#define IDT_NOTIFICATION    3002

// Hotkey ID
#define HOTKEY_ID           4001

// Custom messages
#define WM_TRAYICON             (WM_USER + 1)
#define WM_HOTKEY_CHANGED       (WM_USER + 2)
#define WM_UPDATE_CHECK_DONE    (WM_USER + 3)

// Timeout values (milliseconds)
#define MSGBOX_TIMEOUT_MS   3000
#define NOTIFY_TIMEOUT_MS   3000
#define OVERLAY_TIMEOUT_MS  3000

#endif // RESOURCE_H
