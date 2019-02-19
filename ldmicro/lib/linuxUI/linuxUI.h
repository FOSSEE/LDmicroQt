#ifndef __LINUX_UI__
#define __LINUX_UI__

/// includes
#include <gtk/gtk.h>
#include <QApplication>
#include <QWidget>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
// #include <QtGui>
// #include <QSize>
// #include "freezeLD.h"
// #include "linuxLD.h"
#include <linux/limits.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "linuxLD.h"

// 4000 ICON "ldmicro.ico"

/// version control
#define LDMicro_VERSION_MAJOR 1
#define LDMicro_VERSION_MINOR 0

/// Flags
/// message box
#define MB_OK              0x00000001L
#define MB_OKCANCEL        0x00000002L
#define MB_YESNO           0x00000004L
#define MB_YESNOCANCEL     0x00000008L

#define IDOK      1
#define IDCANCEL  2
#define IDYES     3
#define IDNO      4

#define MB_ICONERROR        0x00000010L
#define MB_ICONQUESTION     0x00000020L
#define MB_ICONWARNING      0x00000040L
#define MB_ICONINFORMATION  0x00000080L

/// Scroll
#define SB_LINEUP        0x00000001
#define SB_PAGEUP        0x00000002
#define SB_LINEDOWN      0x00000004
#define SB_PAGEDOWN      0x00000008
#define SB_TOP           0x00000010
#define SB_BOTTOM        0x00000020
#define SB_THUMBTRACK    0x00000040 
#define SB_THUMBPOSITION 0x00000080 

/// UART terminal flags
#define WM_GETTEXT     0x00000001
#define WM_SETTEXT     0x00000002
#define WM_SETTEXT_END 0x00000004

/// List view flags
#define LVN_ITEMACTIVATE 0x00000001
#define LVN_GETDISPINFO  0x00000002

/// Open/save file
#define OFN_PATHMUSTEXIST     0x00000001L
#define OFN_HIDEREADONLY      0x00000002L
#define OFN_OVERWRITEPROMPT   0x00000004L

/// PatBlt paint flags
#define PATINVERT 0x00000100L

/// Key masks
#define VK_TAB GDK_KEY_Tab

#define VK_DOWN  65364
#define VK_UP    65362
#define VK_LEFT  65361
#define VK_RIGHT 65363

#define VK_NP_DOWN  65433
#define VK_NP_UP    65431
#define VK_NP_LEFT  65430
#define VK_NP_RIGHT 65432

#define VK_RETURN  GDK_KEY_Return
#define VK_ESCAPE  GDK_KEY_Escape
#define VK_F5 GDK_KEY_F5
#define VK_F1 GDK_KEY_F1

#define VK_OEM_PLUS GDK_KEY_plus
#define VK_OEM_MINUS GDK_KEY_minus
#define VK_OEM_PERIOD GDK_KEY_period
#define VK_OEM_COMMA GDK_KEY_comma

#define VK_DELETE GDK_KEY_Delete 
#define VK_NP_DELETE GDK_KEY_KP_Delete

// #define VK_OEM_1 GDK_KEY_colon     // GDK_KEY_semicolon
// #define VK_OEM_2 GDK_KEY_question  // GDK_KEY_slash
// #define VK_OEM_5 GDK_KEY_backslash // GDK_KEY_bar

/// Window brushes
#define BS_SOLID       0x00000001L
#define BS_HOLLOW      0x00000002L
#define BLACK_BRUSH    0x00000004L
#define WHITE_BRUSH    0x00000008L 
#define GRAY_BRUSH     0x00000010L
#define LTGRAY_BRUSH   0x00000020L
#define DKGRAY_BRUSH   0x00000040L

extern const COLORREF BLACK_BR;
extern const COLORREF WHITE_BR;
extern const COLORREF GRAY_BR;
extern const COLORREF LTGRAY_BR;
extern const COLORREF DKGRAY_BR;

/// Font flags
#define FW_REGULAR   0x00000001L
#define FW_BOLD      0x00000002L

/// EnableMenuItem variables
extern const UINT MF_ENABLED;
extern const UINT MF_GRAYED;
extern const UINT MF_CHECKED;
extern const UINT MF_UNCHECKED;

/// Accelerators (keyboard shortcuts)
extern GtkAccelGroup* AccelGroup;
extern GClosure* closure;

/// ListStore
extern HWID view;
extern HTVC column;

/// Structures
typedef struct OpenFileInfoData {
    DWORD         lStructSize;
    HWID          *parentWindow;
    LPTSTR        lpstrFile;
    LPCTSTR       lpstrFilter;
    DWORD         nMaxFile;
    LPCTSTR       lpstrTitle;
    DWORD         Flags;
    LPCTSTR       lpstrDefExt;
} OPENFILENAME;

typedef struct TimerRecordTag {
    BOOL (*pfun)(BOOL);
    UINT  ufID;
    UINT  utID;
} TimerRecord;

/// Variables
extern COLORREF HdcCurrentTextColor;
extern std::vector<TimerRecord> timerRecords;
extern int GLOBAL_mouse_last_clicked_x;
extern int GLOBAL_mouse_last_clicked_y;
/// functions
BOOL GetFocus(HWID window);

COLORREF RGB(
    int red, 
    int green, 
    int blue);

int MessageBox(
    HWID  pWindow, 
    char* message, 
    char* title, 
    UINT  mFlags);

BOOL GetSaveFileName(OPENFILENAME *ofn);

BOOL GetOpenFileName(OPENFILENAME *ofn);

void EnableMenuItem(
    HMENU MenuName, 
    HMENU MenuItem, 
    UINT  CheckEnabledItem);

void CheckMenuItem(
    HMENU MenuName, 
    HMENU MenuItem, 
    UINT  Check);

HANDLE GetStockObject(int fnObject);

void SelectObject(
    QPainter* hcr, 
    HFONT hfont);

HBRUSH CreateBrushIndirect(PLOGBRUSH plb);

HFONT CreateFont(
    int     nHeight,
    int     nWidth,
    int     nOrientation,
    int     fnWeight,
    DWORD   fdwItalic,
    LPCTSTR lpszFace);

void SetBkColor(
    HWID     widget, 
    HCRDC    hcr,
    COLORREF bkCol);

void SetTextColor(
    HCRDC    hcr, 
    COLORREF color);

void TextOut(
    HWID    hWid,
    HCRDC   hcr,
    int     nXStart,
    int     nYStart,
    LPCTSTR lpString,
    int     cchString);

COLORREF GetTextColor(HCRDC Hdc);

BOOL InvalidateRect(
    HWID hWId,
    const RECT *lpRect,
    BOOL bErase);

int FillRect(
    HCRDC        hDC,
    const RECT   *lprc,
    HBRUSH       hbr);

BOOL PatBlt(
    HCRDC  hdc,
    int    nXLeft,
    int    nYLeft,
    int    nWidth,
    int    nHeight,
    DWORD  dwRop,
    HBRUSH hbr);

BOOL GetClientRect(
    HWID   hWid,
    PRECT lpRect);

BOOL MoveWindow(
    HWID hWid,
    int  X,
    int  Y,
    int  nWidth,
    int  nHeight,
    BOOL bRepaint);

BOOL GetWindowRect(
    HWID   hWid,
    PRECT  pRect);

UINT SetTimer(
    HWID  hWid,
    UINT  nIDEvent,
    UINT  uElapse,
    BOOL (*lpTimerFunc)(BOOL));

BOOL KillTimer(
    HWID hWid,
    UINT uIDEvent);

void DestroyWindow (HWID widget);

class PaintWidget : public QWidget
{
    Q_OBJECT
public:
 //  MyWidget();
 
protected:
    void paintEvent(QPaintEvent *event);
signals:
 
public slots:
};

#endif