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

HINSTANCE       Instance;
QApplication*   LDmicroApp;
HWID            MainWindow;
QIcon*          MWIcon;
QMenuBar*       MainMenu;
QGroupBox*      CursorObject;
HWID            DrawWindow;
WM_SCROLL       scrollbar;

static int         MouseY;
int CursorTimer;
int SimulateTimer;

ProgramSlots MenuHandle;

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

//-----------------------------------------------------------------------------
// Get a filename with a common dialog box and then save the program to that
// file and then set our default filename to that.
//-----------------------------------------------------------------------------
static BOOL SaveAsDialog(void)
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
}

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
        ofn.parentWindow = MainWindow;
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
BOOL CheckSaveUserCancels(void)
{
    if(!ProgramChangedNotSaved) {
        // no problem
        return FALSE;
    }

    int r = MessageBox(MainWindow, 
        _("The program has changed since it was last saved.\r\n\r\n"
        "Do you want to save the changes?"), "LDmicro",
        MB_YESNOCANCEL , MB_ICONWARNING);
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
    DrawWindow->repaint();

    GenerateIoListDontLoseSelection();
    // RefreshScrollbars();
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
    // RefreshScrollbars();
}

#define CHANGING_PROGRAM(x) { \
        UndoRemember(); \
        x; \
        ProgramChanged();\
    }

//-----------------------------------------------------------------------------
// Handle a selection from the menu bar of the main window.
//-----------------------------------------------------------------------------
static void ProcessMenu(int code)
{
    if(code >= MNU_PROCESSOR_0 && code < MNU_PROCESSOR_0+NUM_SUPPORTED_MCUS) {
        strcpy(CurrentCompileFile, "");
        Prog.mcu = &SupportedMcus[code - MNU_PROCESSOR_0];
        RefreshControlsToSettings();
        return;
    }
    if(code == MNU_PROCESSOR_0+NUM_SUPPORTED_MCUS) {
        Prog.mcu = NULL;
        strcpy(CurrentCompileFile, "");
        RefreshControlsToSettings();
        return;
    }

    switch(code) {
        case MNU_NEW:
            if(CheckSaveUserCancels()) break;
            NewProgram();
            DrawWindow->resize(scrollbar->size());
            strcpy(CurrentSaveFile, "");
            strcpy(CurrentCompileFile, "");
            GenerateIoListDontLoseSelection();
            // RefreshScrollbars();
            UpdateMainWindowTitleBar();
            break;

        case MNU_OPEN:
            DrawWindow->resize(scrollbar->size());
            if(CheckSaveUserCancels()) break;
            OpenDialog();
            break;

        case MNU_SAVE:
            SaveProgram();
            UpdateMainWindowTitleBar();
            break;

        case MNU_SAVE_AS:
            SaveAsDialog();
            UpdateMainWindowTitleBar();
            break;

        case MNU_EXPORT:
            ExportDialog();
            break;

        case MNU_EXIT:
            if(CheckSaveUserCancels()) break;
            LDmicroApp->exit();
            break;

        case MNU_INSERT_COMMENT:
            CHANGING_PROGRAM(AddComment(_("--add comment here--")));
            break;

        case MNU_INSERT_CONTACTS:
            CHANGING_PROGRAM(AddContact());
            break;

        case MNU_INSERT_CONTACTS_N:
            CHANGING_PROGRAM(AddContactNeg());
            break;

        case MNU_INSERT_COIL:
            CHANGING_PROGRAM(AddCoil());
            break;

        case MNU_INSERT_COIL_N:
            CHANGING_PROGRAM(AddCoilNeg());
            break;

        case MNU_INSERT_COIL_S:
            CHANGING_PROGRAM(AddCoilSet());
            break;

        case MNU_INSERT_COIL_R:
            CHANGING_PROGRAM(AddCoilRes());
            break;
        case MNU_INSERT_TON:
            CHANGING_PROGRAM(AddTimer(ELEM_TON));
            break;

        case MNU_INSERT_TOF:
            CHANGING_PROGRAM(AddTimer(ELEM_TOF));
            break;

        case MNU_INSERT_RTO:
            CHANGING_PROGRAM(AddTimer(ELEM_RTO));
            break;

        case MNU_INSERT_CTU:
            CHANGING_PROGRAM(AddCounter(ELEM_CTU));
            break;

        case MNU_INSERT_CTD:
            CHANGING_PROGRAM(AddCounter(ELEM_CTD));
            break;

        case MNU_INSERT_CTC:
            CHANGING_PROGRAM(AddCounter(ELEM_CTC));
            break;

        case MNU_INSERT_RES:
            CHANGING_PROGRAM(AddReset());
            break;

        case MNU_INSERT_OPEN:
            CHANGING_PROGRAM(AddEmpty(ELEM_OPEN));
            break;

        case MNU_INSERT_SHORT:
            CHANGING_PROGRAM(AddEmpty(ELEM_SHORT));
            break;

        case MNU_INSERT_MASTER_RLY:
            CHANGING_PROGRAM(AddMasterRelay());
            break;

        case MNU_INSERT_SHIFT_REG:
            CHANGING_PROGRAM(AddShiftRegister());
            break;

        case MNU_INSERT_LUT:
            CHANGING_PROGRAM(AddLookUpTable());
            break;
        
        case MNU_INSERT_PWL:
            CHANGING_PROGRAM(AddPiecewiseLinear());
            break;
        
        case MNU_INSERT_FMTD_STR:
            CHANGING_PROGRAM(AddFormattedString());
            break;

        case MNU_INSERT_OSR:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_RISING));
            break;

        case MNU_INSERT_OSF:
            CHANGING_PROGRAM(AddEmpty(ELEM_ONE_SHOT_FALLING));
            break;

        case MNU_INSERT_MOV:
            CHANGING_PROGRAM(AddMove());
            break;

        case MNU_INSERT_SET_PWM:
            CHANGING_PROGRAM(AddSetPwm());
            break;

        case MNU_INSERT_READ_ADC:
            CHANGING_PROGRAM(AddReadAdc());
            break;

        case MNU_INSERT_UART_SEND:
            CHANGING_PROGRAM(AddUart(ELEM_UART_SEND));
            break;

        case MNU_INSERT_UART_RECV:
            CHANGING_PROGRAM(AddUart(ELEM_UART_RECV));
            break;

        case MNU_INSERT_PERSIST:
            CHANGING_PROGRAM(AddPersist());
            break;

        {
            int elem;
            case MNU_INSERT_ADD: elem = ELEM_ADD; goto math;
            case MNU_INSERT_SUB: elem = ELEM_SUB; goto math;
            case MNU_INSERT_MUL: elem = ELEM_MUL; goto math;
            case MNU_INSERT_DIV: elem = ELEM_DIV; goto math;
math:
                CHANGING_PROGRAM(AddMath(elem));
                break;
        }

        {
            int elem;
            case MNU_INSERT_EQU: elem = ELEM_EQU; goto cmp;
            case MNU_INSERT_NEQ: elem = ELEM_NEQ; goto cmp;
            case MNU_INSERT_GRT: elem = ELEM_GRT; goto cmp;
            case MNU_INSERT_GEQ: elem = ELEM_GEQ; goto cmp;
            case MNU_INSERT_LES: elem = ELEM_LES; goto cmp;
            case MNU_INSERT_LEQ: elem = ELEM_LEQ; goto cmp;
cmp:    
                CHANGING_PROGRAM(AddCmp(elem));
                break;
        } 

        case MNU_MAKE_NORMAL:
            CHANGING_PROGRAM(MakeNormalSelected());
            break;

        case MNU_NEGATE:
            CHANGING_PROGRAM(NegateSelected());
            break;

        case MNU_MAKE_SET_ONLY:
            CHANGING_PROGRAM(MakeSetOnlySelected());
            break;

        case MNU_MAKE_RESET_ONLY:
            CHANGING_PROGRAM(MakeResetOnlySelected());
            break;

        case MNU_UNDO:
            UndoUndo();
            break;

        case MNU_REDO:
            UndoRedo();
            break;

        case MNU_INSERT_RUNG_BEFORE:
            CHANGING_PROGRAM(InsertRung(FALSE));
            break;

        case MNU_INSERT_RUNG_AFTER:
            CHANGING_PROGRAM(InsertRung(TRUE));
            break;

        case MNU_DELETE_RUNG:
            CHANGING_PROGRAM(DeleteSelectedRung());
            break;

        case MNU_PUSH_RUNG_UP:
            CHANGING_PROGRAM(PushRungUp());
            break;

        case MNU_PUSH_RUNG_DOWN:
            CHANGING_PROGRAM(PushRungDown());
            break;

        case MNU_DELETE_ELEMENT:
            CHANGING_PROGRAM(DeleteSelectedFromProgram());
            break;

        case MNU_MCU_SETTINGS:
            CHANGING_PROGRAM(ShowConfDialog());
            break;

        case MNU_SIMULATION_MODE:
            ToggleSimulationMode();
            break;

        case MNU_START_SIMULATION:
            StartSimulation();
            break;

        case MNU_STOP_SIMULATION:
            StopSimulation();
            break;

        case MNU_SINGLE_CYCLE:
            SimulateOneCycle(TRUE);
            break;

        case MNU_COMPILE:
            CompileProgram(FALSE);
            break;

        case MNU_COMPILE_AS:
            CompileProgram(TRUE);
            break;

        case MNU_MANUAL:
            ShowHelpDialog(FALSE);
            break;

        case MNU_ABOUT:
            ShowHelpDialog(TRUE);
            break;
    }
    // gtk_widget_queue_draw(DrawWindow);
    DrawWindow->repaint();
}

// //-----------------------------------------------------------------------------
// // WndProc functions for MainWindow.
// //-----------------------------------------------------------------------------

void PaintWidget::keyPressEvent(QKeyEvent* event)
{
        int wParam = event->key();

    if(InSimulationMode) 
    {
        switch(wParam) 
        {
            case VK_DOWN:
                // RefreshScrollbars();
                DrawWindow->repaint();
                break;

            case VK_UP:
                // RefreshScrollbars();
                DrawWindow->repaint();
                break;

            case VK_LEFT:
                DrawWindow->repaint();
                break;

            case VK_RIGHT:
                DrawWindow->repaint();
                break;

            case VK_RETURN:
            case VK_ESCAPE:
                ToggleSimulationMode();
                break;
        }
    }

    switch(wParam) 
    {
        case VK_UP:
            if(event->modifiers() & Qt::ShiftModifier)
            {
                CHANGING_PROGRAM(PushRungUp());
            }
            else
            {
                MoveCursorKeyboard(wParam);
            }

            DrawWindow->repaint();
            break;

        case VK_DOWN:
            if(event->modifiers() & Qt::ShiftModifier)
            {
                CHANGING_PROGRAM(PushRungDown());
            }
            else
            {
                MoveCursorKeyboard(wParam);
            }

            DrawWindow->repaint();
            break;

        case VK_RIGHT:
        case VK_LEFT:
            MoveCursorKeyboard(wParam);
            DrawWindow->repaint();
            break;

        case VK_RETURN:
            CHANGING_PROGRAM(EditSelectedElement());
            DrawWindow->repaint();
            break;

        default:
            break;
    }

    return;
}

void MyWidget::resizeEvent(QResizeEvent *event)
{
    DrawWindow->resize(scrollbar->size());
}

void MyWidget::closeEvent(QCloseEvent* event)
{
    if(CheckSaveUserCancels())
        event->ignore();
    else
    {
        event->accept();
        FreezeWindowPos(MainWindow);
        IoListHeight = IoList->height();
        FreezeDWORD(IoListHeight);
    }
}

void PaintWidget :: mouseReleaseEvent(QMouseEvent* event)
{
    /* Handles:
    * WM_LBUTTONDBLCLK, WM_LBUTTONDOWN
    */

    QRect Rect;
    Rect = DrawWindow->rect();
    QPoint wy = event->pos();

    if((wy.x() <= 0) || (wy.y() <= 0))
        return;

    //No need to identify if mouse is outside the scope since
    //the function is not called

    switch(event->button())
    {
        case Qt::LeftButton:

                GLOBAL_mouse_last_clicked_x = event->x();
                GLOBAL_mouse_last_clicked_y = event->y();

                if(!InSimulationMode) MoveCursorMouseClick(wy.x(), wy.y());

                DrawWindow->repaint();

            break;
}
}
void PaintWidget :: mouseDoubleClickEvent(QMouseEvent* event)
{
    QRect Rect;
    Rect = DrawWindow->rect();
    QPoint wy = event->pos();

    if((wy.x() <= 0) || (wy.y() <= 0))
        return;
    switch (event->button())
    {
        case Qt::LeftButton:
                GLOBAL_mouse_last_clicked_x = event->x();
                GLOBAL_mouse_last_clicked_y = event->y();

                if(InSimulationMode) {
                    EditElementMouseDoubleclick(wy.x(), wy.y());
                } else {
                    CHANGING_PROGRAM(EditElementMouseDoubleclick(wy.x(), wy.y()));
                }
                DrawWindow->repaint();
            break;

    }

}

void ProgramSlots :: LD_WM_Command_call(int CommandCode)
{
    ProcessMenu(CommandCode);
}

inline void MenuHandler ()
{
    QSignalMapper* CommandMapper = new QSignalMapper (&MenuHandle);

    //Create mappings for each menu item
    CommandMapper->setMapping(NewMenu, MNU_NEW);
    CommandMapper->setMapping(OpenMenu, MNU_OPEN);
    CommandMapper->setMapping(SaveMenu, MNU_SAVE);
    CommandMapper->setMapping(SaveAsMenu, MNU_SAVE_AS);
    CommandMapper->setMapping(ExportMenu, MNU_EXPORT);
    CommandMapper->setMapping(ExitMenu, MNU_EXIT);
    CommandMapper->setMapping(InsertCommentMenu, MNU_INSERT_COMMENT);
    CommandMapper->setMapping(InsertContactsMenu, MNU_INSERT_CONTACTS);
    CommandMapper->setMapping(InsertCoilMenu, MNU_INSERT_COIL);
    CommandMapper->setMapping(InsertTonMenu, MNU_INSERT_TON);
    CommandMapper->setMapping(InsertTofMenu, MNU_INSERT_TOF);
    CommandMapper->setMapping(InsertRtoMenu, MNU_INSERT_RTO);
    CommandMapper->setMapping(InsertCtuMenu, MNU_INSERT_CTU);
    CommandMapper->setMapping(InsertCtdMenu, MNU_INSERT_CTD);
    CommandMapper->setMapping(InsertCtcMenu, MNU_INSERT_CTC);
    CommandMapper->setMapping(InsertResMenu, MNU_INSERT_RES);
    CommandMapper->setMapping(InsertOpenMenu, MNU_INSERT_OPEN);
    CommandMapper->setMapping(InsertShortMenu, MNU_INSERT_SHORT);
    CommandMapper->setMapping(InsertMasterRlyMenu, MNU_INSERT_MASTER_RLY);
    CommandMapper->setMapping(InsertShiftRegMenu, MNU_INSERT_SHIFT_REG);
    CommandMapper->setMapping(InsertLutMenu, MNU_INSERT_LUT);
    CommandMapper->setMapping(InsertPwlMenu, MNU_INSERT_PWL);
    CommandMapper->setMapping(InsertFmtdStrMenu, MNU_INSERT_FMTD_STR);
    CommandMapper->setMapping(InsertOsrMenu, MNU_INSERT_OSR);
    CommandMapper->setMapping(InsertOsfMenu, MNU_INSERT_OSF);
    CommandMapper->setMapping(InsertMovMenu, MNU_INSERT_MOV);
    CommandMapper->setMapping(InsertSetPwmMenu, MNU_INSERT_SET_PWM);
    CommandMapper->setMapping(InsertReadAdcMenu, MNU_INSERT_READ_ADC);
    CommandMapper->setMapping(InsertUartSendMenu, MNU_INSERT_UART_SEND);
    CommandMapper->setMapping(InsertUartRecvMenu, MNU_INSERT_UART_RECV);
    CommandMapper->setMapping(InsertPersistMenu,  MNU_INSERT_PERSIST);
    CommandMapper->setMapping(InsertAddMenu, MNU_INSERT_ADD);
    CommandMapper->setMapping(InsertSubMenu, MNU_INSERT_SUB);
    CommandMapper->setMapping(InsertMulMenu, MNU_INSERT_MUL);
    CommandMapper->setMapping(InsertDivMenu, MNU_INSERT_DIV);
    CommandMapper->setMapping(InsertEquMenu, MNU_INSERT_EQU);
    CommandMapper->setMapping(InsertNeqMenu, MNU_INSERT_NEQ);
    CommandMapper->setMapping(InsertGrtMenu, MNU_INSERT_GRT);
    CommandMapper->setMapping(InsertGeqMenu, MNU_INSERT_GEQ);
    CommandMapper->setMapping(InsertLesMenu, MNU_INSERT_LES);
    CommandMapper->setMapping(InsertLeqMenu, MNU_INSERT_LEQ);
    CommandMapper->setMapping(MakeNormalMenu, MNU_MAKE_NORMAL);
    CommandMapper->setMapping(NegateMenu, MNU_NEGATE);
    CommandMapper->setMapping(MakeSetOnlyMenu, MNU_MAKE_SET_ONLY);
    CommandMapper->setMapping(MakeResetOnlyMenu, MNU_MAKE_RESET_ONLY);
    CommandMapper->setMapping(UndoMenu, MNU_UNDO);
    CommandMapper->setMapping(RedoMenu, MNU_REDO);
    CommandMapper->setMapping(InsertRungBeforeMenu, MNU_INSERT_RUNG_BEFORE);
    CommandMapper->setMapping(InsertRungAfterMenu, MNU_INSERT_RUNG_AFTER);
    CommandMapper->setMapping(DeleteRungMenu, MNU_DELETE_RUNG);
    CommandMapper->setMapping(PushRungUpMenu, MNU_PUSH_RUNG_UP);
    CommandMapper->setMapping(PushRungDownMenu, MNU_PUSH_RUNG_DOWN);
    CommandMapper->setMapping(DeleteElementMenu, MNU_DELETE_ELEMENT);
    CommandMapper->setMapping(McuSettingsMenu, MNU_MCU_SETTINGS);
    CommandMapper->setMapping(SimulationModeMenu, MNU_SIMULATION_MODE);
    CommandMapper->setMapping(StartSimulationMenu, MNU_START_SIMULATION);
    CommandMapper->setMapping(StopSimulationMenu, MNU_STOP_SIMULATION);
    CommandMapper->setMapping(SingleCycleMenu, MNU_SINGLE_CYCLE);
    CommandMapper->setMapping(CompileMenu, MNU_COMPILE);
    CommandMapper->setMapping(CompileAsMenu, MNU_COMPILE_AS);
    CommandMapper->setMapping(ManualMenu, MNU_MANUAL);
    CommandMapper->setMapping(AboutMenu, MNU_ABOUT);

    QObject::connect(NewMenu, SIGNAL(triggered()), CommandMapper, SLOT(map()));
    QObject::connect(OpenMenu, SIGNAL(triggered()), CommandMapper, SLOT(map()));
    QObject::connect(SaveMenu, SIGNAL(triggered()), CommandMapper, SLOT(map()));
    QObject::connect(SaveAsMenu, SIGNAL(triggered()), CommandMapper, SLOT(map()));
    QObject::connect(ExportMenu, SIGNAL(triggered()), CommandMapper, SLOT(map()));
    QObject::connect(ExitMenu, SIGNAL(triggered()), CommandMapper, SLOT(map()));

    // QObject::connect(ExitMenu, SIGNAL(triggered()), LDmicroApp, SLOT(quit()));
    QObject::connect(InsertCommentMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertContactsMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertCoilMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertTonMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertTofMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertRtoMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertCtuMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertCtdMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertCtcMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertResMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertOpenMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertShortMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertMasterRlyMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertShiftRegMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertLutMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertPwlMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertFmtdStrMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertOsrMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertOsfMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertMovMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertSetPwmMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertReadAdcMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertUartSendMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertUartRecvMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertPersistMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertAddMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertSubMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertMulMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertDivMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertEquMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertNeqMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertGrtMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertGeqMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertLesMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertLeqMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(MakeNormalMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(NegateMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(MakeSetOnlyMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(MakeResetOnlyMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(UndoMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(RedoMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertRungBeforeMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(InsertRungAfterMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(DeleteRungMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(PushRungUpMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(PushRungDownMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(DeleteElementMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(McuSettingsMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(SimulationModeMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(StartSimulationMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(StopSimulationMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(SingleCycleMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(CompileMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(CompileAsMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(ManualMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    QObject::connect(AboutMenu, SIGNAL(triggered()),
        CommandMapper, SLOT(map()));

    // Connect microcontroller signals automatically
    for(int i = 0; i < NUM_SUPPORTED_MCUS; i++)
    {
        CommandMapper->setMapping(ProcessorMenuItems[i], (MNU_PROCESSOR_0 + i));
        QObject::connect(ProcessorMenuItems[i], SIGNAL(triggered()),
        CommandMapper, SLOT(map()));
    } 

    //Connect map to combined function call
    QObject::connect (CommandMapper, SIGNAL(mapped(int)),
        &MenuHandle, SLOT(LD_WM_Command_call(int))) ;
}
void ActivateItem(QTreeWidgetItem* item, int column)
{
    NMHDR h;
    h.code = LVN_ITEMACTIVATE;
    h.hlistFrom = IoList;
    h.item.iItem = IoList->indexOfTopLevelItem(item);
    QTreeWidgetItem* selection;
    IoListProc(&h);
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
    MainWindow = new MyWidget();
    MWIcon = new QIcon(LDMICRO_ICON);
    MainMenu = new QMenuBar(MainWindow);
    
    // Initialize cursor and set color
    CursorObject = new QGroupBox(DrawWindow);

    MainWindow->setWindowTitle("LDmicro");
    MainWindow->resize(MwSize);
    MainWindow->move( 10, 10);

    app.setWindowIcon(*MWIcon);
    //Icon for main window
    MainWindow->setWindowIcon(*MWIcon);
    CreateToolBar();
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
    
    /// Blink cursor
    CursorTimer = SetTimer(DrawWindow, TIMER_BLINK_CURSOR, 500, CursorTimer);

    GenerateIoListDontLoseSelection(); 

    UpdateMainWindowTitleBar();
    QObject::connect(IoList, &QTreeWidget::itemActivated, ActivateItem);

    return app.exec();
    delete MainWindow;
    delete MWIcon;
}