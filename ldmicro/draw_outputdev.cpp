//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
//
// This file is part of LDmicro.
// 
// LDmicro is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// LDmicro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// The two `output devices' for the drawing code: either the export as text
// stuff to write to a file, or all the routines concerned with drawing to
// the screen.
// Jonathan Westhues, Dec 2004
// 
// Ported to linus by: R Ramana, 2018
//-----------------------------------------------------------------------------

#include "linuxUI.h"
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"


void (*DrawChars)(HCRDC Hcr, int, int, const char *);

// After an undo all the memory addresses change but make an effort to put
// the cursor roughly where it should be.
int SelectedGxAfterNextPaint = -1;
int SelectedGyAfterNextPaint = -1;

// After pushing a rung up or down the position of that rung in the table
// changes, but the cursor should stay where it was.
BOOL ScrollSelectedIntoViewAfterNextPaint;

// Buffer that we write to when exporting (drawing) diagram to a text file. 
// Dynamically allocated so that we're at least slightly efficient.
static char **ExportBuffer;

// The fonts that we will use to draw the ladder diagram: fixed width, one
// normal-weight, one bold.
HFONT       FixedWidthFont;
HFONT       FixedWidthFontBold;

// Different colour brushes for right and left buses in simulation, but same
// colour for both in edit mode; also for the backgrounds in simulation and
// edit modes.
static HBRUSH   BusRightBus;
static HBRUSH   BusLeftBrush;
static HBRUSH   BusBrush;
static HBRUSH   BgBrush;
static HBRUSH   SimBgBrush;

// Parameters that determine our offset if we are scrolled
// int ScrollXOffset;
// int ScrollYOffset;
// int ScrollXOffsetMax;
// int ScrollYOffsetMax;

// Is the cursor currently drawn? We XOR it so this gets toggled.
static BOOL CursorDrawn;

// Colours with which to do syntax highlighting, configurable
SyntaxHighlightingColours HighlightColours;

#define X_RIGHT_PADDING 30

//-----------------------------------------------------------------------------
// Blink the cursor on the schematic; called by a Windows timer. We XOR
// draw it so just draw the same rectangle every time to show/erase the
// cursor. Cursor may be in one of four places in the selected leaf (top,
// bottom, left, right) but we don't care; just go from the coordinates
// computed when we drew the schematic in the paint procedure.
//-----------------------------------------------------------------------------
BOOL BlinkCursor(BOOL kill = FALSE)
{
    // if(GetFocus(MainWindow) != !CursorDrawn) return TRUE;
    
    if(Cursor.left() == 0)
    {
        CursorObject->setVisible(FALSE);
        return TRUE;
    }

    QRect c;
    c = Cursor;

    c.setTop(c.top() - POS_HEIGHT*FONT_HEIGHT);
    c.setLeft(c.left());
    if(DrawWindow == NULL)
        return FALSE;

    HWID Hcr = DrawWindow;

    static int PREV_x = c.left();
    static int PREV_y = c.top();
    static int PREV_w = c.width();
    static int PREV_h = c.height();

    if (PREV_x != c.left() || PREV_y != c.top() ||
        PREV_w != c.width() || PREV_h != c.height())
    {
        CursorObject->setGeometry(c);
        CursorObject->setVisible(TRUE);
        PREV_x = c.left();
        PREV_y = c.top();
        PREV_w = c.width();
        PREV_h = c.height();
    }
    CursorObject->setGeometry(Cursor);

    if (CursorObject->isVisible())
        CursorObject->setVisible(FALSE);
    else
        CursorObject->setVisible(TRUE);
    InvalidateRect(DrawWindow, NULL, FALSE);

    return !kill;
}

//-----------------------------------------------------------------------------
// Output a string to the screen at a particular location, in character-
// sized units.
//-----------------------------------------------------------------------------
static void DrawCharsToScreen(HCRDC Hcr, int cx, int cy, const char *str)
{
    if(cy < -2) return;

    COLORREF prev;
    BOOL firstTime = TRUE;
    BOOL inNumber = FALSE;
    BOOL inComment = FALSE;
    int inBrace = 0;
    for(; *str; str++, cx++) {
        int x = cx * FONT_WIDTH + X_PADDING;
        int y = cy * FONT_HEIGHT + Y_PADDING;

        BOOL hiOk = !(InSimulationMode || ThisHighlighted);

        if(strchr("{}[]", *str) && hiOk && !inComment)  {
            if(*str == '{' || *str == '[') inBrace++;
            if(inBrace == 1) {
                prev = GetTextColor(Hcr);
                SetTextColor(Hcr, HighlightColours.punct);
                TextOut(DrawWindow, Hcr, x, y, str, 1);
                SetTextColor(Hcr, prev);
            } else {
                TextOut(DrawWindow, Hcr, x, y, str, 1);
            }
            if(*str == ']' || *str == '}') inBrace--;
        } else if((
            (isdigit(*str) && (firstTime || isspace(str[-1]) 
                || str[-1] == ':' || str[-1] == '[')) ||
            (*str == '-' && isdigit(str[1]))) && hiOk && !inComment)
        {
            prev = GetTextColor(Hcr);
            SetTextColor(Hcr, HighlightColours.lit);
            TextOut(DrawWindow, Hcr, x, y, str, 1);
            SetTextColor(Hcr, prev);
            inNumber = TRUE;
        } else if(*str == '\x01') {
            cx--;
            if(hiOk) {
                prev = GetTextColor(Hcr);
                SetTextColor(Hcr, HighlightColours.op);
            }
        } else if(*str == '\x02') {
            cx--;
            if(hiOk) {
                SetTextColor(Hcr, prev);
                inComment = FALSE;
            }
        } else if(*str == '\x03') {
            cx--;
            if(hiOk) {
                prev = GetTextColor(Hcr);
                SetTextColor(Hcr, HighlightColours.comment);
                inComment = TRUE;
            }
        } else if(inNumber) {
            if(isdigit(*str) || *str == '.') {
                prev = GetTextColor(Hcr);
                SetTextColor(Hcr, HighlightColours.lit);
                TextOut(DrawWindow, Hcr, x, y, str, 1);
                SetTextColor(Hcr, prev);
            } else {
                TextOut(DrawWindow, Hcr, x, y, str, 1);
                inNumber = FALSE;
            }
        } else {
            TextOut(DrawWindow, Hcr, x, y, str, 1);
        }

        firstTime = FALSE;
    }
}

//-----------------------------------------------------------------------------
// Total number of columns that we can display in the given amount of 
// window area. Need to leave some slop on the right for the scrollbar, of
// course.
//-----------------------------------------------------------------------------
int ScreenColsAvailable(void)
{
    int rw = DrawWindow->width();

    return (rw - (X_PADDING + X_RIGHT_PADDING)) / (POS_WIDTH*FONT_WIDTH);
}

//-----------------------------------------------------------------------------
// Total number of columns that we can display in the given amount of 
// window area. Need to leave some slop on the right for the scrollbar, of
// course, and extra slop at the bottom for the horiz scrollbar if it is
// shown.
//-----------------------------------------------------------------------------
int ScreenRowsAvailable(void)
{
    int adj;
    /*if(ScrollXOffsetMax == 0) {
        adj = 0;
    } else {*/
        adj = 18;
    // }
    return (IoListTop - Y_PADDING - adj) / (POS_HEIGHT*FONT_HEIGHT);
}

void PaintWidget::timerEvent(QTimerEvent *event)
{
    
    if(event->timerId() == CursorTimer)
    {
        BlinkCursor();
        // printf("CursorTimer\n");
    }
    if(event->timerId() == SimulateTimer)
    {
        PlcCycleTimer(FALSE);
    }
}

//-----------------------------------------------------------------------------
// Paint the ladder logic program to the screen. Also figure out where the
// cursor should go and fill in coordinates for BlinkCursor. Not allowed to
// draw deeper than IoListTop, as we would run in to the I/O listbox.
//-----------------------------------------------------------------------------
void PaintWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setClipping(true);
    painter.setRenderHint(QPainter::TextAntialiasing);
    HCRDC Hcr = &painter;

    ok();
   
    int bw = this->width();// = r.right;
    int bh = this->height();
    
    /// now figure out how we should draw the ladder logic
    ColsAvailable = ProgCountWidestRow();
    if(ColsAvailable < ScreenColsAvailable()) {
        ColsAvailable = ScreenColsAvailable();
    }
    memset(DisplayMatrix, 0, sizeof(DisplayMatrix));
    SelectionActive = FALSE;
    memset(&Cursor, 0, sizeof(Cursor));

    DrawChars = DrawCharsToScreen;

    int i;
    int cy = 0;
    int cx = 0;
    // int rowsAvailable = ScreenRowsAvailable();

    for(i = 0; i < Prog.numRungs; i++) {
        int thisHeight = POS_HEIGHT*CountHeightOfElement(ELEM_SERIES_SUBCKT,
            Prog.rungs[i]);

        // For speed, there is no need to draw everything all the time, but
        // we still must draw a bit above and below so that the DisplayMatrix
        // is filled in enough to make it possible to reselect using the
        // cursor keys.
        /*if(((cy + thisHeight) >= (ScrollYOffset - 8)*POS_HEIGHT) &&
            (cy < (ScrollYOffset + rowsAvailable + 8)*POS_HEIGHT))*/
        {
            SetBkColor(DrawWindow, Hcr, InSimulationMode ? HighlightColours.simBg :
                HighlightColours.bg);
            SetTextColor(Hcr, InSimulationMode ? HighlightColours.simRungNum :
                HighlightColours.rungNum);
            SelectObject(Hcr, FixedWidthFont);
            int rung = i + 1;
            int y = Y_PADDING + FONT_HEIGHT*cy;
            int yp = y; /*+ FONT_HEIGHT*(POS_HEIGHT/2) - 
                POS_HEIGHT*FONT_HEIGHT*ScrollYOffset;*/
            
            if(rung < 10) {
                char r[1] = { rung + '0' };
                TextOut(DrawWindow, Hcr, 8 + FONT_WIDTH, yp, r, 1);
            } else {
                char r[2] = { (rung / 10) + '0', (rung % 10) + '0' };
                TextOut(DrawWindow, Hcr, 8, yp, r, 2);
            }

            cx = 0;
            DrawElement(Hcr, ELEM_SERIES_SUBCKT, Prog.rungs[i], &cx, &cy, 
                Prog.rungPowered[i]);
        }

        cy += thisHeight;
        cy += POS_HEIGHT;
    }
    // printf("Endrung:%d\n", cy);
    QSize DWSize = this->size();
    int newHeight = ((cy + (POS_HEIGHT/2)) * FONT_HEIGHT + Y_PADDING + 50);
    if(DWSize.height() + POS_HEIGHT < newHeight)
    {
        DWSize.setHeight(newHeight);
    }
    int newWidth = ((cx + (POS_WIDTH/2)) * FONT_WIDTH + X_PADDING);
    if(DWSize.width() + POS_WIDTH < newWidth)
    {
        DWSize.setWidth(newWidth);
    }
    DrawWindow->resize(DWSize);

    cy -= 2;
    DrawEndRung(Hcr, 0, cy);
    
    if(SelectedGxAfterNextPaint >= 0) {
        MoveCursorNear(SelectedGxAfterNextPaint, SelectedGyAfterNextPaint);
        // InvalidateRect(DrawWindow, NULL, FALSE);
        SelectedGxAfterNextPaint = -1;
        SelectedGyAfterNextPaint = -1;
    } else if(ScrollSelectedIntoViewAfterNextPaint && Selected) {
        SelectElement(-1, -1, Selected->selectedState);
        ScrollSelectedIntoViewAfterNextPaint = FALSE;
        // InvalidateRect(DrawWindow, NULL, FALSE);
    } else {
        if(!SelectionActive) {
            if(Prog.numRungs > 0) {
                if(MoveCursorTopLeft()) {
                    // InvalidateRect(DrawWindow, NULL, FALSE);
                }
            }
        }
    }

    /// draw the `buses' at either side of the screen
    QRect r;
    r.setLeft(X_PADDING - FONT_WIDTH);
    r.setTop(0);
    r.setRight(r.left() + 4);
    r.setBottom(this->height());
    FillRect(Hcr, &r, InSimulationMode ? BusLeftBrush : BusBrush);
    r.setLeft(POS_WIDTH*FONT_WIDTH*ColsAvailable + 32);
    r.setRight(POS_WIDTH*FONT_WIDTH*ColsAvailable + 32);
    FillRect(Hcr, &r, InSimulationMode ? BusRightBus : BusBrush);
    // InvalidateRect(DrawWindow, NULL, FALSE);
 
    CursorDrawn = FALSE;

    ok();
}

//-----------------------------------------------------------------------------
// Set up the syntax highlighting colours, according to the currently selected
// scheme.
//-----------------------------------------------------------------------------
static void SetSyntaxHighlightingColours(void)
{
    static const SyntaxHighlightingColours Schemes[] = {
        {
            RGB(0, 0, 0),           // bg
            RGB(255, 255, 225),     // def
            RGB(255, 110, 90),      // selected
            RGB(255, 150, 90),      // op
            RGB(255, 255, 100),     // punct
            RGB(255, 160, 160),     // lit
            RGB(120, 255, 130),     // name
            RGB(130, 130, 130),     // rungNum
            RGB(130, 130, 245),     // comment

            RGB(255, 255, 255),     // bus

            RGB(0, 0, 0),           // simBg
            RGB(130, 130, 130),     // simRungNum
            RGB(100, 130, 130),     // simOff
            RGB(255, 150, 150),     // simOn

            RGB(255, 150, 150),     // simBusLeft
            RGB(150, 150, 255),     // simBusRight
        },
    };

    memcpy(&HighlightColours, &Schemes[0], sizeof(Schemes[0]));
}

//-----------------------------------------------------------------------------
// Set up the stuff we'll need to draw our schematic diagram. Fonts, brushes,
// pens, etc.
//-----------------------------------------------------------------------------
void InitForDrawing(void)
{
    DrawWindow = new PaintWidget();
    DrawWindow->setFocusPolicy(Qt::StrongFocus);
    SetSyntaxHighlightingColours();

    FixedWidthFont = CreateFont(
        FONT_HEIGHT, FONT_WIDTH,
        0,
        FW_REGULAR,
        FALSE,
        "Lucida Console");

    FixedWidthFontBold = CreateFont(
        FONT_HEIGHT, FONT_WIDTH,
        0,
        FW_BOLD, // the bold text renders funny under Vista
        FALSE,
        "Lucida Console");

    LOGBRUSH lb;
    lb.lbStyle = BS_SOLID;
    lb.lbColor = HighlightColours.simBusRight;
    BusRightBus = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.simBusLeft;
    BusLeftBrush = CreateBrushIndirect(&lb);
    
    lb.lbColor = HighlightColours.bus;
    BusBrush = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.bg;
    BgBrush = CreateBrushIndirect(&lb);

    lb.lbColor = HighlightColours.simBg;
    SimBgBrush = CreateBrushIndirect(&lb);
}

//-----------------------------------------------------------------------------
// DrawChars function, for drawing to the export buffer instead of to the
// screen.
//-----------------------------------------------------------------------------
static void DrawCharsToExportBuffer(HCRDC Hcr, int cx, int cy, const char *str)
{
    while(*str) {
        if(*str >= 10) {
            ExportBuffer[cy][cx] = *str;
            cx++;
        }
        str++;
    }
}

//-----------------------------------------------------------------------------
// Export a text drawing of the ladder logic program to a file.
//-----------------------------------------------------------------------------
void ExportDrawingAsText(char *file)
{
    int maxWidth = ProgCountWidestRow();
    ColsAvailable = maxWidth;

    int totalHeight = 0;
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        totalHeight += CountHeightOfElement(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
        totalHeight += 1;
    }
    totalHeight *= POS_HEIGHT;
    totalHeight += 3;

    ExportBuffer = (char **)CheckMalloc(totalHeight * sizeof(char *));
   
    int l = maxWidth*POS_WIDTH + 8;
    for(i = 0; i < totalHeight; i++) {
        ExportBuffer[i] = (char *)CheckMalloc(l);
        memset(ExportBuffer[i], ' ', l-1);
        ExportBuffer[i][4] = '|';
        ExportBuffer[i][3] = '|';
        ExportBuffer[i][l-3] = '|';
        ExportBuffer[i][l-2] = '|';
        ExportBuffer[i][l-1] = '\0';
    }

    DrawChars = DrawCharsToExportBuffer;

    int cy = 1;

    for(i = 0; i < Prog.numRungs; i++) {
        int cx = 5;
        DrawElement(NULL, ELEM_SERIES_SUBCKT, Prog.rungs[i], &cx, &cy, 
            Prog.rungPowered[i]);

        if((i + 1) < 10) {
            ExportBuffer[cy+1][1] = '0' + (i + 1);
        } else {
            ExportBuffer[cy+1][1] = '0' + ((i + 1) % 10);
            ExportBuffer[cy+1][0] = '0' + ((i + 1) / 10);
        }

        cy += POS_HEIGHT*CountHeightOfElement(ELEM_SERIES_SUBCKT,
            Prog.rungs[i]);
        cy += POS_HEIGHT;
    }
    cy -= 2;
    DrawEndRung(NULL, 5, cy);

    FILE *f = fopen(file, "w");
    if(!f) {
        Error(_("Couldn't open '%s'\n"), f);
        return;
    }

    fprintf(f, "LDmicro export text\n");

    if(Prog.mcu) {
        fprintf(f, "for '%s', %.6f MHz crystal, %.1f ms cycle time\n\n",
            Prog.mcu->mcuName, Prog.mcuClock/1e6, Prog.cycleTime/1e3);
    } else {
        fprintf(f, "no MCU assigned, %.6f MHz crystal, %.1f ms cycle time\n\n",
            Prog.mcuClock/1e6, Prog.cycleTime/1e3);
    }

    fprintf(f, "\nLADDER DIAGRAM:\n\n");

    for(i = 0; i < totalHeight; i++) {
        ExportBuffer[i][4] = '|';
        fprintf(f, "%s\n", ExportBuffer[i]);
        CheckFree(ExportBuffer[i]);
    }
    CheckFree(ExportBuffer);
    ExportBuffer = NULL;

    fprintf(f, _("\n\nI/O ASSIGNMENT:\n\n"));
    
    fprintf(f, _("  Name                       | Type               | Pin\n"));
    fprintf(f,   " ----------------------------+--------------------+------\n");
    for(i = 0; i < Prog.io.count; i++) {
        char b[1024];
        memset(b, '\0', sizeof(b));

        PlcProgramSingleIo *io = &Prog.io.assignment[i];
        char *type = IoTypeToString(io->type);
        char pin[MAX_NAME_LEN];

        PinNumberForIo(pin, io);

        sprintf(b, "                             |                    | %s\n",
            pin);

        memcpy(b+2, io->name, strlen(io->name));
        memcpy(b+31, type, strlen(type));
        fprintf(f, "%s", b);
    }

    fclose(f);

    // we may have trashed the grid tables a bit; a repaint will fix that
    InvalidateRect(MainWindow, NULL, FALSE);
}
