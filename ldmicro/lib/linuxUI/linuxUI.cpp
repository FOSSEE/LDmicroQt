#include "linuxUI.h"

/// Global variables to hole mouse click positions
int GLOBAL_mouse_last_clicked_x;
int GLOBAL_mouse_last_clicked_y;

/// Brushes
const COLORREF BLACK_BR(0, 0, 0);
const COLORREF WHITE_BR(255, 255, 255);
const COLORREF GRAY_BR(128, 128, 128);
const COLORREF LTGRAY_BR(211, 211, 211);
const COLORREF DKGRAY_BR(169, 169, 169);

/// Variable to current text color
COLORREF HdcCurrentTextColor;

/// Variable to hold timers
std::vector<TimerRecord> timerRecords;

/// EnableMenuItem Variables
const UINT MF_ENABLED = 0;
const UINT MF_GRAYED = 1;
const UINT MF_CHECKED = 2;
const UINT MF_UNCHECKED = 3;

/// ListStore
HWID view;
 
/// Wraper function for gtk_window_has_toplevel_focus
BOOL GetFocus(HWID window)
{
    return TRUE;
}

COLORREF RGB(int red, int green, int blue)
{
    COLORREF col(red, green, blue);

    return col;
}

int MessageBox(HWID pWindow, char* message, char* title, UINT mFlags, UINT iFlags)
 {
    QMessageBox msg((QMessageBox::Icon)iFlags, title, message, (QMessageBox::StandardButton)mFlags, pWindow);
    return msg.exec();
 }


BOOL GetSaveFileName(OPENFILENAME *ofn)
{
    std::string strFilter;
    DWORD strFilterLen = 0;
    BOOL filterResetFlag = FALSE;
    char filename[15] = "Untitled";

    if (ofn->lpstrDefExt != NULL)
        sprintf(filename, "Untitled.%s", ofn->lpstrDefExt);
    
    while(!((ofn->lpstrFilter[strFilterLen] == '\0') &&
        (ofn->lpstrFilter[strFilterLen + 1] == '\0')))
    {
        if(filterResetFlag)
        {
            strFilter = strFilter + "(";
            strFilter.append(&ofn->lpstrFilter[strFilterLen]);
            strFilter = strFilter + ");;";
            filterResetFlag = FALSE;
        }
        else
        {
            strFilter.append(&ofn->lpstrFilter[strFilterLen]);
            filterResetFlag = TRUE;
        }
        strFilterLen = strFilterLen + strlen(&ofn->lpstrFilter[strFilterLen]) +1;
    }
    
    QString file = QFileDialog::getSaveFileName(ofn->parentWindow, ofn->lpstrTitle,
        QStandardPaths::locate(QStandardPaths::HomeLocation,"",
            QStandardPaths::LocateDirectory)+ filename,
        strFilter.c_str());
    BOOL exitStatus;
    file.isEmpty() ? exitStatus = FALSE : exitStatus = TRUE;

    if (exitStatus)
    {
        strcpy(ofn->lpstrFile, file.toStdString().c_str());
    }

    return exitStatus;
}

BOOL GetOpenFileName(OPENFILENAME *ofn)
{
    std::string strFilter;
    DWORD strFilterLen = 0;
    BOOL filterResetFlag = FALSE;
    while(!((ofn->lpstrFilter[strFilterLen] == '\0') &&
        (ofn->lpstrFilter[strFilterLen + 1] == '\0')))
    {
        if(filterResetFlag)
        {
            strFilter = strFilter + "(";
            strFilter.append(&ofn->lpstrFilter[strFilterLen]);
            strFilter = strFilter + ");;";
            filterResetFlag = FALSE;
        }
        else
        {
            strFilter.append(&ofn->lpstrFilter[strFilterLen]);
            filterResetFlag = TRUE;
        }
        strFilterLen = strFilterLen + strlen(&ofn->lpstrFilter[strFilterLen]) +1;
    }
    // printf("patterns:%s\n",strFilter.c_str() );
    QString filename = QFileDialog::getOpenFileName(ofn->parentWindow, ofn->lpstrTitle,
        QStandardPaths::locate(QStandardPaths::HomeLocation,".",
            QStandardPaths::LocateDirectory),
        strFilter.c_str());
    if(filename == "")
    {
        return false;
    }
    
        strcpy(ofn->lpstrFile,filename.toStdString().c_str());
        // printf("FileName:%s",ofn->lpstrFile);

    return true;
}


void EnableMenuItem(HMENU MenuName, QAction* MenuItem, UINT CheckEnabledItem) 
{
    switch (CheckEnabledItem){
        case MF_ENABLED :
           MenuItem->setEnabled(true);
        break;
        case MF_GRAYED :
           MenuItem->setEnabled(false);
        break; 
    }
}

// Special function designed for qt, since disabling top-level menu
// Does not disable its child menus. They can still be accessed via
// keyboard shortcuts
void EnableMenuItem(HMENU MenuName, HMENU MenuItem, UINT CheckEnabledItem) 
{
    QList<QAction *> MenuList = MenuItem->actions();
    QList<QAction *>::iterator item = MenuList.begin();
    switch (CheckEnabledItem){
        case MF_ENABLED :
            while((item != MenuList.end()))/* || !(MenuList->isEmpty))*/
            {
                (*item)->setEnabled(true);
                (*item)->blockSignals(false);
                item++;
            }
           MenuItem->setEnabled(true);
           MenuItem->blockSignals(false);
        break;
        case MF_GRAYED :
            while((item != MenuList.end()))/* || !(MenuList->isEmpty))*/
            {
                (*item)->setEnabled(false);
                (*item)->blockSignals(true);
                item++;
            }
           MenuItem->setEnabled(false);
           MenuItem->blockSignals(true);
        break; 
    }
}

void CheckMenuItem(HMENU MenuName, QAction* MenuItem, UINT Check)
{
    switch (Check){
        case MF_CHECKED :
            MenuItem->setChecked(true);
        break;
        case MF_UNCHECKED :
            MenuItem->setChecked(false);
        break;
    }
}

HANDLE GetStockObject(int fnObject)
{
    switch(fnObject)
    {
        case BLACK_BRUSH:
            return (HANDLE)&BLACK_BR;
            break;
        case WHITE_BRUSH:
            return (HANDLE)&WHITE_BR;
            break;
        case GRAY_BRUSH:
            return (HANDLE)&GRAY_BR;
            break;
        case LTGRAY_BRUSH:
            return (HANDLE)&LTGRAY_BR;
            break;
        case DKGRAY_BRUSH:
            return (HANDLE)&DKGRAY_BR;
            break;
        default:
            return (HANDLE)&WHITE_BR;
    }
}

void SelectObject(HCRDC hcr, HFONT hfont)
{
    if (hcr ==NULL)
        return;
    QFont qtfont = hcr->font();
    qtfont.setFamily(hfont->lpszFace);
    qtfont.setPixelSize(hfont->nHeight - 3);
    qtfont.setFixedPitch(TRUE);
    qtfont.setStyle(hfont->fdwItalic ? QFont::StyleItalic : QFont::StyleNormal);
    qtfont.setWeight(hfont->fnWeight == FW_BOLD ? QFont::Bold : QFont::Normal);
    hcr->setFont(qtfont);
}

HBRUSH CreateBrushIndirect(PLOGBRUSH plb)
{
    COLORREF* brush = new COLORREF;
    brush->setRgb(  plb->lbColor.red(), 
                    plb->lbColor.green(),
                    plb->lbColor.blue(),
                    (plb->lbStyle == BS_SOLID) ? 255 : 51);

    return brush;
}

HFONT CreateFont(int nHeight, int nWidth, int nOrientation, int fnWeight,
    DWORD fdwItalic, LPCTSTR lpszFace)
{
    HFONT font = (HFONT)malloc(strlen(lpszFace) + 1 + sizeof(FONT));
    font->nHeight = nHeight;
    font->nWidth = nWidth;
    font->nOrientation = nOrientation;
    font->fnWeight = fnWeight;
    font->fdwItalic = fdwItalic;
    font->lpszFace = (char*)malloc(strlen(lpszFace)+1);
    strcpy(font->lpszFace, lpszFace);
    
    return font;
}

void SetBkColor(HWID widget, HCRDC hcr, COLORREF bkCol)
{
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, bkCol);
    widget->setPalette(pal);
}

void SetTextColor(HCRDC hcr, COLORREF color)
{
    if (hcr == NULL)
        return;
    QPen qtpen = hcr->pen();
    qtpen.setColor(color);
    hcr->setPen(qtpen);
    HdcCurrentTextColor = color;
    // gdk_cairo_set_source_rgba (hcr, &color);
}

void TextOut(HWID hWid, HCRDC hcr, int nXStart, int nYStart, LPCTSTR lpString, int cchString)
{
    if (hcr == NULL)
        return;
    int width = hWid->width();
    int height = hWid->height();
    BOOL resize_flag = FALSE;
    QFont newFont= hcr->font();
    char* text = (char*)malloc(cchString);
    strncpy(text, lpString, cchString);
    text[cchString] = '\0';

    hcr->drawText(nXStart, nYStart, (QString)text);
}

COLORREF GetTextColor(HCRDC Hdc)
{
    return HdcCurrentTextColor;
}

BOOL InvalidateRect(HWID hWid, const RECT *lpRect, BOOL bErase)
{
    hWid->repaint();
    return TRUE;
}

int FillRect(HCRDC hDC, const QRect *lprc, HBRUSH hbr)
{
    if (hDC == NULL)
        return -1;
    QBrush curbrush = hDC->brush();
    curbrush.setColor(*hbr);
    curbrush.setStyle(Qt::SolidPattern);
    hDC->setBrush(curbrush);
    hDC->drawRect(*lprc);    
    return 0;
}

UINT SetTimer(HWID hWid, UINT  nIDEvent, UINT uElapse, UINT TimerID)
{
    if(TimerID != NULL)
        return nIDEvent;
    switch(nIDEvent)
    {
        case TIMER_BLINK_CURSOR:
        {
            TimerID = hWid->startTimer(uElapse);
            CursorObject = new QGroupBox(hWid);
            QPalette pal = CursorObject->palette();
            pal.setColor(QPalette::Window, Qt::white);
            CursorObject->setAutoFillBackground(true);
            CursorObject->setPalette(pal);
            CursorObject->setGeometry(0,0,2,20);
        }
        break;
        
        case TIMER_SIMULATE:
        {
            TimerID = hWid->startTimer(uElapse);
        }
        break;
    }

    return TimerID;
}

BOOL KillTimer(HWID hWid, UINT uIDEvent)
{
    switch(uIDEvent)
    {
        case TIMER_BLINK_CURSOR:
            hWid->killTimer(CursorTimer);
            CursorTimer = NULL;
            CursorObject->setVisible(FALSE);
        break;

        case TIMER_SIMULATE:
            hWid->killTimer(SimulateTimer);
            SimulateTimer = NULL;
        break;
    }

    return TRUE;
}

