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

/// Accelerators (keyboard shortcuts)
GtkAccelGroup* AccelGroup;
GClosure* closure;

/// ListStore
HWID view;
HTVC column;
 
/// Wraper function for gtk_window_has_toplevel_focus
BOOL GetFocus(HWID window)
{
    // return (BOOL) gtk_window_has_toplevel_focus(GTK_WINDOW(window));
    return TRUE;
}

COLORREF RGB(int red, int green, int blue)
{
    COLORREF col(red, green, blue);
    // col.red = red/255.0;
    // col.green = green/255.0;
    // col.blue = blue/255.0;
    // col.alpha = 1.0;

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
    if(filename == NULL)
    {
        return FALSE;
    }
    
        strcpy(ofn->lpstrFile,filename.toStdString().c_str());
        // printf("FileName:%s",ofn->lpstrFile);

    return TRUE;
}


void EnableMenuItem(HMENU MenuName, QAction* MenuItem, UINT CheckEnabledItem) 
{
    switch (CheckEnabledItem){
        case MF_ENABLED :
           MenuItem->setEnabled(true);
           // MenuItem->blockSignals(false);
        break;
        case MF_GRAYED :
           MenuItem->setEnabled(false);
           // MenuItem->blockSignals(true);
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
    pal.setColor(QPalette::Background, bkCol);
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
    // newFont
    /*if(nYStart+(extents.height/2.0) >= height)
    {
        height += extents.height + 50;
        resize_flag = TRUE;
    }
    
    if (nXStart+(extents.width/2.0) >= width)
    {
        width += extents.width;
        resize_flag = TRUE;
    }*/
    char* text = (char*)malloc(cchString);
    strncpy(text, lpString, cchString);
    text[cchString] = '\0';

    hcr->drawText(nXStart, nYStart, (QString)text);
    // cairo_move_to(hcr, nXStart, nYStart);
    // cairo_show_text(hcr, text);

    // cairo_fill (hcr);

    /*if (resize_flag)  // To do later
        hcr->setWindow();*/
    /*if (hcr == NULL)
        return;
    
    nYStart += 30;
    
    cairo_text_extents_t extents;
    cairo_text_extents (hcr, lpString, &extents);
    int width = gtk_widget_get_allocated_width (hWid);
    int height= gtk_widget_get_allocated_height (hWid);
    BOOL resize_flag = FALSE;

    if(nYStart+(extents.height/2.0) >= height)
    {
        height += extents.height + 50;
        resize_flag = TRUE;
    }
    
    if (nXStart+(extents.width/2.0) >= width)
    {
        width += extents.width;
        resize_flag = TRUE;
    }

    if (resize_flag)
        gtk_widget_set_size_request(hWid, width, height);
    
    char* text = (char*)malloc(cchString);
    strncpy(text, lpString, cchString);
    text[cchString] = '\0';

    cairo_move_to(hcr, nXStart, nYStart);
    cairo_show_text(hcr, text);

    cairo_fill (hcr);*/
}

COLORREF GetTextColor(HCRDC Hdc)
{
    // COLORREF col;
    // gtk_style_context_get_color (Hdc,
    //                             gtk_style_context_get_state (Hdc),
    //                             &col);
    
    return HdcCurrentTextColor;
}

BOOL InvalidateRect(HWID hWid, const RECT *lpRect, BOOL bErase)
{
    /*if(!GDK_IS_WINDOW(hWid))
        return FALSE;

    if (lpRect == NULL)
    {
        gdk_window_invalidate_rect (gtk_widget_get_window (hWid), NULL, FALSE);
        return TRUE;
    }

    GDRECT Gdrect;
    RECT_to_GDRECT(lpRect, &Gdrect);
    // gtk_widget_queue_draw(hWid);
    gdk_window_invalidate_rect (gtk_widget_get_window (hWid), &Gdrect, FALSE);
    */
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
    /*
    GDRECT gdrc;
    RECT_to_GDRECT(lprc, &gdrc);

    cairo_set_source_rgb(hDC, hbr->red, hbr->green, hbr->blue);
    cairo_rectangle(hDC, gdrc.x, gdrc.y, gdrc.width, gdrc.height);
    cairo_stroke_preserve(hDC);
    cairo_fill(hDC);*/
    
    return 0;
}

BOOL PatBlt(HWID hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, DWORD dwRop, HBRUSH hbr)
{
    if (hdc == NULL)
        return FALSE;
    
    //cairo_set_source_rgb(hdc, hbr->red, hbr->green, hbr->blue);
    //cairo_rectangle(hdc, nXLeft, nYLeft + 20, nWidth, nHeight);
    //cairo_stroke_preserve(hdc);

    //cairo_fill(hdc);

    return TRUE;
}

BOOL GetClientRect(HWID hWid, PRECT pRect)
{   
 /*   GtkAllocation allocation;
    gtk_widget_get_allocation (hWid, &allocation);

    pRect->top = allocation.x;
    pRect->left = allocation.y;
    pRect->right = allocation.width;
    pRect->bottom = allocation.height;
*/
    return TRUE;
}

BOOL MoveWindow(HWID hWid, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
 /*   gtk_window_move(GTK_WINDOW(hWid), X, Y);
    gtk_window_resize(GTK_WINDOW(hWid), nWidth, nHeight);

    if (bRepaint)
        gdk_window_invalidate_rect (gtk_widget_get_window (hWid), NULL, FALSE);
 */   
    return TRUE;
}


BOOL GetWindowRect(HWID hWid, PRECT pRect)
{
    /*GtkAllocation allocation;
    gtk_widget_get_allocation (hWid, &allocation);

    pRect->top = allocation.x;
    pRect->left = allocation.y;
    pRect->right = allocation.width;
    pRect->bottom = allocation.height;
*/
    return TRUE;
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
            pal.setColor(QPalette::Background, Qt::white);
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
    // if(hWid!=NULL)
    //     CursorObject->setVisible(TRUE);

    return TimerID;
    // auto record_it = std::find_if(timerRecords.begin(), timerRecords.end(),  [&nIDEvent](TimerRecord &Record) { return Record.ufID == nIDEvent; });

    // if (record_it != timerRecords.end())
    //     return 0;

    // TimerRecord tr;
    // tr.pfun = lpTimerFunc;
    // tr.ufID = nIDEvent;
    // tr.utID = g_timeout_add(uElapse, (GSourceFunc)lpTimerFunc, FALSE);

    // timerRecords.push_back(tr);
    // return tr.utID;
}

BOOL KillTimer(HWID hWid, UINT uIDEvent)
{
    /*auto record_it = std::find_if(timerRecords.begin(), timerRecords.end(),  [&uIDEvent](TimerRecord &Record) { return Record.ufID == uIDEvent; });

    if (record_it == timerRecords.end())
        return FALSE;
    
    record_it->pfun(TRUE);
    g_source_remove (record_it->utID);
    timerRecords.erase(record_it);*/
    // printf("KillTimer\n");
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

/*void DestroyWindow (HWID widget)
{
    if (GTK_IS_WIDGET(widget))
    {
        gtk_widget_destroy (widget);
    }
}*/


