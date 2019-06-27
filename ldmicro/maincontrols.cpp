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
// Common controls in the main window. The main window consists of the drawing
// area, where the ladder diagram is displayed, plus various controls for
// scrolling, I/O list, menus.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "linuxUI.h"
#include <typeinfo>
//#include <commctrl.h>
//#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <QKeySequence>
#include <QStatusBar>
#include "ldmicro.h"

// status bar at the bottom of the screen, to display settings
static QLabel*        StatusBar[3];

// have to get back to the menus to gray/ungray, check/uncheck things
static HMENU        FileMenu;
static HMENU        EditMenu;
static HMENU        InstructionMenu;
static HMENU        ProcessorMenu;
static HMENU        SimulateMenu;
static HMENU        TopMenu;
static HMENU        Settings;
static HMENU        Compile;
static HMENU        Help;
HMENU               ScrollWindow;

// Menu IDs
QAction* NewMenu;
QAction* OpenMenu;
QAction* SaveMenu;
QAction* SaveAsMenu;
QAction* ExportMenu;
QAction* ExitMenu;

QAction* UndoMenu;
QAction* RedoMenu;
QAction* PushRungUpMenu;
QAction* PushRungDownMenu;
QAction* InsertRungBeforeMenu;
QAction* InsertRungAfterMenu;
QAction* DeleteElementMenu;
QAction* DeleteRungMenu;

QAction* InsertCommentMenu;
QAction* InsertContactsMenu;
QAction* InsertCoilMenu;
QAction* InsertTonMenu;
QAction* InsertTofMenu;
QAction* InsertRtoMenu;
QAction* InsertResMenu;
QAction* InsertOsrMenu;
QAction* InsertOsfMenu;
QAction* InsertCtuMenu;
QAction* InsertCtdMenu;
QAction* InsertCtcMenu;
QAction* InsertAddMenu;
QAction* InsertSubMenu;
QAction* InsertMulMenu;
QAction* InsertDivMenu;
QAction* InsertMovMenu;
QAction* InsertReadAdcMenu;
QAction* InsertSetPwmMenu;
QAction* InsertUartSendMenu;
QAction* InsertUartRecvMenu;
QAction* InsertEquMenu;
QAction* InsertNeqMenu;
QAction* InsertGrtMenu;
QAction* InsertGeqMenu;
QAction* InsertLesMenu;
QAction* InsertLeqMenu;
QAction* InsertOpenMenu;
QAction* InsertShortMenu;
QAction* InsertMasterRlyMenu;
QAction* InsertShiftRegMenu;
QAction* InsertLutMenu;
QAction* InsertFmtdStrMenu;
QAction* InsertPersistMenu;
QAction* MakeNormalMenu;
QAction* NegateMenu;
QAction* MakeSetOnlyMenu;
QAction* MakeResetOnlyMenu;
QAction* InsertPwlMenu;

QAction* McuSettingsMenu;
QAction* ProcessorMenuItems[NUM_SUPPORTED_MCUS+1];
HMENU MicroControllerMenu;

QAction* SimulationModeMenu;
QAction* StartSimulationMenu;
QAction* StopSimulationMenu;
QAction* SingleCycleMenu;

QAction* CompileMenu;
QAction* CompileAsMenu;

QAction* ManualMenu;
QAction* AboutMenu;

QActionGroup* ProcessorMenuGroup;

// scrollbars for the ladder logic area
// static HWND         HorizScrollBar;
// static HWND         VertScrollBar;
int                 ScrollWidth;
int                 ScrollHeight;
BOOL                NeedHoriz;

// listview used to maintain the list of I/O pins with symbolic names, plus
// the internal relay too
HLIST               IoList;
static int          IoListSelectionPoint;
static BOOL         IoListOutOfSync;
UINT                IoListHeight = 100;
int                 IoListTop;

// whether the simulation is running in real time
static BOOL         RealTimeSimulationRunning;

//-----------------------------------------------------------------------------
// Ubuntu 14 on Vidyut does not handle Menu Shortcuts.
// This function adds corresponding action for the shortcuts to the MainWindow
//-----------------------------------------------------------------------------
#if defined(__UBUNTU_14_)
	inline void SetKeyboardShortcuts()
	{
		MainWindow->addAction(NewMenu);
		MainWindow->addAction(OpenMenu);
		MainWindow->addAction(SaveMenu);
		MainWindow->addAction(SaveAsMenu);
		MainWindow->addAction(ExportMenu);
		MainWindow->addAction(UndoMenu);
		MainWindow->addAction(RedoMenu);
		MainWindow->addAction(InsertRungBeforeMenu);
		MainWindow->addAction(InsertRungAfterMenu);
		MainWindow->addAction(PushRungUpMenu);
		MainWindow->addAction(PushRungDownMenu);
		MainWindow->addAction(DeleteElementMenu);
		MainWindow->addAction(DeleteRungMenu);
		MainWindow->addAction(InsertCommentMenu);
		MainWindow->addAction(InsertContactsMenu);
		MainWindow->addAction(InsertOsrMenu);
		MainWindow->addAction(InsertOsfMenu);
		MainWindow->addAction(InsertTonMenu);
		MainWindow->addAction(InsertTofMenu);
		MainWindow->addAction(InsertRtoMenu);
		MainWindow->addAction(InsertCtuMenu);
		MainWindow->addAction(InsertCtdMenu);
		MainWindow->addAction(InsertCtcMenu);
		MainWindow->addAction(InsertEquMenu);
		MainWindow->addAction(InsertGrtMenu);
		MainWindow->addAction(InsertGeqMenu);
		MainWindow->addAction(InsertLesMenu);
		MainWindow->addAction(InsertLeqMenu);
		MainWindow->addAction(InsertCoilMenu);
		MainWindow->addAction(InsertResMenu);
		MainWindow->addAction(InsertMovMenu);
		MainWindow->addAction(InsertAddMenu);
		MainWindow->addAction(InsertSubMenu);
		MainWindow->addAction(InsertMulMenu);
		MainWindow->addAction(InsertDivMenu);
		MainWindow->addAction(InsertReadAdcMenu);
		MainWindow->addAction(MakeNormalMenu);
		MainWindow->addAction(NegateMenu);
		MainWindow->addAction(MakeSetOnlyMenu);
		MainWindow->addAction(MakeResetOnlyMenu);
		MainWindow->addAction(CompileMenu);
		MainWindow->addAction(ManualMenu);
		MainWindow->addAction(SimulationModeMenu);
		MainWindow->addAction(StartSimulationMenu);
		MainWindow->addAction(StopSimulationMenu);
		MainWindow->addAction(SingleCycleMenu);
	}
#endif

//-----------------------------------------------------------------------------
// Create the top-level menu bar for the main window. Mostly static, but we
// create the "select processor" menu from the list in mcutable.h dynamically.
//-----------------------------------------------------------------------------
HMENU MakeMainWindowMenus(void)
{
    HMENU  FileMenuSeparator;
    HMENU  EditMenuSeparator;
    HMENU  InstructionMenuSeparator;
    HMENU  SimulateMenuSeparator;

    int i;

    // Create new menu bar to hold menu and add it to window

    // Creating various menus
    FileMenu        = new QMenu("&File", MainWindow);
    EditMenu        = new QMenu("&Edit", MainWindow);
    Settings        = new QMenu("&Settings", MainWindow);
    ProcessorMenu   = new QMenu("&Microcontroller", MainWindow);
    InstructionMenu = new QMenu("&Instructions", MainWindow);
    SimulateMenu    = new QMenu("&Simulate", MainWindow);
    Compile         = new QMenu("&Compile", MainWindow);
    Help            = new QMenu("&Help", MainWindow);

    // Creating labels for File Menu
    NewMenu = new QAction("&New", NULL);
    NewMenu->setShortcuts(QKeySequence::New);
    // QKeySequence(Qt::CTRL + Qt::Key_N);
    OpenMenu = new QAction("&Open", NULL);
    OpenMenu->setShortcuts(QKeySequence::Open);
    SaveMenu = new QAction("&Save", NULL);
    SaveMenu->setShortcuts(QKeySequence::Save);
    SaveAsMenu = new QAction("&Save As", NULL);
    SaveAsMenu->setShortcuts(QKeySequence::SaveAs);
    ExportMenu = new QAction("&Export As Text", NULL);
    ExportMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    ExitMenu = new QAction("&Exit", NULL);

    // Appending menu items (labels) to File menu and adding separators
    FileMenu->addAction(NewMenu);
    FileMenu->addAction(OpenMenu);
    FileMenu->addAction(SaveMenu);
    FileMenu->addAction(SaveAsMenu);
    FileMenu->addSeparator();
    FileMenu->addAction(ExportMenu);
    FileMenu->addSeparator();
    FileMenu->addAction(ExitMenu);

    // Creating labels for Edit Menu
    UndoMenu = new QAction("&Undo", NULL);
    UndoMenu->setShortcuts(QKeySequence::Undo);
    RedoMenu = new QAction("&Redo", NULL);
    RedoMenu->setShortcuts(QKeySequence::Redo);
    InsertRungBeforeMenu = new QAction("&Insert Rung Before", NULL);
    InsertRungBeforeMenu->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_6));
    InsertRungAfterMenu = new QAction("&Insert Rung After", NULL);
    InsertRungAfterMenu->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_V));
    PushRungUpMenu = new QAction("&Move Selected Rung Up", NULL);
    PushRungUpMenu->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Up));
    PushRungDownMenu = new QAction("&Move Selected Rung Down", NULL);
    PushRungDownMenu->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Down));
    DeleteElementMenu = new QAction("&Delete Selected Element", NULL);
    DeleteElementMenu->setShortcut(QKeySequence(Qt::Key_Delete));
    DeleteRungMenu = new QAction("&Delete Rung", NULL);
    DeleteRungMenu->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete));

    // Appending menu items to Edit menu and adding separators
    EditMenu->addAction(UndoMenu);
    EditMenu->addAction(RedoMenu);
    EditMenu->addSeparator();
    EditMenu->addAction(InsertRungBeforeMenu);
    EditMenu->addAction(InsertRungAfterMenu);
    EditMenu->addAction(PushRungUpMenu);
    EditMenu->addAction(PushRungDownMenu);
    EditMenu->addSeparator();
    EditMenu->addAction(DeleteElementMenu);
    EditMenu->addAction(DeleteRungMenu);

    // Creating labels for Settings Menu
    McuSettingsMenu = new QAction("&MCU Parameters...", MainWindow);
    MicroControllerMenu = new QMenu("&Microcontroller", MainWindow);
    ProcessorMenuGroup = new QActionGroup(MicroControllerMenu);

    // Appending menu items to Settings menu
    Settings->addAction(McuSettingsMenu);
    Settings->addMenu(MicroControllerMenu);
    // Appending the microcontroller names to "Microcontroller" item
    for (i = 0; i < NUM_SUPPORTED_MCUS; i++){
    ProcessorMenuItems[i] = new QAction(SupportedMcus[i].mcuName, NULL);
    ProcessorMenuItems[i]->setCheckable(true);
    ProcessorMenuItems[i]->setActionGroup(ProcessorMenuGroup);
    MicroControllerMenu->addAction(ProcessorMenuItems[i]);
    }
    ProcessorMenuItems[i] = new QAction("(no microcontroller)", NULL);
    ProcessorMenuItems[i]->setCheckable(true);
    ProcessorMenuItems[i]->setActionGroup(ProcessorMenuGroup);
    MicroControllerMenu->addAction(ProcessorMenuItems[i]);

    // Creating labels for Instruction Menu and adding separators
    InsertCommentMenu = new QAction("&Insert Comment", MainWindow);
    InsertCommentMenu->setShortcut(QKeySequence(Qt::Key_Semicolon));
    InsertContactsMenu = new QAction("&Insert Contacts", MainWindow);
    InsertContactsMenu->setShortcut(QKeySequence(Qt::Key_C));
    InsertOsrMenu = new QAction("&Insert OSR (One Shot Rising)", MainWindow);
    InsertOsrMenu->setShortcut(QKeySequence(Qt::Key_Slash));
    InsertOsfMenu = new QAction("&Insert OSF (One Shot Falling)", MainWindow);
    InsertOsfMenu->setShortcut(QKeySequence(Qt::Key_Backslash));
    InsertTonMenu = new QAction("&Insert TON (Delayed Turn On)", MainWindow);
    InsertTonMenu->setShortcut(QKeySequence(Qt::Key_O));
    InsertTofMenu = new QAction("&Insert TOF (Delayed Turn Off)", MainWindow);
    InsertTofMenu->setShortcut(QKeySequence(Qt::Key_F));
    InsertRtoMenu = new QAction("&Insert RTO (Retentive Delayed Turn On)",
        MainWindow);
    InsertRtoMenu->setShortcut(QKeySequence(Qt::Key_T));
    InsertCtuMenu = new QAction("&Insert CTU (Count Up)", MainWindow);
    InsertCtuMenu->setShortcut(QKeySequence(Qt::Key_U));
    InsertCtdMenu = new QAction("&Insert CTD (Count Down)", MainWindow);
    InsertCtdMenu->setShortcut(QKeySequence(Qt::Key_I));
    InsertCtcMenu = new QAction("&Insert CTC (Count Circular)", MainWindow);
    InsertCtcMenu->setShortcut(QKeySequence(Qt::Key_J));
    InsertEquMenu = new QAction("&Insert EQU (Compare for Equals)", MainWindow);
    InsertEquMenu->setShortcut(QKeySequence(Qt::Key_Equal));
    InsertNeqMenu = new QAction("&Insert NEQ (Compare for Not Equals)",
        MainWindow);
    InsertGrtMenu = new QAction("&Insert GRT (Compare for Greater Than)",
        MainWindow);
    InsertGrtMenu->setShortcut(QKeySequence(Qt::Key_Greater));
    InsertGeqMenu = new QAction(
        "&Insert GEQ (Compare for Greater Than or Equal)", MainWindow);
    InsertGeqMenu->setShortcut(QKeySequence(Qt::Key_Period));
    InsertLesMenu = new QAction(
        "&Insert LES (Compare for Less Than)", MainWindow);
    InsertLesMenu->setShortcut(QKeySequence(Qt::Key_Less));
    InsertLeqMenu = new QAction(
        "&Insert LEQ (Compare for Less Than or Equal)", MainWindow);
    InsertLeqMenu->setShortcut(QKeySequence(Qt::Key_Comma));
    InsertOpenMenu = new QAction("&Insert Open Circuit", MainWindow);
    InsertShortMenu = new QAction("&Insert Short Circuit", MainWindow);
    InsertMasterRlyMenu = new QAction(
        "&Insert Master Control Relay", MainWindow);
    InsertCoilMenu = new QAction("&Insert Coil", MainWindow);
    InsertCoilMenu->setShortcut(QKeySequence(Qt::Key_L));
    InsertResMenu = new QAction("&Insert RES (Counter/RTO Reset)", MainWindow);
    InsertResMenu->setShortcut(QKeySequence(Qt::Key_E));
    InsertMovMenu = new QAction("&Insert MOV (Move)", MainWindow);
    InsertMovMenu->setShortcut(QKeySequence(Qt::Key_M));
    InsertAddMenu = new QAction("&Insert ADD (16-bit Integer Ad)", MainWindow);
    InsertAddMenu->setShortcut(QKeySequence(Qt::Key_Plus));
    InsertSubMenu = new QAction(
        "&Insert SUB (16-bit Integer Subtract)", MainWindow);
    InsertSubMenu->setShortcut(QKeySequence(Qt::Key_Minus));
    InsertMulMenu = new QAction(
        "&Insert MUL (16-bit Integer Multiply)", MainWindow);
    InsertMulMenu->setShortcut(QKeySequence(Qt::Key_Asterisk));
    InsertDivMenu = new QAction(
        "&Insert DIV (16-bit Integer Division)", MainWindow);
    InsertDivMenu->setShortcut(QKeySequence(Qt::Key_D));
    InsertShiftRegMenu = new QAction("&Insert Shift Register", MainWindow);
    InsertLutMenu = new QAction("&Insert Look-Up Table", MainWindow);
    InsertPwlMenu = new QAction("&Insert Piecewise Linear", MainWindow);
    InsertFmtdStrMenu = new QAction(
        "&Insert Formatted String Over UART", MainWindow);
    InsertUartSendMenu = new QAction("&Insert UART Send", MainWindow);
    InsertUartRecvMenu = new QAction("&Insert UART Receive", MainWindow);
    InsertSetPwmMenu = new QAction("&Insert Set PWM Output", MainWindow);
    InsertReadAdcMenu = new QAction("&Insert A/D Converter Read", MainWindow);
    InsertReadAdcMenu->setShortcut(QKeySequence(Qt::Key_P));
    InsertPersistMenu = new QAction("&Insert Make Persistent", MainWindow);
    MakeNormalMenu = new QAction("&Make Normal", MainWindow);
    MakeNormalMenu->setShortcut(QKeySequence(Qt::Key_A));
    NegateMenu = new QAction("&Make Negated", MainWindow);
    NegateMenu->setShortcut(QKeySequence(Qt::Key_N));
    MakeSetOnlyMenu = new QAction("&Make Set-Only", MainWindow);
    MakeSetOnlyMenu->setShortcut(QKeySequence(Qt::Key_S));
    MakeResetOnlyMenu = new QAction("&Make Reset-Only", MainWindow);
    MakeResetOnlyMenu->setShortcut(QKeySequence(Qt::Key_R));

    // Appending menu items to Instruction menu and adding separators
    InstructionMenu->addAction(InsertCommentMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertContactsMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertOsrMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertOsfMenu);
    InstructionMenu->addAction(InsertTonMenu);
    InstructionMenu->addAction(InsertTofMenu);
    InstructionMenu->addAction(InsertRtoMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertCtuMenu);
    InstructionMenu->addAction(InsertCtdMenu);
    InstructionMenu->addAction(InsertCtcMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertEquMenu);
    InstructionMenu->addAction(InsertNeqMenu);
    InstructionMenu->addAction(InsertGrtMenu);
    InstructionMenu->addAction(InsertGeqMenu);
    InstructionMenu->addAction(InsertLesMenu);
    InstructionMenu->addAction(InsertLeqMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertOpenMenu);
    InstructionMenu->addAction(InsertShortMenu);
    InstructionMenu->addAction(InsertMasterRlyMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertCoilMenu);
    InstructionMenu->addAction(InsertResMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertMovMenu);
    InstructionMenu->addAction(InsertAddMenu);
    InstructionMenu->addAction(InsertSubMenu);
    InstructionMenu->addAction(InsertMulMenu);
    InstructionMenu->addAction(InsertDivMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertShiftRegMenu);
    InstructionMenu->addAction(InsertLutMenu);
    InstructionMenu->addAction(InsertPwlMenu);
    InstructionMenu->addAction(InsertFmtdStrMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(InsertUartSendMenu);
    InstructionMenu->addAction(InsertUartRecvMenu);
    InstructionMenu->addAction(InsertSetPwmMenu);
    InstructionMenu->addAction(InsertReadAdcMenu);
    InstructionMenu->addAction(InsertPersistMenu);
    InstructionMenu->addSeparator();
    InstructionMenu->addAction(MakeNormalMenu);
    InstructionMenu->addAction(NegateMenu);
    InstructionMenu->addAction(MakeSetOnlyMenu);
    InstructionMenu->addAction(MakeResetOnlyMenu);

    // Creating labels for Compile Menu
    CompileMenu = new QAction("&Compile", MainWindow);
    CompileMenu->setShortcut(QKeySequence(Qt::Key_F5));
    CompileAsMenu = new QAction("Compile &As", MainWindow);

    // Appending menu items to Compile menu
    Compile->addAction(CompileMenu);
    Compile->addAction(CompileAsMenu);

    // Creating labels for Help Menu
    ManualMenu = new QAction("&Manual", MainWindow);
    ManualMenu->setShortcut(QKeySequence(Qt::Key_F1));
    AboutMenu = new QAction("&About", MainWindow);

    // Appending menu items to Help menu
    Help->addAction(ManualMenu);
    Help->addAction(AboutMenu);

    // Creating labels for Simulation Menu
    SimulationModeMenu = new QAction("&Simulation Mode", MainWindow);
    SimulationModeMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    StartSimulationMenu = new QAction("&Start Real-Time Simulation", MainWindow);
    StartSimulationMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    StopSimulationMenu = new QAction("&Halt Simulation", MainWindow);
    StopSimulationMenu->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    SingleCycleMenu = new QAction("&Single Cycle", MainWindow);
    SingleCycleMenu->setShortcut(QKeySequence(Qt::Key_Space));
    // Appending menu items to Simulate menu and adding separators
    SimulateMenu->addAction(SimulationModeMenu);
    SimulateMenu->addAction(StartSimulationMenu);
    SimulateMenu->addAction(StopSimulationMenu);
    SimulateMenu->addAction(SingleCycleMenu);
    SimulateMenu->addSeparator();

    // Appending the menu item to the menu bar
    MainMenu->addMenu(FileMenu);
    MainMenu->addMenu(EditMenu);
    MainMenu->addMenu(Settings);
    MainMenu->addMenu(InstructionMenu);
    MainMenu->addMenu(SimulateMenu);
    MainMenu->addMenu(Compile);
    MainMenu->addMenu(Help);
    #if defined(__UBUNTU_14_)
    	SetKeyboardShortcuts();
    #endif

    return FileMenu;
}

//-----------------------------------------------------------------------------
// Create the standard Windows controls used in the main window: a Listview
// for the I/O list, and a status bar for settings.
//-----------------------------------------------------------------------------
void MakeMainWindowControls(void)
{
    QVBoxLayout* PackBoxMenu = new QVBoxLayout;
    QSplitter *splitter = new QSplitter(Qt::Orientation::Vertical);
    MainWindow->setLayout(PackBoxMenu);
    PackBoxMenu->setMenuBar(MainMenu);
    IoList = new QTreeWidget();
    IoList->setColumnCount(IO_COLUMN_COUNT);
    IoList->setSelectionMode(QAbstractItemView::SingleSelection);
    QStringList ColumnNames = {"Name",
            "Type",
            "State",
            "Pin on Processor",
            "MCU Port"};
    IoList->setHeaderLabels(ColumnNames);
    DrawWindow->setAutoFillBackground(true);
    QSize DWSize;
    scrollbar = new QScrollArea();
    scrollbar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollbar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollbar->setFocusPolicy(Qt::NoFocus);
    DWSize.setHeight(MainWindow->height() - IoListHeight);
    DWSize.setWidth(MainWindow->width());
    DrawWindow->setMinimumHeight(100);
    scrollbar->resize(DWSize);
    scrollbar->setWidget(DrawWindow);
    splitter->addWidget(scrollbar);
    DWSize.setWidth(MainWindow->width() - 
        (scrollbar->sizeHint().width()+ MainWindow->sizeHint().width()));
    DrawWindow->resize(DWSize);
    DWSize.setHeight(IoListHeight);
    IoList->resize(DWSize);
    splitter->addWidget(IoList);
    QStatusBar* StatusGrid = new QStatusBar(MainWindow);
    PackBoxMenu->addWidget(splitter);
    for(int i = 0; i<3;i++)
    {
        StatusBar[i] = new QLabel(StatusGrid);
        StatusBar[i]->setText("LDMicro Started");
        StatusGrid->addPermanentWidget(StatusBar[i], 1);
    }
    PackBoxMenu->addWidget(StatusGrid);
}

//-----------------------------------------------------------------------------
// Adjust the size and visibility of the scrollbars as necessary, either due
// to a change in the size of the program or a change in the size of the
// window.
//-----------------------------------------------------------------------------
/*void RefreshScrollbars(void)
{
}*/

//-----------------------------------------------------------------------------
// Set up the title bar text for the main window; indicate whether we are in
// simulation or editing mode, and indicate the filename.
//-----------------------------------------------------------------------------
void UpdateMainWindowTitleBar(void)
{
    char line[PATH_MAX+100];
    if(InSimulationMode) {
        if(RealTimeSimulationRunning) {
            strcpy(line, "LDmicro - Simulation (Running)");
        } else {
            strcpy(line, "LDmicro - Simulation (Stopped)");
        }
    } else {
        strcpy(line, "LDmicro - Program Editor");
    }
    if(strlen(CurrentSaveFile) > 0) {
        sprintf(line+strlen(line), " - %s", CurrentSaveFile);
    } else {
        strcat(line, " - (not yet saved)");
    }
    
  // gtk_window_set_title (GTK_WINDOW (MainWindow), line);
    MainWindow->setWindowTitle(line);
}

//-----------------------------------------------------------------------------
// Set the enabled state of the logic menu items to reflect where we are on
// the schematic (e.g. can't insert two coils in series).
//-----------------------------------------------------------------------------
void SetMenusEnabled(BOOL canNegate, BOOL canNormal, BOOL canResetOnly,
    BOOL canSetOnly, BOOL canDelete, BOOL canInsertEnd, BOOL canInsertOther,
    BOOL canPushDown, BOOL canPushUp, BOOL canInsertComment)
{
    EnableMenuItem(EditMenu, PushRungUpMenu,
        canPushUp ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(EditMenu, PushRungDownMenu,
        canPushDown ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(EditMenu, DeleteRungMenu,
        (Prog.numRungs > 1) ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(InstructionMenu, NegateMenu,
        canNegate ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MakeNormalMenu,
        canNormal ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MakeResetOnlyMenu,
        canResetOnly ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MakeSetOnlyMenu,
        canSetOnly ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(InstructionMenu, InsertCommentMenu,
        canInsertComment ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(EditMenu, DeleteElementMenu,
        canDelete ? MF_ENABLED : MF_GRAYED);

    int t;
    t = canInsertEnd ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(InstructionMenu, InsertCoilMenu, t);
    EnableMenuItem(InstructionMenu, InsertResMenu, t);
    EnableMenuItem(InstructionMenu, InsertMovMenu, t);
    EnableMenuItem(InstructionMenu, InsertAddMenu, t);
    EnableMenuItem(InstructionMenu, InsertSubMenu, t);
    EnableMenuItem(InstructionMenu, InsertMulMenu, t);
    EnableMenuItem(InstructionMenu, InsertDivMenu, t);
    EnableMenuItem(InstructionMenu, InsertCtcMenu, t);
    EnableMenuItem(InstructionMenu, InsertPersistMenu, t);
    EnableMenuItem(InstructionMenu, InsertReadAdcMenu, t);
    EnableMenuItem(InstructionMenu, InsertSetPwmMenu, t);
    EnableMenuItem(InstructionMenu, InsertMasterRlyMenu, t);
    EnableMenuItem(InstructionMenu, InsertShiftRegMenu, t);
    EnableMenuItem(InstructionMenu, InsertLutMenu, t);
    EnableMenuItem(InstructionMenu, InsertPwlMenu, t);

    t = canInsertOther ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(InstructionMenu, InsertTonMenu, t);
    EnableMenuItem(InstructionMenu, InsertTofMenu, t);
    EnableMenuItem(InstructionMenu, InsertOsrMenu, t);
    EnableMenuItem(InstructionMenu, InsertOsfMenu, t);
    EnableMenuItem(InstructionMenu, InsertRtoMenu, t);
    EnableMenuItem(InstructionMenu, InsertContactsMenu, t);
    EnableMenuItem(InstructionMenu, InsertCtuMenu, t);
    EnableMenuItem(InstructionMenu, InsertCtdMenu, t);
    EnableMenuItem(InstructionMenu, InsertEquMenu, t);
    EnableMenuItem(InstructionMenu, InsertNeqMenu, t);
    EnableMenuItem(InstructionMenu, InsertGrtMenu, t);
    EnableMenuItem(InstructionMenu, InsertGeqMenu, t);
    EnableMenuItem(InstructionMenu, InsertLesMenu, t);
    EnableMenuItem(InstructionMenu, InsertLeqMenu, t);
    EnableMenuItem(InstructionMenu, InsertShortMenu, t);
    EnableMenuItem(InstructionMenu, InsertOpenMenu, t);
    EnableMenuItem(InstructionMenu, InsertUartSendMenu, t);
    EnableMenuItem(InstructionMenu, InsertUartRecvMenu, t);
    EnableMenuItem(InstructionMenu, InsertFmtdStrMenu, t);
}

//-----------------------------------------------------------------------------
// Set the enabled state of the undo/redo menus.
//-----------------------------------------------------------------------------
void SetUndoEnabled(BOOL undoEnabled, BOOL redoEnabled)
{
    EnableMenuItem(EditMenu, UndoMenu, undoEnabled ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(EditMenu, RedoMenu, redoEnabled ? MF_ENABLED : MF_GRAYED);
}

//-----------------------------------------------------------------------------
// Toggle whether we are in simulation mode. A lot of options are only
// available in one mode or the other.
//-----------------------------------------------------------------------------
void ToggleSimulationMode(void)
{
    InSimulationMode = !InSimulationMode;
    if(InSimulationMode) {
        KillTimer(DrawWindow, TIMER_BLINK_CURSOR);
        EnableMenuItem(SimulateMenu, StartSimulationMenu, MF_ENABLED);
        EnableMenuItem(SimulateMenu, SingleCycleMenu, MF_ENABLED);

        EnableMenuItem(FileMenu, OpenMenu, MF_GRAYED);
        EnableMenuItem(FileMenu, SaveMenu, MF_GRAYED);
        EnableMenuItem(FileMenu, SaveAsMenu, MF_GRAYED);
        EnableMenuItem(FileMenu, NewMenu, MF_GRAYED);
        EnableMenuItem(FileMenu, ExportMenu, MF_GRAYED);

        EnableMenuItem(TopMenu, EditMenu, MF_GRAYED);
        EnableMenuItem(TopMenu, Settings, MF_GRAYED);
        EnableMenuItem(TopMenu, InstructionMenu, MF_GRAYED);
        EnableMenuItem(TopMenu, Compile, MF_GRAYED);

        CheckMenuItem(SimulateMenu, SimulationModeMenu, MF_CHECKED);

        ClearSimulationData(); // simulation.cpp, ldmicro.h
        // Recheck InSimulationMode, because there could have been a Compile
        // error, which would have kicked us out of simulation mode.
        if(UartFunctionUsed() && InSimulationMode) {
            ShowUartSimulationWindow(); // simulate.cpp
        }
    }
    else {
        RealTimeSimulationRunning = FALSE;
        KillTimer(DrawWindow, TIMER_SIMULATE);
        CursorTimer = SetTimer(DrawWindow, TIMER_BLINK_CURSOR, 500, CursorTimer);
        EnableMenuItem(SimulateMenu, StartSimulationMenu, MF_GRAYED);
        EnableMenuItem(SimulateMenu, StopSimulationMenu, MF_GRAYED);
        EnableMenuItem(SimulateMenu, SingleCycleMenu, MF_GRAYED);

        EnableMenuItem(FileMenu, OpenMenu, MF_ENABLED);
        EnableMenuItem(FileMenu, SaveMenu, MF_ENABLED);
        EnableMenuItem(FileMenu, SaveAsMenu, MF_ENABLED);
        EnableMenuItem(FileMenu, NewMenu, MF_ENABLED);
        EnableMenuItem(FileMenu, ExportMenu, MF_ENABLED);

        EnableMenuItem(TopMenu, EditMenu, MF_ENABLED);
        EnableMenuItem(TopMenu, Settings, MF_ENABLED);
        EnableMenuItem(TopMenu, InstructionMenu, MF_ENABLED);
        EnableMenuItem(TopMenu, Compile, MF_ENABLED);

        CheckMenuItem(SimulateMenu, SimulationModeMenu, MF_UNCHECKED);

        if(UartFunctionUsed()) {
            DestroyUartSimulationWindow();
        }
        }

    UpdateMainWindowTitleBar();
    RefreshControlsToSettings();
}

//-----------------------------------------------------------------------------
// Cause the status bar and the list view to be in sync with the actual data
// structures describing the settings and the I/O configuration. Listview
// does callbacks to get the strings it displays, so it just needs to know
// how many elements to populate.
//-----------------------------------------------------------------------------
void RefreshControlsToSettings(void)
{
    QTreeWidgetItem iter;
    QTreeWidgetItem* selection;
    if(!IoListOutOfSync) {
        selection = IoList->currentItem();
        IoListSelectionPoint =IoList->indexOfTopLevelItem(selection);
    }
    NMHDR h;
    h.code = LVN_GETDISPINFO;
    h.hlistFrom = IoList;
    IoList->clear();
    h.hlistIter.clear();
    for(int i = 0; i < Prog.io.count; i++) {
        h.item.iItem = i;
        IoListProc(&h);
    }
    IoList->insertTopLevelItems(0, h.hlistIter);
    if(IoListSelectionPoint >= 0)
    {
        IoList->setCurrentItem(IoList->topLevelItem(IoListSelectionPoint));
    }

    if(Prog.mcu) {
        StatusBar[0]->setText(Prog.mcu->mcuName);
    } 
    else {
        StatusBar[0]->setText("no MCU selected");
    }
    char buf[256];
    sprintf(buf, _("cycle time %.2f ms"), (double)Prog.cycleTime/1000.0);
    StatusBar[1]->setText(buf);

    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_ANSIC ||
        Prog.mcu->whichIsa == ISA_INTERPRETED))
    {
        strcpy(buf, "");
    } else {
        sprintf(buf, _("processor clock %.4f MHz"),
            (double)Prog.mcuClock/1000000.0);
    }
    StatusBar[2]->setText(buf);


    for(int i = 0; i < NUM_SUPPORTED_MCUS; i++) {
        if(&SupportedMcus[i] == Prog.mcu) {
            CheckMenuItem(ProcessorMenu, ProcessorMenuItems[i], MF_CHECKED);
        }
        else {
            CheckMenuItem(ProcessorMenu, ProcessorMenuItems[i], MF_UNCHECKED);
        }
    }
    // `(no microcontroller)' setting
    if (!Prog.mcu){
        CheckMenuItem(ProcessorMenu,
            ProcessorMenuItems[NUM_SUPPORTED_MCUS], MF_CHECKED);
    }
    else {
        CheckMenuItem(ProcessorMenu,
            ProcessorMenuItems[NUM_SUPPORTED_MCUS], MF_UNCHECKED);
    }
}

//-----------------------------------------------------------------------------
// Regenerate the I/O list, keeping the selection in the same place if
// possible.
//-----------------------------------------------------------------------------
void GenerateIoListDontLoseSelection(void)
{
    QTreeWidgetItem* selection;
    selection = IoList->currentItem();
    IoListSelectionPoint =IoList->indexOfTopLevelItem(selection);
    IoListSelectionPoint = GenerateIoList(IoListSelectionPoint);
    
    // can't just update the listview index; if I/O has been added then the
    // new selection point might be out of range till we refill it
    IoListOutOfSync = TRUE;
    RefreshControlsToSettings();
}

//-----------------------------------------------------------------------------
// Called when the main window has been resized. Adjust the size of the
// status bar and the listview to reflect the new window size.
//-----------------------------------------------------------------------------
void MainWindowResized(void)
{
    IoListTop = DrawWindow->height();

}

//-----------------------------------------------------------------------------
// Start real-time simulation. Have to update the controls grayed status
// to reflect this.
//-----------------------------------------------------------------------------
void StartSimulation(void)
{
    RealTimeSimulationRunning = TRUE;
    EnableMenuItem(SimulateMenu, StartSimulationMenu, MF_GRAYED);
    EnableMenuItem(SimulateMenu, StopSimulationMenu, MF_ENABLED);
    StartSimulationTimer();
    UpdateMainWindowTitleBar();
}

//-----------------------------------------------------------------------------
// Stop real-time simulation. Have to update the controls grayed status
// to reflect this.
//-----------------------------------------------------------------------------
void StopSimulation(void)
{
    RealTimeSimulationRunning = FALSE;

    EnableMenuItem(SimulateMenu, StartSimulationMenu, MF_ENABLED);
    EnableMenuItem(SimulateMenu, StopSimulationMenu, MF_GRAYED);
    KillTimer(DrawWindow, TIMER_SIMULATE);
    
    UpdateMainWindowTitleBar();
}