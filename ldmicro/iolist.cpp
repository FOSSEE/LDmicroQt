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
// Routines to maintain the processor I/O list. Whenever the user changes the
// name of an element, rebuild the I/O list from the PLC program, so that new
// assigned names are automatically reflected in the I/O list. Also keep a 
// list of old I/Os that have been deleted, so that if the user deletes a
// a name and then recreates it the associated settings (e.g. pin number)
// will not be forgotten. Also the dialog box for assigning I/O pins.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "linuxUI.h"
//#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

// I/O that we have seen recently, so that we don't forget pin assignments
// when we re-extract the list
#define MAX_IO_SEEN_PREVIOUSLY 512
static struct {
    char    name[MAX_NAME_LEN];
    int     type;
    int     pin;
} IoSeenPreviously[MAX_IO_SEEN_PREVIOUSLY];
static int IoSeenPreviouslyCount;

// // stuff for the dialog box that lets you choose pin assignments
static BOOL DialogDone;
static BOOL DialogCancel;

static HWID IoDialog;

HWID PinList;
static HWID OkButton;
static HWID CancelButton;

// // stuff for the popup that lets you set the simulated value of an analog in
static HWID AnalogSliderMain;
static HWID AnalogSliderTrackbar;
static BOOL AnalogSliderDone;
static BOOL AnalogSliderCancel;


//-----------------------------------------------------------------------------
// Append an I/O to the I/O list if it is not in there already.
//-----------------------------------------------------------------------------
static void AppendIo(char *name, int type)
{
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name)==0) {
            if(type != IO_TYPE_GENERAL && Prog.io.assignment[i].type ==
                IO_TYPE_GENERAL)
            {
                Prog.io.assignment[i].type = type;
            }
            // already in there
            return;
        }
    }
    if(i < MAX_IO) {
        Prog.io.assignment[i].type = type;
        Prog.io.assignment[i].pin = NO_PIN_ASSIGNED;
        strcpy(Prog.io.assignment[i].name, name);
        (Prog.io.count)++;
    }
}

//-----------------------------------------------------------------------------
// Move an I/O pin into the `seen previously' list. This means that if the
// user creates input Xasd, assigns it a pin, deletes, and then recreates it,
// then it will come back with the correct pin assigned.
//-----------------------------------------------------------------------------
static void AppendIoSeenPreviously(char *name, int type, int pin)
{
    if(strcmp(name+1, "new")==0) return;

    int i;
    for(i = 0; i < IoSeenPreviouslyCount; i++) {
        if(strcmp(name, IoSeenPreviously[i].name)==0 &&
            type == IoSeenPreviously[i].type)
        {
            if(pin != NO_PIN_ASSIGNED) {
                IoSeenPreviously[i].pin = pin;
            }
            return;
        }
    }
    if(IoSeenPreviouslyCount >= MAX_IO_SEEN_PREVIOUSLY) {
        // maybe improve later; just throw away all our old information, and
        // the user might have to reenter the pin if they delete and recreate
        // things
        IoSeenPreviouslyCount = 0;
    }

    i = IoSeenPreviouslyCount;
    IoSeenPreviously[i].type = type;
    IoSeenPreviously[i].pin = pin;
    strcpy(IoSeenPreviously[i].name, name);
    IoSeenPreviouslyCount++;
}

//-----------------------------------------------------------------------------
// Walk a subcircuit, calling ourselves recursively and extracting all the
// I/O names out of it.
//-----------------------------------------------------------------------------
static void ExtractNamesFromCircuit(int which, void *any)
{
    ElemLeaf *l = (ElemLeaf *)any;

    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int i;
            for(i = 0; i < p->count; i++) {
                ExtractNamesFromCircuit(p->contents[i].which,
                    p->contents[i].d.any);
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int i;
            for(i = 0; i < s->count; i++) {
                ExtractNamesFromCircuit(s->contents[i].which,
                    s->contents[i].d.any);
            }
            break;
        }
        case ELEM_CONTACTS:
            switch(l->d.contacts.name[0]) {
                case 'R':
                    AppendIo(l->d.contacts.name, IO_TYPE_INTERNAL_RELAY);
                    break;

                case 'Y':
                    AppendIo(l->d.contacts.name, IO_TYPE_DIG_OUTPUT);
                    break;

                case 'X':
                    AppendIo(l->d.contacts.name, IO_TYPE_DIG_INPUT);
                    break;

                default:
                    oops();
                    break;
            }
            break;

        case ELEM_COIL:
            AppendIo(l->d.coil.name, l->d.coil.name[0] == 'R' ?
                IO_TYPE_INTERNAL_RELAY : IO_TYPE_DIG_OUTPUT);
            break;

        case ELEM_TON:
        case ELEM_TOF:
            AppendIo(l->d.timer.name, which == ELEM_TON ?  IO_TYPE_TON :
                IO_TYPE_TOF);
            break;

        case ELEM_RTO:
            AppendIo(l->d.timer.name, IO_TYPE_RTO);
            break;

        case ELEM_MOVE:
            AppendIo(l->d.move.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
            AppendIo(l->d.math.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_FORMATTED_STRING:
            if(strlen(l->d.fmtdStr.var) > 0) {
                AppendIo(l->d.fmtdStr.var, IO_TYPE_UART_TX);
            }
            break;

        case ELEM_UART_SEND:
            AppendIo(l->d.uart.name, IO_TYPE_UART_TX);
            break;

        case ELEM_UART_RECV:
            AppendIo(l->d.uart.name, IO_TYPE_UART_RX);
            break;

        case ELEM_SET_PWM:
            AppendIo(l->d.setPwm.name, IO_TYPE_PWM_OUTPUT);
            break;

        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC:
            AppendIo(l->d.counter.name, IO_TYPE_COUNTER);
            break;

        case ELEM_READ_ADC:
            AppendIo(l->d.readAdc.name, IO_TYPE_READ_ADC);
            break;

        case ELEM_SHIFT_REGISTER: {
            int i;
            for(i = 0; i < l->d.shiftRegister.stages; i++) {
                char str[MAX_NAME_LEN+10];
                sprintf(str, "%s%d", l->d.shiftRegister.name, i);
                AppendIo(str, IO_TYPE_GENERAL);
            }
            break;
        }

        case ELEM_LOOK_UP_TABLE:
            AppendIo(l->d.lookUpTable.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_PIECEWISE_LINEAR:
            AppendIo(l->d.piecewiseLinear.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_PLACEHOLDER:
        case ELEM_COMMENT:
        case ELEM_SHORT:
        case ELEM_OPEN:
        case ELEM_MASTER_RELAY:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
        case ELEM_RES:
        case ELEM_PERSIST:
            break;

        default:
            oops();
    }
}

//-----------------------------------------------------------------------------
// Compare function to qsort() the I/O list. Group by type, then 
// alphabetically within each section.
//-----------------------------------------------------------------------------
static int CompareIo(const void *av, const void *bv)
{
    PlcProgramSingleIo *a = (PlcProgramSingleIo *)av;
    PlcProgramSingleIo *b = (PlcProgramSingleIo *)bv;

    if(a->type != b->type) {
        return a->type - b->type;
    }

    if(a->pin == NO_PIN_ASSIGNED && b->pin != NO_PIN_ASSIGNED) return  1;
    if(b->pin == NO_PIN_ASSIGNED && a->pin != NO_PIN_ASSIGNED) return -1;

    return strcmp(a->name, b->name);
}

//-----------------------------------------------------------------------------
// Wipe the I/O list and then re-extract it from the PLC program, taking
// care not to forget the pin assignments. Gets passed the selected item
// as an index into the list; modifies the list, so returns the new selected
// item as an index into the new list.
//-----------------------------------------------------------------------------
int GenerateIoList(int prevSel)
{
    int i, j;

    char selName[MAX_NAME_LEN];
    if(prevSel >= 0) {
        strcpy(selName, Prog.io.assignment[prevSel].name);
    }

    if(IoSeenPreviouslyCount > MAX_IO_SEEN_PREVIOUSLY/2) {
        // flush it so there's lots of room, and we don't run out and
        // forget important things
        IoSeenPreviouslyCount = 0;
    }
    
    // remember the pin assignments
    for(i = 0; i < Prog.io.count; i++) {
        AppendIoSeenPreviously(Prog.io.assignment[i].name,
            Prog.io.assignment[i].type, Prog.io.assignment[i].pin);
    }
    // wipe the list
    Prog.io.count = 0;
    // extract the new list so that it must be up to date
    for(i = 0; i < Prog.numRungs; i++) {
        ExtractNamesFromCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }

    for(i = 0; i < Prog.io.count; i++) {
        if(Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_READ_ADC)
        {
            for(j = 0; j < IoSeenPreviouslyCount; j++) {
                if(strcmp(Prog.io.assignment[i].name, 
                    IoSeenPreviously[j].name)==0)
                {
                    Prog.io.assignment[i].pin = IoSeenPreviously[j].pin;
                    break;
                }
            }
        }
    }

    qsort(Prog.io.assignment, Prog.io.count, sizeof(PlcProgramSingleIo),
        CompareIo);

    if(prevSel >= 0) {
        for(i = 0; i < Prog.io.count; i++) {
            if(strcmp(Prog.io.assignment[i].name, selName)==0)
                break;
        }
        if(i < Prog.io.count)
            return i;
    }
    // no previous, or selected was deleted
    return -1;
}

//-----------------------------------------------------------------------------
// Load the I/O list from a file. Since we are just loading pin assignments,
// put it into IoSeenPreviously so that it will get used on the next
// extraction.
//-----------------------------------------------------------------------------
BOOL LoadIoListFromFile(FILE *f)
{
    char line[80];
    char name[MAX_NAME_LEN];
    int pin;
    while(fgets(line, sizeof(line), f)) {
        ManageLineEnding(line);
        if(strcmp(line, "END\n")==0) {
            return TRUE;
        }
        // Don't internationalize this! It's the file format, not UI.
        if(sscanf(line, "    %s at %d", name, &pin)==2) {
            int type;
            switch(name[0]) {
                case 'X': type = IO_TYPE_DIG_INPUT; break;
                case 'Y': type = IO_TYPE_DIG_OUTPUT; break;
                case 'A': type = IO_TYPE_READ_ADC; break;
                default: oops();
            }
            AppendIoSeenPreviously(name, type, pin);
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Write the I/O list to a file. Since everything except the pin assignment
// can be extracted from the schematic, just write the Xs and Ys.
//-----------------------------------------------------------------------------
void SaveIoListToFile(FILE *f)
{
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT  ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_READ_ADC)
        {
            // Don't internationalize this! It's the file format, not UI.
            fprintf(f, "    %s at %d\n", Prog.io.assignment[i].name,
                Prog.io.assignment[i].pin);
        }
    }
}

//-----------------------------------------------------------------------------
// Dialog proc for the popup that lets you set the value of an analog input for
// simulation.
//-----------------------------------------------------------------------------
/*static gboolean AnalogSliderDialogKeyboardProc(GtkWidget* widget, GdkEventKey* event, gpointer name)
{
    SWORD v = (SWORD)gtk_range_get_value(GTK_RANGE(AnalogSliderTrackbar));
    SetAdcShadow((char*)name, v);
    if (AnalogSliderDone == TRUE || AnalogSliderCancel == TRUE)
    {
        DestroyWindow (AnalogSliderMain);
        ProgramChanged();
        return FALSE;
    }

    if (event->keyval == GDK_KEY_Return){
        DestroyWindow (AnalogSliderMain);
        ProgramChanged();
        AnalogSliderDone = TRUE;
    }
    else if (event->keyval == GDK_KEY_Escape){
        DestroyWindow (AnalogSliderMain);
        ProgramChanged();
        AnalogSliderDone = TRUE;
        AnalogSliderCancel = TRUE;
    }

    return FALSE;
}

static gboolean AnalogSliderDialogMouseProc(GtkWidget *widget, GdkEventButton *event, gpointer name)
{
    SWORD v = (SWORD)gtk_range_get_value(GTK_RANGE(AnalogSliderTrackbar));
    SetAdcShadow((char*)name, v);
    if (event->button == 1 && event->type == GDK_BUTTON_RELEASE){
        DestroyWindow (AnalogSliderMain);
        ProgramChanged();
        AnalogSliderDone = TRUE;
    }

    return FALSE;
}

void AnalogSliderUpdateProc (GtkRange *range, GtkScrollType step, gpointer name)
{
    SWORD v = (SWORD)gtk_range_get_value(GTK_RANGE(AnalogSliderTrackbar));
    SetAdcShadow((char*)name, v);
}

//-----------------------------------------------------------------------------
// A little toolbar-style window that pops up to allow the user to set the
// simulated value of an ADC pin.
//-----------------------------------------------------------------------------
void ShowAnalogSliderPopup(char *name)
{
    SWORD currentVal = GetAdcShadow(name);

    SWORD maxVal;
    if(Prog.mcu) {
        maxVal = Prog.mcu->adcMax;
    } else {
        maxVal = 1023;
    }
    if(maxVal == 0) {
        Error(_("No ADC or ADC not supported for selected micro."));
        return;
    }

    int left = GLOBAL_mouse_last_clicked_x - 10;
    // try to put the slider directly under the cursor (though later we might
    // realize that that would put the popup off the screen)
    int top = GLOBAL_mouse_last_clicked_y - (15 + (73*currentVal)/maxVal);

    int x, y;
    gdk_window_get_origin (gtk_widget_get_window (view), &x, &y);

    if(top < 0) top = y + 10;
    if(left < 0) left = x + 20;
    
    if (GTK_IS_WINDOW(AnalogSliderMain))
        return;
    
    AnalogSliderMain = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_title(GTK_WINDOW(AnalogSliderMain),  _("I/O Pin"));
    gtk_window_resize (GTK_WINDOW(AnalogSliderMain), 30, 100);
    gtk_window_move(GTK_WINDOW(AnalogSliderMain), left, top);

    AnalogSliderTrackbar = gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL,
                    0,
                    maxVal,
                    1);

    gtk_range_set_value (GTK_RANGE(AnalogSliderTrackbar), currentVal);
    
    gtk_container_add(GTK_CONTAINER(AnalogSliderMain), AnalogSliderTrackbar);
    
    // SetFocus(AnalogSliderTrackbar);
    gtk_window_set_focus_visible (GTK_WINDOW(AnalogSliderMain), TRUE);
    gtk_window_set_keep_above (GTK_WINDOW(AnalogSliderMain), TRUE);
    gtk_window_set_focus (GTK_WINDOW(AnalogSliderMain), AnalogSliderTrackbar);
    
    g_signal_connect (GTK_RANGE(AnalogSliderTrackbar), "key-press-event",
                    G_CALLBACK(AnalogSliderDialogKeyboardProc), (PVOID)name);
    g_signal_connect (GTK_RANGE(AnalogSliderTrackbar), "button-release-event", 
                    G_CALLBACK (AnalogSliderDialogMouseProc), (PVOID)name);
    g_signal_connect (GTK_RANGE(AnalogSliderTrackbar), "value-changed", 
                    G_CALLBACK (AnalogSliderUpdateProc), (PVOID)name);

    gtk_widget_show_all(AnalogSliderMain);

    AnalogSliderDone = FALSE;
    AnalogSliderCancel = FALSE;
  
    // ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
}

//-----------------------------------------------------------------------------
// Window proc for the contacts dialog box
//-----------------------------------------------------------------------------
static void IoDialogProc(BOOL DialogDone, int item)
{
    if(DialogDone)
    {
        char pin[16];
        ITLIST iter;

        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(PinList));
        gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

        if(gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
            GValue valproc = G_VALUE_INIT;

            gtk_tree_model_get_value (gtk_tree_view_get_model (GTK_TREE_VIEW(PinList)),
                            &iter, 0, &valproc);
            gchar* str = (char*)g_value_get_string(&valproc);
            strcpy(pin, str);
            g_free(str);
        }
        else
            strcpy(pin, _("(no pin)"));

        if(strcmp(pin, _("(no pin)"))==0) 
        {
            int i;
            for(i = 0; i < IoSeenPreviouslyCount; i++) {
                if(strcmp(IoSeenPreviously[i].name,
                    Prog.io.assignment[item].name)==0)
                {
                    IoSeenPreviously[i].pin = NO_PIN_ASSIGNED;
                }
            }
            Prog.io.assignment[item].pin = NO_PIN_ASSIGNED;
        } 
        else 
        {
            Prog.io.assignment[item].pin = atoi(pin);
            // Only one name can be bound to each pin; make sure that there's
            // not another entry for this pin in the IoSeenPreviously list,
            // that might get used if the user creates a new pin with that
            // name.
            int i;
            for(i = 0; i < IoSeenPreviouslyCount; i++) {
                if(IoSeenPreviously[i].pin == atoi(pin)) {
                    IoSeenPreviously[i].pin = NO_PIN_ASSIGNED;
                }
            }
        }
        RefreshControlsToSettings();
    }

    DestroyWindow(IoDialog);
    ProgramChanged();
}

void IoDialogRowActivateProc(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    IoDialogProc(TRUE, GPOINTER_TO_INT(user_data));
}

void IoDialogCancelProc(HWID widget, gpointer data)
{
    IoDialogProc(FALSE, GPOINTER_TO_INT(data));
}

void IoDialogOkProc(HWID widget, gpointer data)
{
    IoDialogProc(TRUE, GPOINTER_TO_INT(data));
}

static gboolean IoDialogKeyPressProc(HWID widget, GdkEventKey* event, gpointer data)
{
    if (event -> keyval == GDK_KEY_Return)
    {
        IoDialogProc(TRUE, GPOINTER_TO_INT(data));
    }
    else if (event -> keyval == GDK_KEY_Escape)
    {
        IoDialogProc(FALSE, GPOINTER_TO_INT(data));
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Create our window class; nothing exciting.
//-----------------------------------------------------------------------------
// static BOOL MakeWindowClass()
// {
//     WNDCLASSEX wc;
//     memset(&wc, 0, sizeof(wc));
//     wc.cbSize = sizeof(wc);

//     wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
//                             CS_DBLCLKS;
//     wc.lpfnWndProc      = (WNDPROC)IoDialogProc;
//     wc.hInstance        = Instance;
//     wc.hbrBackground    = (HBRUSH)COLOR_BTNSHADOW;
//     wc.lpszClassName    = "LDmicroIo";
//     wc.lpszMenuName     = NULL;
//     wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
//     wc.hIcon            = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
//                             IMAGE_ICON, 32, 32, 0);
//     wc.hIconSm          = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
//                             IMAGE_ICON, 16, 16, 0);

//     return RegisterClassEx(&wc);
// }

static void MakeControls(HWID Dialog)
{
    HLIST IoPinList = (GtkTreeModel*)gtk_list_store_new (1, G_TYPE_STRING);
    
    PinList = gtk_tree_view_new_with_model (GTK_TREE_MODEL(IoPinList));
    HTVC column = gtk_tree_view_column_new_with_attributes(_("Assign:"),
                                                    gtk_cell_renderer_text_new(),
                                                    "text", 0,
                                                    NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(PinList), column);
    FixedFont(PinList);

    OkButton = gtk_button_new_with_label (_("OK"));
    NiceFont(OkButton);

    CancelButton = gtk_button_new_with_label (_("Cancel"));
    NiceFont(CancelButton);

    /// Add list to scrolled window to enable scrolling
    HWID PinScroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (PinScroll),
				                          GTK_POLICY_AUTOMATIC, 
				                          GTK_POLICY_ALWAYS);
    gtk_widget_set_hexpand(GTK_WIDGET(PinScroll), TRUE);  
    gtk_widget_set_vexpand(GTK_WIDGET(PinScroll), TRUE);

    gtk_container_add (GTK_CONTAINER(PinScroll), PinList);

    /// Pack all the widgets into main window
    HWID PinBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(PinBox), PinScroll, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(PinBox), OkButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(PinBox), CancelButton, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(Dialog), PinBox);
}

void ShowIoDialog(int item)
{
    if(!Prog.mcu) {
        MessageBox(MainWindow,
            _("No microcontroller has been selected. You must select a "
            "microcontroller before you can assign I/O pins.\r\n\r\n"
            "Select a microcontroller under the Settings menu and try "
            "again."), _("I/O Pin Assignment"), MB_OK | MB_ICONWARNING);
        return;
    }

    if(Prog.mcu->whichIsa == ISA_ANSIC) {
        Error(_("Can't specify I/O assignment for ANSI C target; compile and "
            "see comments in generated source code."));
        return;
    }

    if(Prog.mcu->whichIsa == ISA_INTERPRETED) {
        Error(_("Can't specify I/O assignment for interpretable target; see "
            "comments in reference implementation of interpreter."));
        return;
    }

    if(Prog.io.assignment[item].name[0] != 'X' && 
       Prog.io.assignment[item].name[0] != 'Y' &&
       Prog.io.assignment[item].name[0] != 'A')
    {
        Error(_("Can only assign pin number to input/output pins (Xname or "
            "Yname or Aname)."));
        return;
    }

    if(Prog.io.assignment[item].name[0] == 'A' && Prog.mcu->adcCount == 0) {
        Error(_("No ADC or ADC not supported for this micro."));
        return;
    }

    if(strcmp(Prog.io.assignment[item].name+1, "new")==0) {
        Error(_("Rename I/O from default name ('%s') before assigning "
            "MCU pin."), Prog.io.assignment[item].name);
        return;
    }

    // We need the TOOLWINDOW style, or else the window will be forced to
    // a minimum width greater than our current width. And without the
    // APPWINDOW style, it becomes impossible to get the window back (by
    // Alt+Tab or taskbar).
    IoDialog = CreateWindowClient(GTK_WINDOW_TOPLEVEL, GDK_WINDOW_TYPE_HINT_MENU, _("I/O Pin"),
        100, 100, 107, 387, NULL);
    
    MakeControls(IoDialog);

    ITLIST iter;
    GValue val = G_VALUE_INIT;
    g_value_init (&val, G_TYPE_STRING);
    
    HLIST model = gtk_tree_view_get_model (GTK_TREE_VIEW(PinList));
    
    gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter);

    gtk_list_store_append (GTK_LIST_STORE(model), &iter);
    
    g_value_set_string(&val, _("(no pin)"));
    gtk_list_store_set_value (GTK_LIST_STORE(model), &iter, 0, &val);
    
    int i;
    for(i = 0; i < Prog.mcu->pinCount; i++) {
        int j;
        for(j = 0; j < Prog.io.count; j++) {
            if(j == item) continue;
            if(Prog.io.assignment[j].pin == Prog.mcu->pinInfo[i].pin) {
                goto cant_use_this_io;
            }
        }

        if(UartFunctionUsed() && Prog.mcu &&
                ((Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.rxPin) ||
                 (Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.txPin)))
        {
            goto cant_use_this_io;
        }

        if(PwmFunctionUsed() && 
            Prog.mcu->pinInfo[i].pin == Prog.mcu->pwmNeedsPin)
        {
            goto cant_use_this_io;
        }

        if(Prog.io.assignment[item].name[0] == 'A') {
            for(j = 0; j < Prog.mcu->adcCount; j++) {
                if(Prog.mcu->adcInfo[j].pin == Prog.mcu->pinInfo[i].pin) {
                    // okay; we know how to connect it up to the ADC
                    break;
                }
            }
            if(j == Prog.mcu->adcCount) {
                goto cant_use_this_io;
            }
        }

        char buf[40];
        if(Prog.mcu->pinCount <= 21) {
            sprintf(buf, "%3d   %c%c%d", Prog.mcu->pinInfo[i].pin,
                Prog.mcu->portPrefix, Prog.mcu->pinInfo[i].port,
                Prog.mcu->pinInfo[i].bit);
        } else {
            sprintf(buf, "%3d  %c%c%d", Prog.mcu->pinInfo[i].pin,
                Prog.mcu->portPrefix, Prog.mcu->pinInfo[i].port,
                Prog.mcu->pinInfo[i].bit);
        }
        gtk_list_store_append (GTK_LIST_STORE(model), &iter);
        g_value_set_string(&val, buf);
        gtk_list_store_set_value (GTK_LIST_STORE(model), &iter, 0, &val);
cant_use_this_io:;
    }

    gtk_widget_show_all(IoDialog);

    g_signal_connect (PinList, "row_activated", G_CALLBACK (IoDialogRowActivateProc), GINT_TO_POINTER(item));
    g_signal_connect (IoDialog, "key_press_event", G_CALLBACK (IoDialogKeyPressProc), GINT_TO_POINTER(item));
    g_signal_connect (CancelButton, "clicked", G_CALLBACK (IoDialogCancelProc), GINT_TO_POINTER(item));
    g_signal_connect (OkButton, "clicked", G_CALLBACK (IoDialogOkProc), GINT_TO_POINTER(item));
}
*/
//-----------------------------------------------------------------------------
// Called in response to a notify for the listview. Handles click, text-edit
// operations etc., but also gets called to find out what text to display
// where (LPSTR_TEXTCALLBACK); that way we don't have two parallel copies of
// the I/O list to keep in sync.
//----------------------------------------------------------------  -------------
void IoListProc(NMHDR *h)
{
    QStringList StrL;
    int         item;
    switch(h->code) {
        case LVN_GETDISPINFO: {
            item = h->item.iItem;
            /// Don't confuse people by displaying bogus pin assignments
            /// for the C target.
            char IO_value_holder[60];
            QString val;
            /// case LV_IO_NAME:
            val = QString::fromStdString((const char*)Prog.io.assignment[item].name);
            StrL.insert(0,val);

            /// case LV_IO_TYPE: 
            val = QString::fromStdString(IoTypeToString(Prog.io.assignment[item].type));
            StrL.insert(1,val);

            /// case LV_IO_STATE: 
            if(InSimulationMode) {
                char *name = Prog.io.assignment[item].name;
                DescribeForIoList(name, IO_value_holder);
            } else {
                strcpy(IO_value_holder, "");
            }
            val = QString::fromStdString(IO_value_holder);
            StrL.insert(2,val);

            /// case LV_IO_PIN:
            if(Prog.mcu && (Prog.mcu->whichIsa == ISA_ANSIC ||
                            Prog.mcu->whichIsa == ISA_INTERPRETED) )
            {
                strcpy(IO_value_holder, "");
            }
            else
            {
                PinNumberForIo(IO_value_holder, &(Prog.io.assignment[item]));
            }
            val = QString::fromStdString(IO_value_holder);
            StrL.insert(3,val);

            /// case LV_IO_PORT: 
            /// Don't confuse people by displaying bogus pin assignments
            /// for the C target.

            if(Prog.mcu && Prog.mcu->whichIsa == ISA_ANSIC) {
                strcpy(IO_value_holder, "");

                break;
            }

            int type = Prog.io.assignment[item].type;
            if(type != IO_TYPE_DIG_INPUT && type != IO_TYPE_DIG_OUTPUT
                && type != IO_TYPE_READ_ADC)
            {
                strcpy(IO_value_holder, "");
                break;
            }

            int pin = Prog.io.assignment[item].pin;
            if(pin == NO_PIN_ASSIGNED || !Prog.mcu) {
                strcpy(IO_value_holder, "");
                break;
            }
            if(UartFunctionUsed() && Prog.mcu) {
                if((Prog.mcu->uartNeeds.rxPin == pin) ||
                    (Prog.mcu->uartNeeds.txPin == pin))
                {
                    strcpy(IO_value_holder, _("<UART needs!>"));
                    break;
                }
            }

            if(PwmFunctionUsed() && Prog.mcu) {
                if(Prog.mcu->pwmNeedsPin == pin) {
                    strcpy(IO_value_holder, _("<PWM needs!>"));
                    break;
                }
            }

            int j;
            for(j = 0; j < Prog.mcu->pinCount; j++) {
                if(Prog.mcu->pinInfo[j].pin == pin) {
                    sprintf(IO_value_holder, "%c%c%d",
                        Prog.mcu->portPrefix,
                        Prog.mcu->pinInfo[j].port,
                        Prog.mcu->pinInfo[j].bit);
                    break;
                }
            }
            if(j == Prog.mcu->pinCount) {
                sprintf(IO_value_holder, _("<not an I/O!>"));
            }

            val = QString::fromStdString(IO_value_holder);
            StrL.insert(4,val);
            break;
        }
        /*case LVN_ITEMACTIVATE: {
            if(InSimulationMode) {
                char *name = Prog.io.assignment[h->item.iItem].name;
                if(name[0] == 'X') {
                    SimulationToggleContact(name);
                } else if(name[0] == 'A') {
                    ShowAnalogSliderPopup(name);
                }
            } else {
                UndoRemember();
                ShowIoDialog(h->item.iItem);
                ProgramChanged();
                InvalidateRect(MainWindow, NULL, FALSE);
            }
            break;
        }*/
    }
    h->hlistIter.insert(item,new QTreeWidgetItem(StrL));
}