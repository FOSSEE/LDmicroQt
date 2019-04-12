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
// All the simple dialogs that just require a row of text boxes: timer name
// and delay, counter name and count, move and arithmetic destination and
// operands. Try to reuse code a bit.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "linuxUI.h"
//#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"

static HWID SimpleDialog;
static HWID OkButton;
static HWID CancelButton;

#define MAX_BOXES 5

static HWID Textboxes[MAX_BOXES];
static HWID Labels[MAX_BOXES];

static LONG_PTR PrevAlnumOnlyProc[MAX_BOXES];
static LONG_PTR PrevNumOnlyProc[MAX_BOXES];

static BOOL NoCheckingOnBox[MAX_BOXES];

static BOOL SIMPLE_DIALOG_ACTIVE = FALSE;

static SimpleDialogData SDdata;

/// Simple dialog data flags
#define SD_TIMER            0x0000001
#define SD_COUNTER          0x0000002
#define SD_CMP              0x0000003
#define SD_MOVE             0x0000004
#define SD_READ_ADC         0x0000005
#define SD_SET_PWM          0x0000006
#define SD_UART             0x0000007
#define SD_MATH             0x0000008
#define SD_SHIFT_REGISTER   0x0000009
#define SD_FORMATTED_STRING 0x0000010
#define SD_PERSIST          0x0000011
/*
//-----------------------------------------------------------------------------
// Don't allow any characters other than -A-Za-z0-9_ in the box.
//-----------------------------------------------------------------------------
static void MyAlnumOnlyProc (GtkEditable *editable, gchar *NewText, gint length, 
    gint *position, gpointer data)
{
    for (int i = 0; i < length; i++)
    {
        if (!(isalpha (NewText[i]) || NewText[i] == '_' || isdigit (NewText[i]) ||
                NewText[i] == '\b' || NewText[i] == '\'' || NewText[i] == '-'))
        {
            g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");
            return;
        }
    }
    // if(msg == WM_CHAR) {
    //     if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' ||
    //         wParam == '\b' || wParam == '-' || wParam == '\''))
    //     {
    //         return 0;
    //     }
    // }

    // int i;
    // for(i = 0; i < MAX_BOXES; i++) {
    //     if(hwnd == Textboxes[i]) {
    //         return CallWindowProc((WNDPROC)PrevAlnumOnlyProc[i], hwnd, msg, 
    //             wParam, lParam);
    //     }
    // }
    // oops();
}

//-----------------------------------------------------------------------------
// Don't allow any characters other than -0-9. in the box.
//-----------------------------------------------------------------------------
static void MyNumOnlyProc (GtkEditable *editable, gchar *NewText, gint length, 
    gint *position, gpointer data)
{
    for (int i = 0; i < length; i++)
    {
        if (!(isdigit (NewText[i]) || NewText[i] == '\b' || 
            NewText[i] == '.' || NewText[i] == '-'))
        {
            g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");
            return;
        }
    }
    // if(msg == WM_CHAR) {
    //     if(!(isdigit(wParam) || wParam == '.' || wParam == '\b' 
    //         || wParam == '-'))
    //     {
    //         return 0;
    //     }
    // }

    // int i;
    // for(i = 0; i < MAX_BOXES; i++) {
    //     if(hwnd == Textboxes[i]) {
    //         return CallWindowProc((WNDPROC)PrevNumOnlyProc[i], hwnd, msg, 
    //             wParam, lParam);
    //     }
    // }
    // oops();
}

static void MakeControls(int boxes, char **labels, DWORD fixedFontMask)
{
    int i;
    // HDC hdc = GetDC(SimpleDialog);
    // SelectObject(hdc, MyNiceFont);

    // SIZE si;

    // int maxLen = 0;
    // for(i = 0; i < boxes; i++) {
    //     GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);
    //     if(si.cx > maxLen) maxLen = si.cx;
    // }

    // int adj;
    // if(maxLen > 70) {
    //     adj = maxLen - 70;
    // } else {
    //     adj = 0;
    // }
    HWID grid = gtk_grid_new();
    
    for(i = 0; i < boxes; i++) {
        // GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);

        Labels[i] = gtk_label_new (labels[i]);
        // CreateWindowEx(0, WC_STATIC, labels[i],
        //     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        //     (80 + adj) - si.cx - 4, 13 + i*30, si.cx, 21,
        //     SimpleDialog, NULL, Instance, NULL);
        NiceFont(Labels[i]);
        gtk_grid_attach (GTK_GRID (grid), Labels[i], 0, i, 1, 1);

        Textboxes[i] = gtk_entry_new ();
        // CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        //     WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS |
        //     WS_VISIBLE,
        //     80 + adj, 12 + 30*i, 120 - adj, 21,
        //     SimpleDialog, NULL, Instance, NULL);

        if(fixedFontMask & (1 << i)) {
            FixedFont(Textboxes[i]);
        } else {
            NiceFont(Textboxes[i]);
        }
        gtk_grid_attach (GTK_GRID (grid), Textboxes[i], 1, i, 1, 1);
    }
    // ReleaseDC(SimpleDialog, hdc);

    OkButton = gtk_button_new_with_label (_("OK"));
    // CreateWindowEx(0, WC_BUTTON, _("OK"),
    //     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
    //     218, 11, 70, 23, SimpleDialog, NULL, Instance, NULL); 
    NiceFont(OkButton);
    gtk_grid_attach (GTK_GRID (grid), OkButton, 2, 0, 1, 1);

    CancelButton = gtk_button_new_with_label(_("Cancel"));
    // CreateWindowEx(0, WC_BUTTON, _("Cancel"),
    //     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
    //     218, 41, 70, 23, SimpleDialog, NULL, Instance, NULL); 
    NiceFont(CancelButton);
    gtk_grid_attach (GTK_GRID (grid), CancelButton, 2, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(SimpleDialog), grid);
}

void SimpleDialogWrapUp()
{
    // if(!didCancel)
    for(int i = 0; i < SDdata.boxes; i++) {
        if(NoCheckingOnBox[i]) {
            // char get[64];
            // SendMessage(Textboxes[i], WM_GETTEXT, 60, (LPARAM)get);
            char *get = (char*)gtk_entry_get_text (GTK_ENTRY(Textboxes[i]));
            strcpy(SDdata.dests[i], get);
            if (strlen(get) < 60)
                strcpy(SDdata.dests[i], get);
            else
            {
                strncpy(SDdata.dests[i], get, 60);
                SDdata.dests[i][60] = '\0';
            }
        } else {
            char get[20];
            // SendMessage(Textboxes[i], WM_GETTEXT, 15, (LPARAM)get);
            char *str = (char*)gtk_entry_get_text (GTK_ENTRY(Textboxes[i]));
            strcpy(get, str);
            if (strlen(str) < 15)
                strcpy(get, str);
            else
            {
                strncpy(get, str, 15);
                get[15] = '\0';
            }

            if( (!strchr(get, '\'')) ||
                    (get[0] == '\'' && get[2] == '\'' && strlen(get)==3) )
            {
                if(strlen(get) == 0) {
                    Error(_("Empty textbox; not permitted."));
                } else {
                    strcpy(SDdata.dests[i], get);
                }
            } else {
                Error(_("Bad use of quotes: <%s>"), get);
            }
        }
    }

    switch(SDdata.uflag)
    {
        case SD_TIMER:
        {
            SDdata.str1[0] = 'T';
            strcpy(SDdata.str1+1, SDdata.dests[0]);
            //g_print("%s, %s\n", SDdata.str1, SDdata.dests[0]);
            double del = atof(SDdata.dests[1]);
            if(del > 2140000) { // 2**31/1000, don't overflow signed int
                Error(_("Delay too long; maximum is 2**31 us."));
            } else if(del <= 0) {
                Error(_("Delay cannot be zero or negative."));
            } else {
                *SDdata.num1 = (int)(1000*del + 0.5);
            }
            break;
        }
        case SD_COUNTER:
        {
            *SDdata.num1 = atoi(SDdata.dests[1]);
            break;
        }
        case SD_CMP:
        {
            break;
        }
        case SD_MOVE:
        {
            break;
        }
        case SD_READ_ADC:
        {
            break;
        }
        case SD_SET_PWM:
        {
            *SDdata.num1 = atoi(SDdata.dests[1]);
            break;
        }
        case SD_UART:
        {
            break;
        }
        case SD_MATH:
        {
            break;
        }
        case SD_SHIFT_REGISTER:
        {
            *SDdata.num1 = atoi(SDdata.dests[1]);

            if(*SDdata.num1 <= 0 || *SDdata.num1 >= 200) 
            {
                Error(_("Not a reasonable size for a shift register."));
                *SDdata.num1 = 1;
            }
            break;
        }
        case SD_FORMATTED_STRING:
        {
            break;
        }
        case SD_PERSIST:
        {
            break;
        }
    }

    DestroyWindow(SimpleDialog);
    ProgramChanged();
    SIMPLE_DIALOG_ACTIVE = FALSE;
}

void SimpleDialogCancelProc()
{
    DestroyWindow(SimpleDialog);
    ProgramChanged();
    SIMPLE_DIALOG_ACTIVE = FALSE;
}

static gboolean SimpleDialogKeyPressProc(HWID widget, GdkEventKey* event, gpointer data)
{
    if(event -> keyval == GDK_KEY_Return) 
    {
        // DialogDone = TRUE;
        SimpleDialogWrapUp();
    } else if(event -> keyval == GDK_KEY_Escape) 
    {
        // DialogDone = TRUE;
        // DialogCancel = TRUE;
        SimpleDialogCancelProc();
    }

    return FALSE;
}*/

void ShowSimpleDialog(char *title, int boxes, char **labels, DWORD numOnlyMask,
    DWORD alnumOnlyMask, DWORD fixedFontMask, char **dests)
{
    if(SIMPLE_DIALOG_ACTIVE)
        return;
    
    SIMPLE_DIALOG_ACTIVE = TRUE;

    BOOL didCancel = FALSE;

    if(boxes > MAX_BOXES) oops();

    SimpleDialog = CreateWindowClient(GTK_WINDOW_TOPLEVEL, GDK_WINDOW_TYPE_HINT_NORMAL, 
        title, 100, 100, 304, 15 + 30*(boxes < 2 ? 2 : boxes), MainWindow);
    SimpleDialog->show();
    // CreateWindowClient(0, "LDmicroDialog", title, 
    //     WS_OVERLAPPED | WS_SYSMENU,
    //     100, 100, 304, 15 + 30*(boxes < 2 ? 2 : boxes), NULL, NULL,
    //     Instance, NULL);

/*    MakeControls(boxes, labels, fixedFontMask);
  
    int i;
    for(i = 0; i < boxes; i++) 
    {
        // SendMessage(Textboxes[i], WM_SETTEXT, 0, (LPARAM)dests[i]);
        gtk_entry_set_text (GTK_ENTRY(Textboxes[i]), dests[i]);

        if(numOnlyMask & (1 << i)) 
        {
            g_signal_connect (G_OBJECT(Textboxes[i]), "insert-text",
                G_CALLBACK(MyNumOnlyProc), NULL);
            // PrevNumOnlyProc[i] = SetWindowLongPtr(Textboxes[i], GWLP_WNDPROC, 
            //     (LONG_PTR)MyNumOnlyProc);
        } else if(alnumOnlyMask & (1 << i)) 
        {
            g_signal_connect (G_OBJECT(Textboxes[i]), "insert-text",
                G_CALLBACK(MyAlnumOnlyProc), NULL);
            // PrevAlnumOnlyProc[i] = SetWindowLongPtr(Textboxes[i], GWLP_WNDPROC, 
            //     (LONG_PTR)MyAlnumOnlyProc);
        }
    }

    g_signal_connect (CancelButton, "clicked", G_CALLBACK (SimpleDialogCancelProc), NULL);
    g_signal_connect (OkButton, "clicked", G_CALLBACK (SimpleDialogWrapUp), NULL);
    g_signal_connect (SimpleDialog, "key_press_event", G_CALLBACK (SimpleDialogKeyPressProc), NULL);

    // EnableWindow(MainWindow, FALSE);
    // ShowWindow(SimpleDialog, TRUE);
    gtk_widget_show_all(SimpleDialog);
*/    // SetFocus(Textboxes[0]);
    // SendMessage(Textboxes[0], EM_SETSEL, 0, -1);

    // MSG msg;
    // DWORD ret;
    // DialogDone = FALSE;
    // DialogCancel = FALSE;
    // while((ret = GetMessage(&msg, NULL, 0, 0)) && !DialogDone) {
    //     if(msg.message == WM_KEYDOWN) {
    //         if(msg.wParam == VK_RETURN) {
    //             DialogDone = TRUE;
    //             break;
    //         } else if(msg.wParam == VK_ESCAPE) {
    //             DialogDone = TRUE;
    //             DialogCancel = TRUE;
    //             break;
    //         }
    //     }

    //     if(IsDialogMessage(SimpleDialog, &msg)) continue;
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }

    // didCancel = DialogCancel;



    // EnableWindow(MainWindow, TRUE);
    // DestroyWindow(SimpleDialog);

    // return !didCancel;
}

/*void ShowTimerDialog(int which, int *delay, char *name)
{
    char *s;
    switch(which) { 
        case ELEM_TON: s = _("Turn-On Delay"); break;
        case ELEM_TOF: s = _("Turn-Off Delay"); break;
        case ELEM_RTO: s = _("Retentive Turn-On Delay"); break;
        default: oops(); break;
    }
   
    char *labels[] = { _("Name:"), _("Delay (ms):") };

    char delBuf[16];
    char nameBuf[16];
    sprintf(delBuf, "%.3f", (*delay / 1000.0));
    strcpy(nameBuf, name+1);
    char *dests[] = { nameBuf, delBuf };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_TIMER;
    SDdata.boxes = 2;
    SDdata.str1 = name;
    SDdata.num1 = delay;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = new char[16];
    SDdata.dests[1] = new char[16];
    sprintf(SDdata.dests[1], "%.3f", (*delay / 1000.0));
    strcpy(SDdata.dests[0], name+1);

    ShowSimpleDialog(s, 2, labels, (1 << 1), (1 << 0), (1 << 0), dests);
}

void ShowCounterDialog(int which, int *maxV, char *name)
{
    char *title;

    switch(which) {
        case ELEM_CTU:  title = _("Count Up"); break;
        case ELEM_CTD:  title = _("Count Down"); break;
        case ELEM_CTC:  title = _("Circular Counter"); break;

        default: oops();
    }

    char *labels[] = { _("Name:"), (which == ELEM_CTC ? _("Max value:") : 
        _("True if >= :")) };
    char maxS[128];
    sprintf(maxS, "%d", *maxV);
    char *dests[] = { name+1, maxS };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_COUNTER;
    SDdata.boxes = 2;
    SDdata.str1 = name;
    SDdata.num1 = maxV;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = name+1;
    SDdata.dests[1] = new char[128];
    sprintf(SDdata.dests[1], "%d", *maxV);

    ShowSimpleDialog(title, 2, labels, 0x2, 0x1, 0x1, dests);
}

void ShowCmpDialog(int which, char *op1, char *op2)
{
    char *title;
    char *l2;
    switch(which) {
        case ELEM_EQU:
            title = _("If Equals");
            l2 = "= :";
            break;

        case ELEM_NEQ:
            title = _("If Not Equals");
            l2 = "/= :";
            break;

        case ELEM_GRT:
            title = _("If Greater Than");
            l2 = "> :";
            break;

        case ELEM_GEQ:
            title = _("If Greater Than or Equal To");
            l2 = ">= :";
            break;

        case ELEM_LES:
            title = _("If Less Than");
            l2 = "< :";
            break;

        case ELEM_LEQ:
            title = _("If Less Than or Equal To");
            l2 = "<= :";
            break;

        default:
            oops();
    }
    char *labels[] = { _("'Closed' if:"), l2 };
    char *dests[] = { op1, op2 };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_CMP;
    SDdata.boxes = 2;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = op1;
    SDdata.dests[1] = op2;

    ShowSimpleDialog(title, 2, labels, 0, 0x3, 0x3, dests);
}

void ShowMoveDialog(char *dest, char *src)
{
    char *labels[] = { _("Destination:"), _("Source:") };
    char *dests[] = { dest, src };
    
    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_MOVE;
    SDdata.boxes = 2;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = dest;
    SDdata.dests[1] = src;

    ShowSimpleDialog(_("Move"), 2, labels, 0, 0x3, 0x3, dests);
}

void ShowReadAdcDialog(char *name)
{
    char *labels[] = { _("Destination:") };
    char *dests[] = { name };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_READ_ADC;
    SDdata.boxes = 1;
    SDdata.dests = new char*[1];
    SDdata.dests[0] = name;

    ShowSimpleDialog(_("Read A/D Converter"), 1, labels, 0, 0x1, 0x1, dests);
}

void ShowSetPwmDialog(char *name, int *targetFreq)
{
    char freq[100];
    sprintf(freq, "%d", *targetFreq);

    char *labels[] = { _("Duty cycle var:"), _("Frequency (Hz):") };
    char *dests[] = { name, freq };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_SET_PWM;
    SDdata.boxes = 2;
    SDdata.num1 = targetFreq;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = name;
    SDdata.dests[1] = new char[100];
    sprintf(SDdata.dests[1], "%d", *targetFreq);

    ShowSimpleDialog(_("Set PWM Duty Cycle"), 2, labels, 0x2, 0x1, 0x1, dests);
}

void ShowUartDialog(int which, char *name)
{
    char *labels[] = { (which == ELEM_UART_RECV) ? _("Destination:") :
        _("Source:") };
    char *dests[] = { name };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_UART;
    SDdata.boxes = 1;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = name;

    ShowSimpleDialog((which == ELEM_UART_RECV) ? _("Receive from UART") :
        _("Send to UART"), 1, labels, 0, 0x1, 0x1, dests);
}

void ShowMathDialog(int which, char *dest, char *op1, char *op2)
{
    char *l2, *title;
    if(which == ELEM_ADD) {
        l2 = "+ :";
        title = _("Add");
    } else if(which == ELEM_SUB) {
        l2 = "- :";
        title = _("Subtract");
    } else if(which == ELEM_MUL) {
        l2 = "* :";
        title = _("Multiply");
    } else if(which == ELEM_DIV) {
        l2 = "/ :";
        title = _("Divide");
    } else oops();

    char *labels[] = { _("Destination:"), _("is set := :"), l2 };
    char *dests[] = { dest, op1, op2 };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_MATH;
    SDdata.boxes = 3;
    SDdata.dests = new char*[3];
    SDdata.dests[0] = dest;
    SDdata.dests[1] = op1;
    SDdata.dests[2] = op2;

    ShowSimpleDialog(title, 3, labels, 0, 0x7, 0x7, dests);
}

void ShowShiftRegisterDialog(char *name, int *stages)
{
    char stagesStr[20];
    sprintf(stagesStr, "%d", *stages);

    char *labels[] = { _("Name:"), _("Stages:") };
    char *dests[] = { name, stagesStr };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_SHIFT_REGISTER;
    SDdata.boxes = 2;
    SDdata.num1 = stages;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = name;
    SDdata.dests[1] = new char[20];
    sprintf(SDdata.dests[1], "%d", *stages);

    ShowSimpleDialog(_("Shift Register"), 2, labels, 0x2, 0x1, 0x1, dests);
}

void ShowFormattedStringDialog(char *var, char *string)
{
    char *labels[] = { _("Variable:"), _("String:") };
    char *dests[] = { var, string };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_FORMATTED_STRING;
    SDdata.boxes = 2;
    SDdata.dests = new char*[2];
    SDdata.dests[0] = var;
    SDdata.dests[1] = string;

    NoCheckingOnBox[0] = TRUE;
    NoCheckingOnBox[1] = TRUE;
    ShowSimpleDialog(_("Formatted String Over UART"), 2, labels, 0x0,
        0x1, 0x3, dests);
    NoCheckingOnBox[0] = FALSE;
    NoCheckingOnBox[1] = FALSE;
}

void ShowPersistDialog(char *var)
{
    char *labels[] = { _("Variable:") };
    char *dests[] = { var };

    if (SIMPLE_DIALOG_ACTIVE)
        return;
    
    SDdata.uflag = SD_PERSIST;
    SDdata.boxes = 1;
    SDdata.dests = new char*[1];
    SDdata.dests[0] = var;

    ShowSimpleDialog(_("Make Persistent"), 1, labels, 0, 1, 1, dests);
}
*/