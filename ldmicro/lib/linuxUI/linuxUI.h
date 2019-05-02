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
#include <QGroupBox>
#include <QSignalMapper>
#include <QKeyEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QSlider>
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

// Timer IDs associated with the main window.
#define TIMER_BLINK_CURSOR      1
#define TIMER_SIMULATE          2

/// Flags
/// message box
#define MB_OK              QMessageBox::Ok
#define MB_OKCANCEL        QMessageBox::Ok | QMessageBox::Cancel
#define MB_YESNO           QMessageBox::Yes | QMessageBox::No
#define MB_YESNOCANCEL     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel

#define IDOK      QMessageBox::Ok
#define IDCANCEL  QMessageBox::Cancel
#define IDYES     QMessageBox::Yes
#define IDNO      QMessageBox::No

#define MB_ICONERROR        QMessageBox::Critical
#define MB_ICONQUESTION     QMessageBox::Question
#define MB_ICONWARNING      QMessageBox::Warning
#define MB_ICONINFORMATION  QMessageBox::Information

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
#define VK_TAB Qt::Key_Tab

#define VK_DOWN  Qt::Key_Down
#define VK_UP    Qt::Key_Up
#define VK_LEFT  Qt::Key_Left
#define VK_RIGHT Qt::Key_Right

#define VK_NP_DOWN  Qt::Key_Down
#define VK_NP_UP    Qt::Key_Up
#define VK_NP_LEFT  Qt::Key_Left
#define VK_NP_RIGHT Qt::Key_Right

#define VK_RETURN  Qt::Key_Return
#define VK_ESCAPE  Qt::Key_Escape
#define VK_F5 Qt::Key_F5
#define VK_F1 Qt::Key_F1

#define VK_OEM_PLUS Qt::Key_Plus
#define VK_OEM_MINUS Qt::Key_Minus
#define VK_OEM_PERIOD Qt::Key_Period
#define VK_OEM_COMMA Qt::Key_Comma

#define VK_DELETE Qt::Key_Delete 
#define VK_NP_DELETE Qt::Key_Delete

#define VK_OEM_1 Qt::Key_Colon     // GDK_KEY_semicolon
#define VK_OEM_2 Qt::Key_Question  // GDK_KEY_slash
#define VK_OEM_5 Qt::Key_Backslash // GDK_KEY_bar

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
extern QGroupBox*       CursorObject;

/// ListStore
extern HWID view;
extern HTVC column;

// Timer IDs associated with the main window.
extern int CursorTimer;
extern int SimulateTimer;

/// Structures
typedef struct OpenFileInfoData {
    DWORD         lStructSize;
    HWID          parentWindow;
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

/******************************************************************
Class to create slots for signals

*******************************************************************/
class ProgramSlots : public QObject
{
    Q_OBJECT
    public:
    signals:
    public slots:
    void LD_WM_Command_call(int CommandCode);
};

/// Variables
extern COLORREF HdcCurrentTextColor;
extern std::vector<TimerRecord> timerRecords;
extern int GLOBAL_mouse_last_clicked_x;
extern int GLOBAL_mouse_last_clicked_y;
extern ProgramSlots MenuHandle;
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
    UINT  mFlags,
    UINT  iFlags);

BOOL GetSaveFileName(OPENFILENAME *ofn);

BOOL GetOpenFileName(OPENFILENAME *ofn);

void EnableMenuItem(
    HMENU MenuName, 
    QAction* MenuItem, 
    UINT  CheckEnabledItem);

void EnableMenuItem(
    HMENU MenuName, 
    HMENU MenuItem, 
    UINT  CheckEnabledItem);

void CheckMenuItem(
    HMENU MenuName, 
    QAction* MenuItem, 
    UINT  Check);

HANDLE GetStockObject(int fnObject);

void SelectObject(
    HCRDC hcr, 
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
    const QRect   *lprc,
    HBRUSH       hbr);

BOOL PatBlt(
    HWID   hdc,
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
    UINT TimerID);

BOOL KillTimer(
    HWID hWid,
    UINT uIDEvent);

// void DestroyWindow (HWID widget);

class PaintWidget : public QWidget
{
    Q_OBJECT
public:
 //  MyWidget();
    // PaintWidget(QWidget* parent): QWidget(parent)
    // {}
    void keyPressEvent(QKeyEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
 
protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);
signals:
 
public slots:
};

#endif