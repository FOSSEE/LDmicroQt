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

static QDialog*     SimpleDialog;

#define MAX_BOXES 5

static QLineEdit*   Textboxes[MAX_BOXES];
static QLabel*      Labels[MAX_BOXES];

static QGridLayout* SimpleGrid;

/*static LONG_PTR PrevAlnumOnlyProc[MAX_BOXES];
static LONG_PTR PrevNumOnlyProc[MAX_BOXES];*/

static BOOL NoCheckingOnBox[MAX_BOXES];

/*static BOOL SIMPLE_DIALOG_ACTIVE = FALSE;*/

/*static SimpleDialogData SDdata;*/

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

static inline void DestroyWindow (int boxes){
    for(int i = 0; i < boxes; i++) {
        delete Labels[i];
        delete Textboxes[i];
    }
    delete SimpleGrid;
    delete SimpleDialog;
    ProgramChanged();
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
    QGridLayout* grid = new QGridLayout();
    QDialogButtonBox *ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel, Qt::Vertical,
        SimpleDialog);
    NiceFont(SimpleDialog);
    
    for(i = 0; i < boxes; i++) {
        // GetTextExtentPoint32(hdc, labels[i], strlen(labels[i]), &si);

        Labels[i] = new QLabel(labels[i]);
        NiceFont(Labels[i]);
        grid->addWidget(Labels[i],i,0);

        Textboxes[i] = new QLineEdit();

        if(fixedFontMask & (1 << i)) {
            FixedFont(Textboxes[i]);
        } else {
            NiceFont(Textboxes[i]);
        }
        grid->addWidget(Textboxes[i],i,1);
    }
    SimpleGrid->addLayout(grid, 0,0);
    SimpleGrid->addWidget(ButtonBox, 0, 1);
    QObject::connect(ButtonBox, SIGNAL(accepted()),
        SimpleDialog, SLOT(accept()));
    QObject::connect(ButtonBox, SIGNAL(rejected()),
        SimpleDialog, SLOT(reject()));
}

BOOL ShowSimpleDialog(char *title, int boxes, char **labels, DWORD numOnlyMask,
    DWORD alnumOnlyMask, DWORD fixedFontMask, char **dests)
{
    /*if(SIMPLE_DIALOG_ACTIVE)
        return;
    
    SIMPLE_DIALOG_ACTIVE = TRUE;

    BOOL didCancel = FALSE;*/
    BOOL RetVal = FALSE;

    if(boxes > MAX_BOXES) oops();

    SimpleDialog = CreateWindowClient(title, 100, 100,
        304, 15 + 30*(boxes < 2 ? 2 : boxes), MainWindow);
    SimpleGrid = new QGridLayout(SimpleDialog);
    SimpleDialog->setWindowTitle(title);
    MakeControls(boxes, labels, fixedFontMask);
    int i;
    for(i = 0; i < boxes; i++) {
        Textboxes[i]->setText(dests[i]);

        if(numOnlyMask & (1 << i)) {
            Textboxes[i]->setValidator(
        new QRegularExpressionValidator(QRegularExpression(QStringLiteral("-?[0-9]+[.]?[0-9]+"))));
        }
        if(alnumOnlyMask & (1 << i)) {
            Textboxes[i]->setValidator(
                new QRegularExpressionValidator(
                    QRegularExpression(QStringLiteral("[a-zA-Z0-9_'-]+"))));
        }
    }
    Textboxes[0]->setFocus();
    int ret = SimpleDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            for(i = 0; i < boxes; i++) {
                if(NoCheckingOnBox[i]) {
                    strncpy(dests[i],
                        Textboxes[i]->text().toStdString().c_str(),60);
                } else {
                    char get[20];
                    // SendMessage(Textboxes[i], WM_GETTEXT, 15, (LPARAM)get);
                    strncpy(get, Textboxes[i]->text().toStdString().c_str(),15);

                    if( (!strchr(get, '\'')) ||
                        (get[0] == '\'' && get[2] == '\'' && strlen(get)==3) )
                    {
                        if(strlen(get) == 0) {
                            Error(_("Empty textbox; not permitted."));
                        } else {
                            strcpy(dests[i], get);
                        }
                    } else {
                        Error(_("Bad use of quotes: <%s>"), get);
                    }
                }
            }
            RetVal = TRUE;
            break;
        }
        case  QDialog::Rejected:
        RetVal = FALSE;
        break;
    }
    DestroyWindow(boxes);
    return RetVal;
}

void ShowTimerDialog(int which, int *delay, char *name)
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
    if(ShowSimpleDialog(s, 2, labels, (1 << 1), (1 << 0), (1 << 0), dests)) {
        name[0] = 'T';
        strcpy(name+1, nameBuf);
        double del = atof(delBuf);
        if(del > 2140000) { // 2**31/1000, don't overflow signed int
            Error(_("Delay too long; maximum is 2**31 us."));
        } else if(del <= 0) {
            Error(_("Delay cannot be zero or negative."));
        } else {
            *delay = (int)(1000*del + 0.5);
        }
    }
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
    ShowSimpleDialog(title, 2, labels, 0x2, 0x1, 0x1, dests);
    *maxV = atoi(maxS);
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
    ShowSimpleDialog(title, 2, labels, 0, 0x3, 0x3, dests);
}

void ShowMoveDialog(char *dest, char *src)
{
    char *labels[] = { _("Destination:"), _("Source:") };
    char *dests[] = { dest, src };
    ShowSimpleDialog(_("Move"), 2, labels, 0, 0x3, 0x3, dests);
}

void ShowReadAdcDialog(char *name)
{
    char *labels[] = { _("Destination:") };
    char *dests[] = { name };
    ShowSimpleDialog(_("Read A/D Converter"), 1, labels, 0, 0x1, 0x1, dests);
}

void ShowSetPwmDialog(char *name, int *targetFreq)
{
    char freq[100];
    sprintf(freq, "%d", *targetFreq);

    char *labels[] = { _("Duty cycle var:"), _("Frequency (Hz):") };
    char *dests[] = { name, freq };
    ShowSimpleDialog(_("Set PWM Duty Cycle"), 2, labels, 0x2, 0x1, 0x1, dests);
    *targetFreq = atoi(freq);
}

void ShowUartDialog(int which, char *name)
{
    char *labels[] = { (which == ELEM_UART_RECV) ? _("Destination:") :
        _("Source:") };
    char *dests[] = { name };

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
    ShowSimpleDialog(title, 3, labels, 0, 0x7, 0x7, dests);
}

void ShowShiftRegisterDialog(char *name, int *stages)
{
    char stagesStr[20];
    sprintf(stagesStr, "%d", *stages);

    char *labels[] = { _("Name:"), _("Stages:") };
    char *dests[] = { name, stagesStr };

    ShowSimpleDialog(_("Shift Register"), 2, labels, 0x2, 0x1, 0x1, dests);
    *stages = atoi(stagesStr);

    if(*stages <= 0 || *stages >= 200) {
        Error(_("Not a reasonable size for a shift register."));
        *stages = 1;
    }
}

void ShowFormattedStringDialog(char *var, char *string)
{
    char *labels[] = { _("Variable:"), _("String:") };
    char *dests[] = { var, string };
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
    ShowSimpleDialog(_("Make Persistent"), 1, labels, 0, 1, 1, dests);
}
