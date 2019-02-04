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
// A ladder logic compiler for 8 bit micros: user draws a ladder diagram,
// with an appropriately constrained `schematic editor,' and then we can
// simulated it under Windows or generate PIC/AVR code that performs the
// requested operations. This files contains the program entry point, plus
// most of the UI logic relating to the main window.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "linuxUI.h"

#include <stdio.h>
#include <stdlib.h>
#include "ldmicro.h"
#include "freezeLD.h"
#include "mcutable.h"
#include <iomanip>
#include <iostream>
#include <QPushButton>
// #include <qapplication>

using namespace std;


HINSTANCE       Instance;
QApplication*   LDmicroApp;
HWID            MainWindow;
QIcon*          MWIcon;
QMenuBar*       MainMenu;
HWID            DrawWindow;
// parameters used to capture the mouse when implementing our totally non-
// general splitter control
//static HHOOK       MouseHookHandle;
static int         MouseY;

// For the open/save dialog boxes
#define LDMICRO_PATTERN "LDmicro Ladder Logic Programs (*.ld)\0*.ld\0" \
                     "All files\0*\0\0"
char CurrentSaveFile[MAX_PATH];
static BOOL ProgramChangedNotSaved = FALSE;

#define HEX_PATTERN  "Intel Hex Files (*.hex)\0*.hex\0All files\0*\0\0"
#define C_PATTERN "C Source Files (*.c)\0*.c\0All Files\0*\0\0"
#define INTERPRETED_PATTERN \
    "Interpretable Byte Code Files (*.int)\0*.int\0All Files\0*\0\0"
char CurrentCompileFile[MAX_PATH];

#define TXT_PATTERN  "Text Files (*.txt)\0*.txt\0All files\0*\0\0"

// Everything relating to the PLC's program, I/O configuration, processor
// choice, and so on--basically everything that would be saved in the
// project file.
PlcProgram Prog;

/// Function to safely quit program gtk main loop
gboolean LD_WM_Close_call(GtkWidget *widget, GdkEvent *event, gpointer user_data);

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then save the program to that
// file and then set our default filename to that.
//-----------------------------------------------------------------------------
/*static BOOL SaveAsDialog(void)
{
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.parentWindow = MainWindow;
    ofn.lpstrFilter = LDMICRO_PATTERN;
    ofn.lpstrDefExt = "ld";
    ofn.lpstrFile = CurrentSaveFile;
    ofn.nMaxFile = sizeof(CurrentSaveFile);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if(!GetSaveFileName(&ofn))
        return FALSE;

    if(!SaveProjectToFile(CurrentSaveFile)) {
        Error(_("Couldn't write to '%s'."), CurrentSaveFile);
        return FALSE;
    } else {
        ProgramChangedNotSaved = FALSE;
        return TRUE;
    }
}

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then export the program as
// an ASCII art drawing.
//-----------------------------------------------------------------------------
static void ExportDialog(void)
{
    char exportFile[MAX_PATH];
    OPENFILENAME ofn;

    exportFile[0] = '\0';

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.parentWindow = MainWindow;
    ofn.lpstrFilter = TXT_PATTERN;
    ofn.lpstrFile = exportFile;
    ofn.lpstrDefExt = "txt";
    ofn.lpstrTitle = _("Export As Text");
    ofn.nMaxFile = sizeof(exportFile);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    if(!GetSaveFileName(&ofn))
        return;

    ExportDrawingAsText(exportFile);
}

//-----------------------------------------------------------------------------
// If we already have a filename, save the program to that. Otherwise same
// as Save As. Returns TRUE if it worked, else returns FALSE.
//-----------------------------------------------------------------------------
static BOOL SaveProgram(void)
{
    if(strlen(CurrentSaveFile)) {
        if(!SaveProjectToFile(CurrentSaveFile)) {
            Error(_("Couldn't write to '%s'."), CurrentSaveFile);
            return FALSE;
        } else {
            ProgramChangedNotSaved = FALSE;
            return TRUE;
        }
    } else {
        return SaveAsDialog();
    }
}*/

//-----------------------------------------------------------------------------
// Compile the program to a hex file for the target micro. Get the output
// file name if necessary, then call the micro-specific compile routines.
//-----------------------------------------------------------------------------
static void CompileProgram(BOOL compileAs)
{
    if(compileAs || strlen(CurrentCompileFile)==0) {
        OPENFILENAME ofn;

        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.parentWindow = &MainWindow;
        ofn.lpstrTitle = _("Compile To");
        if(Prog.mcu && Prog.mcu->whichIsa == ISA_ANSIC) {
            ofn.lpstrFilter = C_PATTERN;
            ofn.lpstrDefExt = "c";
        } else if(Prog.mcu && Prog.mcu->whichIsa == ISA_INTERPRETED) {
            ofn.lpstrFilter = INTERPRETED_PATTERN;
            ofn.lpstrDefExt = "int";
        } else {
            ofn.lpstrFilter = HEX_PATTERN;
            ofn.lpstrDefExt = "hex";
        }
        ofn.lpstrFile = CurrentCompileFile;
        ofn.nMaxFile = sizeof(CurrentCompileFile);
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if(!GetSaveFileName(&ofn))
            return;

        // hex output filename is stored in the .ld file
        ProgramChangedNotSaved = TRUE;
    }

    if(!GenerateIntermediateCode()) return;
 
    if(Prog.mcu == NULL) {
        Error(_("Must choose a target microcontroller before compiling."));
        return;
    } 
    
    if(UartFunctionUsed() && Prog.mcu->uartNeeds.rxPin == 0) {
        Error(_("UART function used but not supported for this micro."));
        return;
    }
    
    if(PwmFunctionUsed() && Prog.mcu->pwmNeedsPin == 0) {
        Error(_("PWM function used but not supported for this micro."));
        return;
    }
    
    switch(Prog.mcu->whichIsa) {
        case ISA_AVR:           CompileAvr(CurrentCompileFile); break;
        case ISA_PIC16:         CompilePic16(CurrentCompileFile); break;
        case ISA_ANSIC:         CompileAnsiC(CurrentCompileFile); break;
        case ISA_INTERPRETED:   CompileInterpreted(CurrentCompileFile); break;
        case ISA_ARDUINO:   CompileArduino(CurrentCompileFile); break;

        default: oops();
    }
    
    IntDumpListing("t.pl");
}

//-----------------------------------------------------------------------------
// If the program has been modified then give the user the option to save it
// or to cancel the operation they are performing. Return TRUE if they want
// to cancel.
//-----------------------------------------------------------------------------
/*BOOL CheckSaveUserCancels(void)
{
    if(!ProgramChangedNotSaved) {
        // no problem
        return FALSE;
    }

    int r = MessageBox(MainWindow, 
        _("The program has changed since it was last saved.\r\n\r\n"
        "Do you want to save the changes?"), "LDmicro",
        MB_YESNOCANCEL | MB_ICONWARNING);
    switch(r) {
        case IDYES:
            if(SaveProgram())
                return FALSE;
            else
                return TRUE;

        case IDNO:
            return FALSE;

        case IDCANCEL:
            return TRUE;

        default:
            return TRUE;
            oops();
    }
    
}

//-----------------------------------------------------------------------------
// Load a new program from a file. If it succeeds then set our default filename
// to that, else we end up with an empty file then.
//-----------------------------------------------------------------------------
static void OpenDialog(void)
{
    OPENFILENAME ofn;

    char tempSaveFile[MAX_PATH] = "";

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.parentWindow = MainWindow;
    ofn.lpstrFilter = LDMICRO_PATTERN;
    ofn.lpstrDefExt = "ld";
    ofn.lpstrFile = tempSaveFile;
    ofn.nMaxFile = sizeof(tempSaveFile);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    if(!GetOpenFileName(&ofn))
        return;

    if(!LoadProjectFromFile(tempSaveFile)) {
        Error(_("Couldn't open '%s'."), tempSaveFile);
        CurrentSaveFile[0] = '\0';
    } else {
        ProgramChangedNotSaved = FALSE;
        strcpy(CurrentSaveFile, tempSaveFile);
        UndoFlush();
    }

    GenerateIoListDontLoseSelection();
    RefreshScrollbars();
    UpdateMainWindowTitleBar();
}

//-----------------------------------------------------------------------------
// Housekeeping required when the program changes: mark the program as
// changed so that we ask if user wants to save before exiting, and update
// the I/O list.
//-----------------------------------------------------------------------------
void ProgramChanged(void)
{
    ProgramChangedNotSaved = TRUE;
    GenerateIoListDontLoseSelection();
    RefreshScrollbars();
}
#define CHANGING_PROGRAM(x) { \
        UndoRemember(); \
        x; \
        ProgramChanged();\
    }*/

//-----------------------------------------------------------------------------
// Hook that we install when the user starts dragging the `splitter,' in case
// they drag it out of the narrow area of the drawn splitter bar. Resize
// the listview in response to mouse move, and unhook ourselves when they
// release the mouse button.
//-----------------------------------------------------------------------------
// static LRESULT CALLBACK MouseHook(int code, WPARAM wParam, LPARAM lParam)
// {
//     switch(code) {
//         case HC_ACTION: {
//             MSLLHOOKSTRUCT *mhs = (MSLLHOOKSTRUCT *)lParam;

//             switch(wParam) {
//                 case WM_MOUSEMOVE: {
//                     int dy = MouseY - mhs->pt.y;
                   
//                     IoListHeight += dy;
//                     if(IoListHeight < 50) IoListHeight = 50;
//                     MouseY = mhs->pt.y;
//                     MainWindowResized();

//                     break;
//                 }

//                 case WM_LBUTTONUP:
//                     UnhookWindowsHookEx(MouseHookHandle);
//                     break;
//             }
//             break;
//         }
//     }
//     return CallNextHookEx(MouseHookHandle, code, wParam, lParam);
// }

//-----------------------------------------------------------------------------
// Handle a selection from the menu bar of the main window.
//-----------------------------------------------------------------------------
// static void ProcessMenu(int code)
// {
//     if(code >= MNU_PROCESSOR_0 && code < MNU_PROCESSOR_0+NUM_SUPPORTED_MCUS) {
//         strcpy(CurrentCompileFile, "");
//         Prog.mcu = &SupportedMcus[code - MNU_PROCESSOR_0];
//         RefreshControlsToSettings();
//         return;
//     }
//     if(code == MNU_PROCESSOR_0+NUM_SUPPORTED_MCUS) {
//         Prog.mcu = NULL;
//         strcpy(CurrentCompileFile, "");
//         RefreshControlsToSettings();
//         return;
//     }

//     switch(code) {
//         case MNU_NEW:
//             if(CheckSaveUserCancels()) break;
//             NewProgram();
//             strcpy(CurrentSaveFile, "");
//             strcpy(CurrentCompileFile, "");
//             GenerateIoListDontLoseSelection();
//             RefreshScrollbars();
//             UpdateMainWindowTitleBar();
//             break;

//         case MNU_OPEN:
//             if(CheckSaveUserCancels()) break;
//             OpenDialog();
//             break;

//         case MNU_SAVE:
//             SaveProgram();
//             UpdateMainWindowTitleBar();
//             break;

//         case MNU_SAVE_AS:
//             SaveAsDialog();
//             UpdateMainWindowTitleBar();
//             break;

//         case MNU_EXPORT:
//             ExportDialog();
//             break;

//         case MNU_EXIT:
//             if(CheckSaveUserCancels()) break;
//             LD_WM_Close_call(NULL, NULL, NULL);
//             // PostQuitMessage(0);
//             break;

//         case MNU_INSERT_COMMENT:
//             CHANGING_PROGRAM(AddComment(_("--add comment here--")));
//             break;

//         case MNU_INSERT_CONTACTS:
//             CHANGING_PROGRAM(AddContact());
//             break;

//         case MNU_INSERT_COIL:
//             CHANGING_PROGRAM(AddCoil());
//             break;

//         case MNU_INSERT_TON:
//             CHANGING_PROGRAM(AddTimer(ELEM_TON));
//             break;

//         case MNU_INSERT_TOF:
//             CHANGING_PROGRAM(AddTimer(ELEM_TOF));
//             break;

//         case MNU_INSERT_RTO:
//             CHANGING_PROGRAM(AddTimer(ELEM_RTO));
//             break;

//         case MNU_INSERT_CTU:
//             CHANGING_PROGRAM(AddCounter(ELEM_CTU));
//             break;

//         case MNU_INSERT_CTD:
//             CHANGING_PROGRAM(AddCounter(ELEM_CTD));
//             break;

//         case MNU_INSERT_CTC:
//             CHANGING_PROGRAM(AddCounter(ELEM_CTC));
//             break;

//         case MNU_INSERT_RES:
//             CHANGING_PROGRAM(AddReset());
//             break;

//         case MNU_INSERT_OPEN:
//             CHANGING_PROGRAM(AddEmpty(ELEM_OPEN));
//             break;

//         case MNU_INSERT_SHORT:
//             CHANGING_PROGRAM(AddEmpty(ELEM_SHORT));
//             break;

//         case MNU_INSERT_MASTER_RLY:
//             CHANGING_PROGRAM(AddMasterRelay());
//             break;

//         case MNU_INSERT_SHIFT_REG:
//             CHANGING_PROGRAM(AddShiftRegister());
//             break;

//         case MNU_INSERT_LUT:
//             CHANGING_PROGRAM(AddLookUpTable());
//             break;
        
//         case MNU_INSERT_PWL:
//             CHANGING_PROGRAM(AddPiecewiseLinear());
//             break;
        
//         case MNU_INSERT_FMTD_STR:
//             CHANGING_PROGRAM(AddFormattedString());
//             break;

//         case MNU_INSERT_OSR:
//             CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
//             break;

//         case MNU_INSERT_OSF:
//             CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
//             break;

//         case MNU_INSERT_MOV:
//             CHANGING_PROGRAM(AddMove());
//             break;

//         case MNU_INSERT_SET_PWM:
//             CHANGING_PROGRAM(AddSetPwm());
//             break;

//         case MNU_INSERT_READ_ADC:
//             CHANGING_PROGRAM(AddReadAdc());
//             break;

//         case MNU_INSERT_UART_SEND:
//             CHANGING_PROGRAM(AddUart(ELEM_UART_SEND));
//             break;

//         case MNU_INSERT_UART_RECV:
//             CHANGING_PROGRAM(AddUart(ELEM_UART_RECV));
//             break;

//         case MNU_INSERT_PERSIST:
//             CHANGING_PROGRAM(AddPersist());
//             break;

//         {
//             int elem;
//             case MNU_INSERT_ADD: elem = ELEM_ADD; goto math;
//             case MNU_INSERT_SUB: elem = ELEM_SUB; goto math;
//             case MNU_INSERT_MUL: elem = ELEM_MUL; goto math;
//             case MNU_INSERT_DIV: elem = ELEM_DIV; goto math;
// math:
//                 CHANGING_PROGRAM(AddMath(elem));
//                 break;
//         }

//         {
//             int elem;
//             case MNU_INSERT_EQU: elem = ELEM_EQU; goto cmp;
//             case MNU_INSERT_NEQ: elem = ELEM_NEQ; goto cmp;
//             case MNU_INSERT_GRT: elem = ELEM_GRT; goto cmp;
//             case MNU_INSERT_GEQ: elem = ELEM_GEQ; goto cmp;
//             case MNU_INSERT_LES: elem = ELEM_LES; goto cmp;
//             case MNU_INSERT_LEQ: elem = ELEM_LEQ; goto cmp;
// cmp:    
//                 CHANGING_PROGRAM(AddCmp(elem));
//                 break;
//         } 

//         case MNU_MAKE_NORMAL:
//             CHANGING_PROGRAM(MakeNormalSelected());
//             break;

//         case MNU_NEGATE:
//             CHANGING_PROGRAM(NegateSelected());
//             break;

//         case MNU_MAKE_SET_ONLY:
//             CHANGING_PROGRAM(MakeSetOnlySelected());
//             break;

//         case MNU_MAKE_RESET_ONLY:
//             CHANGING_PROGRAM(MakeResetOnlySelected());
//             break;

//         case MNU_UNDO:
//             UndoUndo();
//             break;

//         case MNU_REDO:
//             UndoRedo();
//             break;

//         case MNU_INSERT_RUNG_BEFORE:
//             CHANGING_PROGRAM(InsertRung(FALSE));
//             break;

//         case MNU_INSERT_RUNG_AFTER:
//             CHANGING_PROGRAM(InsertRung(TRUE));
//             break;

//         case MNU_DELETE_RUNG:
//             CHANGING_PROGRAM(DeleteSelectedRung());
//             break;

//         case MNU_PUSH_RUNG_UP:
//             CHANGING_PROGRAM(PushRungUp());
//             break;

//         case MNU_PUSH_RUNG_DOWN:
//             CHANGING_PROGRAM(PushRungDown());
//             break;

//         case MNU_DELETE_ELEMENT:
//             CHANGING_PROGRAM(DeleteSelectedFromProgram());
//             break;

//         case MNU_MCU_SETTINGS:
//             CHANGING_PROGRAM(ShowConfDialog());
//             break;

//         case MNU_SIMULATION_MODE:
//             ToggleSimulationMode();
//             break;

//         case MNU_START_SIMULATION:
//             StartSimulation();
//             break;

//         case MNU_STOP_SIMULATION:
//             StopSimulation();
//             break;

//         case MNU_SINGLE_CYCLE:
//             SimulateOneCycle(TRUE);
//             break;

//         case MNU_COMPILE:
//             CompileProgram(FALSE);
//             break;

//         case MNU_COMPILE_AS:
//             CompileProgram(TRUE);
//             break;

//         case MNU_MANUAL:
//             ShowHelpDialog(FALSE);
//             break;

//         case MNU_ABOUT:
//             ShowHelpDialog(TRUE);
//             break;
//     }
//     gtk_widget_queue_draw(DrawWindow);
// }

// //-----------------------------------------------------------------------------
// // WndProc functions for MainWindow.
// //-----------------------------------------------------------------------------
// gboolean LD_WM_KeyDown_call(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
// {   
//     /* Handles:
//     * WM_KEYDOWN
//     */

//     UINT wParam = event->keyval;

//     if(wParam == VK_TAB) {
//         // SetFocus(IoList);
//         gtk_window_set_focus (GTK_WINDOW(MainWindow), view);
//         // BlinkCursor(0, 0, 0, 0);
        
//     }

//     if(InSimulationMode) 
//     {
//         switch(wParam) 
//         {
//             case VK_DOWN:
//                 if(ScrollYOffset < ScrollYOffsetMax)
//                     ScrollYOffset++;
//                 RefreshScrollbars();
//                 gtk_widget_queue_draw(DrawWindow);
//                 break;

//             case VK_UP:
//                 if(ScrollYOffset > 0)
//                     ScrollYOffset--;
//                 RefreshScrollbars();
//                 gtk_widget_queue_draw(DrawWindow);
//                 break;

//             case VK_LEFT:
//                 ScrollXOffset -= FONT_WIDTH;
//                 if(ScrollXOffset < 0) 
//                     ScrollXOffset = 0;
//                 RefreshScrollbars();
//                 gtk_widget_queue_draw(DrawWindow);
//                 break;

//             case VK_RIGHT:
//                 ScrollXOffset += FONT_WIDTH;
//                 if(ScrollXOffset >= ScrollXOffsetMax)
//                     ScrollXOffset = ScrollXOffsetMax;
//                 RefreshScrollbars();
//                 gtk_widget_queue_draw(DrawWindow);
//                 break;

//             case VK_RETURN:
//             case VK_ESCAPE:
//                 ToggleSimulationMode();
//                 break;
//         }
//     }

//     switch(wParam) 
//     {
//         case VK_UP:
//             if(event->state & GDK_SHIFT_MASK)
//             {
//                 CHANGING_PROGRAM(PushRungUp());
//             }
//             else
//             {
//                 MoveCursorKeyboard(wParam);
//             }

//             gtk_widget_queue_draw(DrawWindow);
//             break;

//         case VK_DOWN:
//             if(event->state & GDK_SHIFT_MASK)
//             {
//                 CHANGING_PROGRAM(PushRungDown());
//             }
//             else
//             {
//                 MoveCursorKeyboard(wParam);
//             }

//             gtk_widget_queue_draw(DrawWindow);
//             break;

//         case VK_RIGHT:
//         case VK_LEFT:
//             MoveCursorKeyboard(wParam);
//             gtk_widget_queue_draw(DrawWindow);
//             break;

//         case VK_RETURN:
//             CHANGING_PROGRAM(EditSelectedElement());
//             gtk_widget_queue_draw(DrawWindow);
//             break;

//         default:
//             break;
//     }

//     return FALSE;
// }

// gboolean LD_WM_Close_call(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_CLOSE
//     */
    
//     if(CheckSaveUserCancels())
//         return TRUE;
//     GdkRectangle allocation;
//     gtk_widget_get_allocation(GTK_WIDGET(view), &allocation); 
//     IoListHeight = allocation.height;
//     FreezeWindowPos(MainWindow);
//     FreezeDWORD(IoListHeight);
//     g_print("List Height close: %d\n",IoListHeight);

//     gtk_main_quit();
//     gdk_threads_leave();
// }

// gboolean LD_GTK_mouse_click_hook(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_LBUTTONDBLCLK, WM_LBUTTONDOWN
//     */

//     RECT Rect;
//     GetWindowRect(ScrollWindow, &Rect);
//     int wy;
//     gtk_window_get_position(GTK_WINDOW(MainWindow), NULL, &wy);

//     // g_print("net: %i\n", wy + 30 + Rect.bottom);

//     /// Identify if mouse is clicked outside drawing area
//     if (wy + 30 + Rect.bottom < event->button.y_root)
//         return FALSE;

//     GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(ScrollWindow));

//     switch(event->button.type)
//     {
//         case GDK_BUTTON_PRESS:
//             if (event->button.button == 1) /// left click
//             {
//                 GLOBAL_mouse_last_clicked_x = event->button.x_root;
//                 GLOBAL_mouse_last_clicked_y = event->button.y_root;

//                 int x = event->button.x;
//                 int y = event->button.y - 30 + gtk_adjustment_get_value(adjustment);

//                 if(!InSimulationMode) MoveCursorMouseClick(x, y);

//                 gtk_widget_queue_draw(DrawWindow);
//             }
//             break;
//         case GDK_2BUTTON_PRESS:
//             if (event->button.button == 1) /// left click
//             {
//                 GLOBAL_mouse_last_clicked_x = event->button.x_root;
//                 GLOBAL_mouse_last_clicked_y = event->button.y_root;
                
//                 int x = event->button.x;
//                 int y = event->button.y - 30 + gtk_adjustment_get_value(adjustment);

//                 if(InSimulationMode) {
//                     EditElementMouseDoubleclick(x, y);
//                 } else {
//                     CHANGING_PROGRAM(EditElementMouseDoubleclick(x, y));
//                 }
//                 gtk_widget_queue_draw(DrawWindow);
//             }
//             break;

//     }
//     return FALSE;
// }

// gboolean LD_GTK_mouse_scroll_hook(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_VSCROLL, WM_HSCROLL, WM_MOUSEWHEEL
//     */

//     GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(ScrollWindow));
    
//     switch(event->scroll.direction)
//     {
//         case GDK_SCROLL_UP:
//             if (gtk_adjustment_get_value(adjustment) == gtk_adjustment_get_lower(adjustment))
//                 VscrollProc(SB_TOP);
//             else
//                 VscrollProc(SB_LINEUP);
//             break;
//         case GDK_SCROLL_DOWN:
//             if (gtk_adjustment_get_value(adjustment) == gtk_adjustment_get_upper(adjustment) - gtk_widget_get_allocated_height (ScrollWindow))
//                 VscrollProc(SB_BOTTOM);
//             else
//                 VscrollProc(SB_LINEDOWN);
//             break;
//         case GDK_SCROLL_LEFT:
//             HscrollProc(SB_LINEUP);
//             break;
//         case GDK_SCROLL_RIGHT:
//             HscrollProc(SB_LINEDOWN);
//             break;
//         case GDK_SCROLL_SMOOTH:
//             double d_x, d_y;
//             gdk_event_get_scroll_deltas (event, &d_x, &d_y);
//             if(d_y > 0) {
//                 VscrollProc(SB_LINEUP);
//             } else {
//                 VscrollProc(SB_LINEDOWN);
//             }
//             break;

//     }

//     gtk_widget_queue_draw(DrawWindow);
//     return FALSE;
// }

// gboolean LD_WM_MouseMove_call(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_MOUSEMOVE
//     */

//     // int x = LOWORD(lParam);
//     // int y = HIWORD(lParam);

//     // if((y > (IoListTop - 9)) && (y < (IoListTop + 3))) {
//     //     SetCursor(LoadCursor(NULL, IDC_SIZENS));
//     // } else {
//     //     SetCursor(LoadCursor(NULL, IDC_ARROW));
//     // }
    
//     // int dy = MouseY - mhs->pt.y;
    
//     // int dy = MouseY - mhs->pt.y;

//     // IoListHeight += dy;
//     // if(IoListHeight < 50) IoListHeight = 50;
//     // MouseY = mhs->pt.y;
//     // MainWindowResized();

//     return FALSE;
// }

// gboolean LD_WM_Paint_call(HWID widget, HCRDC cr, gpointer data)
// {
//     /* Handles:
//     * WM_PAINT
//     */

//     static BOOL Paint_call_first = TRUE;

//     if (Paint_call_first)
//     {        
//         gtk_widget_override_background_color(GTK_WIDGET(widget), 
//                     GTK_STATE_FLAG_NORMAL, (HBRUSH)GetStockObject(BLACK_BRUSH));

//         gint width = gtk_widget_get_allocated_width (widget);
//         gint height = gtk_widget_get_allocated_height (widget);

//         gtk_widget_set_size_request(widget, width, height + 1);

//         gdk_cairo_set_source_rgba (cr, (HBRUSH)GetStockObject(BLACK_BRUSH));

//         cairo_rectangle(cr, 0, 0, width, height);
//         cairo_stroke_preserve(cr);

//         cairo_fill (cr);

//         Paint_call_first = FALSE;
//     }

//     /// This draws the schematic.
//     MainWindowResized();
//     PaintWindow(cr);

//     return FALSE;
// }

// gboolean LD_WM_Destroy_call(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_DESTROY
//     */

//     if(CheckSaveUserCancels())
//         return TRUE;

//     FreezeWindowPos(MainWindow);
//     FreezeDWORD(IoListHeight);

//     gtk_main_quit();
//     gdk_threads_leave();
// }

// gboolean LD_WM_Size_call(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_SIZE
//     */
//     MainWindowResized();
//     return FALSE;
// }

// gboolean LD_WM_Command_call(GtkMenuItem* men, gpointer gpcode)
// {
//     int tempcode = GPOINTER_TO_INT(gpcode);
//     ProcessMenu (tempcode);

//     return FALSE;
// }

// void ProcessorCall(GtkCheckMenuItem* men, gpointer gpcode)
// {
//     int tempcode = GPOINTER_TO_INT(gpcode);
//     if(gtk_check_menu_item_get_active(men))
//     {
//         ProcessMenu (tempcode);
//     }
// }

// gboolean LD_WM_SetFocus_call(GtkWidget *widget, GdkEvent *event, gpointer user_data)
// {
//     /* Handles:
//     * WM_SETFOCUS
//     */

//     InvalidateRect(DrawWindow, NULL, FALSE);

//     return FALSE;
// }

// void LD_WM_Notify_Row_Activate_call(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
// {
//     /* Handles:
//     * WM_NOTIFY
//     */

//     // g_print("Row activated!\n");

//     int *ip = gtk_tree_path_get_indices ( path );

//     NMHDR h;
//     h.code = LVN_ITEMACTIVATE;
//     h.item.iItem = ip[0];
//     h.hlistFrom = IoList;

//     IoListProc(&h);
// }

// void LD_WM_Notify_Cursor_Change_call(GtkTreeView *tree_view, gpointer user_data)
// {
//     /* Handles:
//     * WM_NOTIFY
//     */
    
//     ITLIST iter;

//     // BOOL empty = !gtk_tree_model_get_iter_first (IoList, &iter);
//     // g_print("empty = %i\n", (empty == TRUE) );

//     HLIST pTreeModel;
//     int *ip;
//     GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
//     gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
//     if(gtk_tree_selection_get_selected (selection, &pTreeModel, &iter))
//     {
//         GtkTreePath *path = gtk_tree_model_get_path ( pTreeModel , &iter ) ;
//         ip = gtk_tree_path_get_indices ( path );
//     }
//     else
//         if(!gtk_tree_model_get_iter_first (IoList, &iter))
//             return;

//     NMHDR h;
//     h.code = LVN_GETDISPINFO;
//     h.item.iItem = (ip == NULL) ? 0 : ip[0];
//     h.hlistFrom = IoList;
//     h.hlistIter = &iter;
//     IoListProc(&h);
// }

inline void MenuHandler ()
{
    QObject::connect(ExitMenu, SIGNAL(triggered()), LDmicroApp, SLOT(quit()));
    // QObject::connect(ExitMenu, SIGNAL(changed()), LDmicroApp, SLOT(aboutQt()));
//     g_signal_connect(G_OBJECT(NewMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_NEW));

//     g_signal_connect(G_OBJECT(OpenMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_OPEN));

//     g_signal_connect(G_OBJECT(SaveMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_SAVE));

//     g_signal_connect(G_OBJECT(SaveAsMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_SAVE_AS));

//     g_signal_connect(G_OBJECT(ExportMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_EXPORT));

//     g_signal_connect(G_OBJECT(ExitMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_EXIT));

//     g_signal_connect(G_OBJECT(InsertCommentMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_COMMENT));

//     g_signal_connect(G_OBJECT(InsertContactsMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_CONTACTS));

//     g_signal_connect(G_OBJECT(InsertCoilMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_COIL));

//     g_signal_connect(G_OBJECT(InsertTonMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_TON));

//     g_signal_connect(G_OBJECT(InsertTofMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_TOF));

//     g_signal_connect(G_OBJECT(InsertRtoMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_RTO));

//     g_signal_connect(G_OBJECT(InsertCtuMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_CTU));

//     g_signal_connect(G_OBJECT(InsertCtdMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_CTD));

//     g_signal_connect(G_OBJECT(InsertCtcMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_CTC));

//     g_signal_connect(G_OBJECT(InsertResMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_RES));

//     g_signal_connect(G_OBJECT(InsertOpenMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_OPEN));

//     g_signal_connect(G_OBJECT(InsertShortMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_SHORT));    

//     g_signal_connect(G_OBJECT(InsertMasterRlyMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_MASTER_RLY));

//     g_signal_connect(G_OBJECT(InsertShiftRegMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_SHIFT_REG));

//     g_signal_connect(G_OBJECT(InsertLutMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_LUT));

//     g_signal_connect(G_OBJECT(InsertPwlMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_PWL));

//     g_signal_connect(G_OBJECT(InsertFmtdStrMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_FMTD_STR));

//     g_signal_connect(G_OBJECT(InsertOsrMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_OSR));

//     g_signal_connect(G_OBJECT(InsertOsfMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_OSF));

//     g_signal_connect(G_OBJECT(InsertMovMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_MOV));

//     g_signal_connect(G_OBJECT(InsertSetPwmMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_SET_PWM));

//     g_signal_connect(G_OBJECT(InsertReadAdcMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_READ_ADC));

//     g_signal_connect(G_OBJECT(InsertUartSendMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_UART_SEND));

//     g_signal_connect(G_OBJECT(InsertUartRecvMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_UART_RECV)); 

//     g_signal_connect(G_OBJECT(InsertPersistMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_PERSIST)); 

//     g_signal_connect(G_OBJECT(InsertAddMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_ADD)); 

//     g_signal_connect(G_OBJECT(InsertSubMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_SUB)); 

//     g_signal_connect(G_OBJECT(InsertMulMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_MUL)); 

//     g_signal_connect(G_OBJECT(InsertDivMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_DIV)); 

//     g_signal_connect(G_OBJECT(InsertEquMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_EQU)); 

//     g_signal_connect(G_OBJECT(InsertNeqMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_NEQ)); 

//     g_signal_connect(G_OBJECT(InsertGrtMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_GRT)); 

//     g_signal_connect(G_OBJECT(InsertGeqMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_GEQ)); 

//     g_signal_connect(G_OBJECT(InsertLesMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_LES)); 

//     g_signal_connect(G_OBJECT(InsertLeqMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_LEQ)); 

//     g_signal_connect(G_OBJECT(MakeNormalMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_MAKE_NORMAL)); 

//     g_signal_connect(G_OBJECT(NegateMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_NEGATE)); 

//     g_signal_connect(G_OBJECT(MakeSetOnlyMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_MAKE_SET_ONLY)); 

//     g_signal_connect(G_OBJECT(MakeResetOnlyMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_MAKE_RESET_ONLY)); 

//     g_signal_connect(G_OBJECT(UndoMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_UNDO)); 

//     g_signal_connect(G_OBJECT(RedoMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_REDO)); 

//     g_signal_connect(G_OBJECT(InsertRungBeforeMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_RUNG_BEFORE)); 

//     g_signal_connect(G_OBJECT(InsertRungAfterMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_INSERT_RUNG_AFTER)); 

//     g_signal_connect(G_OBJECT(DeleteRungMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_DELETE_RUNG)); 

//     g_signal_connect(G_OBJECT(PushRungUpMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_PUSH_RUNG_UP)); 

//     g_signal_connect(G_OBJECT(PushRungDownMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_PUSH_RUNG_DOWN)); 

//     g_signal_connect(G_OBJECT(DeleteElementMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_DELETE_ELEMENT)); 

//     g_signal_connect(G_OBJECT(McuSettingsMenu), "activate", 
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_MCU_SETTINGS));

//     g_signal_connect(G_OBJECT(SimulationModeMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_SIMULATION_MODE)); 

//     g_signal_connect(G_OBJECT(StartSimulationMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_START_SIMULATION)); 

//     g_signal_connect(G_OBJECT(StopSimulationMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_STOP_SIMULATION)); 

//     g_signal_connect(G_OBJECT(SingleCycleMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_SINGLE_CYCLE)); 

//     g_signal_connect(G_OBJECT(CompileMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_COMPILE)); 

//     g_signal_connect(G_OBJECT(CompileAsMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_COMPILE_AS)); 

//     g_signal_connect(G_OBJECT(ManualMenu), "activate",  
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_MANUAL));
 
//     g_signal_connect(G_OBJECT(AboutMenu), "activate",
//         G_CALLBACK(LD_WM_Command_call), GINT_TO_POINTER(MNU_ABOUT));
//     // Connect microcontroller signals automatically
//     for(int i = 0; i < NUM_SUPPORTED_MCUS; i++)
//     {
//         g_signal_connect(G_OBJECT(ProcessorMenuItems[i]), "toggled",
//             G_CALLBACK(ProcessorCall), GINT_TO_POINTER((MNU_PROCESSOR_0 + i)));
//     } 
}

//-----------------------------------------------------------------------------
// Entry point into the program.
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    /// Check if we're running in non-interactive mode; in that case we should
    /// load the file, compile, and exit.
    if(argc >= 2) {
        RunningInBatchMode = TRUE;
        char *err =
            "Bad command line arguments: run 'ldmicro /c src.ld dest.hex'\n";
        if(argc < 4)
        {
            printf(err); 
            exit(-1); 
        }
        char *source = (char*)malloc(strlen(argv[2]) + strlen(argv[3]) + 2);
        sprintf(source, "%s %s", argv[2], argv[3]);

        while(isspace(*source)) {
            source++;
        }
        if(*source == '\0') 
        { 
            Error(err); 
            free(source);
            exit(-1); 
        }

        char *dest = source;
        while(!isspace(*dest) && *dest) {
            dest++;
        }
        if(*dest == '\0') 
        { 
            Error(err); 
            free(source);
            exit(-1); 
        }
        *dest = '\0'; dest++;
        while(isspace(*dest)) {
            dest++;
        }

        if(*dest == '\0') 
        { 
            Error(err); 
            free(source);
            exit(-1); 
        }

        if(!LoadProjectFromFile(source)) {
            Error("Couldn't open '%s', running non-interactively.\n", source);
            free(source);
            exit(-1);
        }
        strcpy(CurrentCompileFile, dest);
        GenerateIoList(-1);
        CompileProgram(FALSE);
        exit(0);
    }
    
    //Basic qt application
    ::QApplication app(argc, argv);
    LDmicroApp = &app;
    MainHeap = HeapCreate(0, 1024*64, 0);
    QSize MwSize(800,600);

    // Make main window
    MainWindow = new QWidget;
    MWIcon = new QIcon(LDMICRO_ICON);
    MainMenu = new QMenuBar(MainWindow);

    // QMenu TopMenu("Top Menu", MainWindow);
    // MainMenu->addMenu(&TopMenu);
    // MainWindow->layout()->setMenuBar(MainMenu);
    MainWindow->setWindowTitle("LDmicro");
    MainWindow->resize(MwSize);
    MainWindow->move( 10, 10);
    // MainWindow->setStyleSheet("background-color: black;");
    //Default Icon for entire app
    app.setWindowIcon(*MWIcon);
    //Icon for main window
    MainWindow->setWindowIcon(*MWIcon);
    MakeMainWindowMenus();
    MakeDialogBoxClass();
    InitForDrawing();

    ThawWindowPos(MainWindow);
    IoListHeight = 100;
    ThawDWORD(IoListHeight);
    
    MakeMainWindowControls();
    MainWindowResized();

    MenuHandler();
    
    NewProgram();
    strcpy(CurrentSaveFile, "");

    MainWindow->show();
    
    GenerateIoListDontLoseSelection(); 
    
    // MakeDialogBoxClass();


    /*ThawWindowPos(MainWindow);
    ThawDWORD(IoListHeight);
    g_print("IoListHeight start: %d\n", IoListHeight);
    MakeMainWindowControls(); /// takes care of MakeMainWindowMenus()
    MainWindowResized();

    /// Keyboard and mouse hooks equivalent to MainWndProc
    g_signal_connect (MainWindow, "delete_event", G_CALLBACK (LD_WM_Close_call), NULL);
    g_signal_connect (MainWindow, "key_press_event", G_CALLBACK (LD_WM_KeyDown_call), NULL);
    g_signal_connect (MainWindow, "button_press_event", G_CALLBACK (LD_GTK_mouse_click_hook), NULL);
    g_signal_connect (MainWindow, "scroll_event", G_CALLBACK (LD_GTK_mouse_scroll_hook), NULL);
    g_signal_connect (MainWindow, "motion_notify_event", G_CALLBACK (LD_WM_MouseMove_call), NULL);
    g_signal_connect (DrawWindow, "draw", G_CALLBACK (LD_WM_Paint_call), NULL);
    g_signal_connect (MainWindow, "destroy_event", G_CALLBACK (LD_WM_Destroy_call), NULL);
    g_signal_connect (MainWindow, "configure_event", G_CALLBACK (LD_WM_Size_call), NULL);
    g_signal_connect (MainWindow, "focus_in_event", G_CALLBACK (LD_WM_SetFocus_call), NULL);
    g_signal_connect (view, "row_activated", G_CALLBACK (LD_WM_Notify_Row_Activate_call), NULL);
    g_signal_connect (view, "cursor_changed", G_CALLBACK (LD_WM_Notify_Cursor_Change_call), NULL);
    MenuHandler();
    /// Keyboard and mouse hooks equivalent to MainWndProc - end

    NewProgram();
    strcpy(CurrentSaveFile, "");

    /// We are running interactively, or we would already have exited. We
    /// can therefore show the window now, and otherwise set up the GUI.

    /// Displaying the window
    gtk_widget_show_all(MainWindow);

    /// Blink cursor
    SetTimer(DrawWindow, TIMER_BLINK_CURSOR, 200, BlinkCursor);
    // SetTimer(MainWindow, TIMER_BLINK_CURSOR, 800, BlinkCursor);
    */
    /*if(argc >= 2) {
        char line[MAX_PATH];
        if(*argv[1] == '"') { 
            strcpy(line, argv[1]+1);
        } else {
            strcpy(line, argv[1]);
        }
        if(strchr(line, '"')) *strchr(line, '"') = '\0';
        
        realpath(line, CurrentSaveFile);
        if(!LoadProjectFromFile(CurrentSaveFile)) {
            NewProgram();
            Error(_("Couldn't open '%s'."), CurrentSaveFile);
            CurrentSaveFile[0] = '\0';
        }
        UndoFlush();
    }*/

    /*GenerateIoListDontLoseSelection(); 
    RefreshScrollbars();
    UpdateMainWindowTitleBar();
*/
    // MSG msg;
    // DWORD ret;
    // while(ret = GetMessage(&msg, NULL, 0, 0)) {
    //     if(msg.hwnd == IoList && msg.message == WM_KEYDOWN) {
    //         if(msg.wParam == VK_TAB) {
    //             SetFocus(MainWindow);
    //             continue;
    //         }
    //     }
    //     if(msg.message == WM_KEYDOWN && msg.wParam != VK_UP &&
    //         msg.wParam != VK_DOWN && msg.wParam != VK_RETURN && msg.wParam
    //         != VK_SHIFT)
    //     {
    //         if(msg.hwnd == IoList) {
    //             msg.hwnd = MainWindow;
    //             SetFocus(MainWindow);
    //         }
    //     }
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }
    
/*    gtk_main();
    return EXIT_SUCCESS;*/
    return app.exec();
    delete MainWindow;
    delete MWIcon;
}