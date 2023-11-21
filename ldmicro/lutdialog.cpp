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
// Dialog for entering the elements of a look-up table. I allow two formats:
// as a simple list of integer values, or like a string. The lookup table
// can either be a straight LUT, or one with piecewise linear interpolation
// in between the points.
// Jonathan Westhues, Dec 2005
//-----------------------------------------------------------------------------
#include "linuxUI.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include <commctrl.h>

#include "ldmicro.h"

static QDialog* LutDialog;

static QCheckBox* AsStringCheckbox;
static QLineEdit* CountTextbox;
static QLineEdit* DestTextbox;
static QLineEdit* IndexTextbox;
static QLabel* Labels[3];

static QLineEdit* StringTextbox;
static bool checkString;

static BOOL WasAsString;
static BOOL asString;
static int ControlCount;
static int WasCount;

static QLineEdit* ValuesTextbox[MAX_LOOK_UP_TABLE_LEN];
static LONG_PTR PrevValuesProc[MAX_LOOK_UP_TABLE_LEN];
static QLabel* ValuesLabel[MAX_LOOK_UP_TABLE_LEN];

static SWORD ValuesCache[MAX_LOOK_UP_TABLE_LEN];

static LONG_PTR PrevDestProc;
static LONG_PTR PrevIndexProc;
static LONG_PTR PrevCountProc;
static QDialogButtonBox* ButtonBox;
static UINT LUT_DIALOG_REFRESH_TIMER_ID_1 = 0;
static UINT LUT_DIALOG_REFRESH_TIMER_ID_2 = 0;

static QGridLayout* LutGrid;
static QGridLayout* LutPackingBox;
static QGridLayout* FixedControlGrid;
static QGridLayout* LutControlGrid;
char PrevTableAsString[1024];

/*struct LookUpTableDialogBuffer{
    int tmpcount;
    bool tmpasString;
    char PrevTableAsString[1024] = "";
} temp;*/

static int piecewiseTmpCount;
void CheckBoxFunction(int state);
void CountFunction(QString text);
void StringFunction(QString text);


static inline void DestroyWindow (){
    delete Labels[0];
    delete Labels[1];
    delete Labels[2];
    delete DestTextbox;
    delete IndexTextbox;
    delete CountTextbox;
    if(WasAsString)
        delete AsStringCheckbox;
    delete FixedControlGrid;
    delete LutGrid;
    delete ButtonBox;
    delete LutDialog;
    ProgramChanged();
}

//-----------------------------------------------------------------------------
// Make the controls that are guaranteed not to move around as the count/
// as string settings change. This is different for the piecewise linear,
// because in that case we should not provide a checkbox to change whether
// the table is edited as a string or table.
//-----------------------------------------------------------------------------
static void MakeFixedControls(BOOL forPwl)
{
    bool madeCheckbox = FALSE;
    FixedControlGrid = new QGridLayout();
    Labels[0] = new QLabel(_("Destination"));
    Labels[1] = new QLabel(_("Index:"));
    Labels[2] = new QLabel(forPwl ? _("Points:") : _("Count:"));

    DestTextbox = new QLineEdit();
    IndexTextbox = new QLineEdit();
    CountTextbox = new QLineEdit();
    DestTextbox->setValidator(
        new QRegularExpressionValidator(
                    QRegularExpression(QStringLiteral("[a-zA-Z0-9_]+"))));
    IndexTextbox->setValidator(
        new QRegularExpressionValidator(
                    QRegularExpression(QStringLiteral("[a-zA-Z0-9_]+"))));
    CountTextbox->setValidator(
        new QRegularExpressionValidator(
                    QRegularExpression(QStringLiteral("[0-9]+"))));

    if(!forPwl) {
        AsStringCheckbox = new QCheckBox(
            _("Edit table of ASCII values like a string"), LutDialog);
    }
    FixedControlGrid->addWidget(Labels[0],0,0);
    FixedControlGrid->addWidget(DestTextbox,0,1);
    FixedControlGrid->addWidget(Labels[1],1,0);
    FixedControlGrid->addWidget(IndexTextbox,1,1);
    FixedControlGrid->addWidget(Labels[2],2,0);
    FixedControlGrid->addWidget(CountTextbox,2,1);
    if (!forPwl){
        FixedControlGrid->addWidget(AsStringCheckbox, 3, 0, 1, 0,
            Qt::AlignTop);
    }
    LutGrid->addLayout(FixedControlGrid, 0, 0,
        Qt::AlignLeft | Qt::AlignTop);
    ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel, Qt::Vertical,
        LutDialog);
    LutGrid->addWidget(ButtonBox,0,1);
    QObject::connect(ButtonBox, SIGNAL(accepted()), LutDialog, SLOT(accept()));
    QObject::connect(ButtonBox, SIGNAL(rejected()), LutDialog, SLOT(reject()));
    if (!forPwl)
    {
        QObject::connect(AsStringCheckbox,
            &QCheckBox::stateChanged, CheckBoxFunction);
    }
    QObject::connect(CountTextbox, &QLineEdit::textChanged,CountFunction);
}

//-----------------------------------------------------------------------------
// Destroy all of the controls so that we can start anew. This is necessary
// because if the size of the LUT changes, or if the user switches from 
// table entry to string entry, we must completely reconfigure the dialog.
//-----------------------------------------------------------------------------
static void DestroyLutControls(BOOL destroyFlag = TRUE)
{
    if(WasAsString) {
        // Nothing to do; we constantly update the cache from the user-
        // specified string, because we might as well do that when we
        // calculate the length.
        delete StringTextbox;
    }
    else {
        int i;
        for(i = 0; i < WasCount; i++) {
            const char* buf;
            buf = ValuesTextbox[i]->text().toStdString().c_str();
            ValuesCache[i] = atoi(buf);
            delete ValuesTextbox[i];
            delete ValuesLabel[i];
        }
        delete LutControlGrid;
    }
}


//-----------------------------------------------------------------------------
// Make the controls that hold the LUT. The exact configuration of the dialog
// will depend on (a) whether the user chose table-type or string-type entry,
// and for table-type entry, on (b) the number of entries, and on (c)
// whether we are editing a PWL table (list of points) or a straight LUT.
//-----------------------------------------------------------------------------
static void MakeLutControls(BOOL asString, int counts, BOOL forPwl)
{
    // Remember these, so that we know from where to cache stuff if we have
    // to destroy these textboxes and make something new later.
    WasAsString = asString;
    WasCount = counts;
    if(forPwl && asString) oops();

    if(asString) {
        char str[3*MAX_LOOK_UP_TABLE_LEN+1];
        int i, j;
        j = 0;
        for(i = 0; i < counts; i++) {
            int c = ValuesCache[i];
            if(c >= 32 && c <= 127 && c != '\\') {
                str[j++] = c;
            } else if(c == '\\') {
                str[j++] = '\\';
                str[j++] = '\\';
            } else if(c == '\r') {
                str[j++] = '\\';
                str[j++] = 'r';
            } else if(c == '\b') {
                str[j++] = '\\';
                str[j++] = 'b';
            } else if(c == '\f') {
                str[j++] = '\\';
                str[j++] = 'f';
            } else if(c == '\n') {
                str[j++] = '\\';
                str[j++] = 'n';
            } else {
                str[j++] = 'X';
            }
        }
        str[j++] = '\0';
            StringTextbox = new QLineEdit();
            FixedFont(StringTextbox);
            FixedControlGrid->addWidget(StringTextbox, 4, 0, 1, 0,
            Qt::AlignTop);
            StringTextbox->setText(str);
            QObject::connect(StringTextbox, &QLineEdit::textChanged,StringFunction);
            /*checkString = TRUE;*/
            CountTextbox->setReadOnly(TRUE);
    }
    else {
        LutControlGrid = new QGridLayout();
        int i;
        int base;
        if(forPwl) {
            base = 100;
            }
        else {
            base = 140;
            }
        for(i = 0; i < counts; i++) {
            int x, y;

            if(i < 16) {
                x = i;
                y = 0;
                }
            else {
                x = i % 16;
                y = 2;
                }

            char buf[20];
            sprintf(buf, "%d", ValuesCache[i]);
            ValuesTextbox[i] = new QLineEdit();
            ValuesTextbox[i]->setMaximumWidth(80);
            ValuesTextbox[i]->setValidator(
                new QRegularExpressionValidator(
                    QRegularExpression(QStringLiteral("-?[0-9]+"))));
            NiceFont(ValuesTextbox[i]);
            ValuesTextbox[i]->setText(buf);
            LutControlGrid->addWidget(ValuesTextbox[i], x, y + 1);

            if(forPwl) {
                sprintf(buf, "%c%d:", (i & 1) ? 'y' : 'x', i/2);
                }
            else {
                sprintf(buf, "%2d:", i);
                }

            ValuesLabel[i] = new QLabel(buf);
            FixedFont(ValuesLabel[i]);
            LutControlGrid->addWidget(ValuesLabel[i], x , y);
        }
            FixedControlGrid->addLayout(LutControlGrid, 4, 0, 1, 0,
            Qt::AlignTop);
        CountTextbox->setReadOnly(FALSE);
    }
}


//-----------------------------------------------------------------------------
// Decode a string into a look-up table; store the values in ValuesCache[],
// and update the count checkbox (which in string mode is read-only) to
// reflect the new length. Returns FALSE if the new string is too long, else
// TRUE.
//-----------------------------------------------------------------------------
BOOL StringToValuesCache(char *str, int *c)
{   
    int count = 0;
    while(*str) {
        if(*str == '\\') {
            str++;
            switch(*str) {
                case 'r': ValuesCache[count++] = '\r'; break;
                case 'n': ValuesCache[count++] = '\n'; break;
                case 'f': ValuesCache[count++] = '\f'; break;
                case 'b': ValuesCache[count++] = '\b'; break;
                default:  ValuesCache[count++] = *str; break;
            }
        } else {
            ValuesCache[count++] = *str;
        }
        if(*str) {
            str++;
        }
        if(count >= 32) {
            return FALSE;
        }
    }

    char buf[10];
    sprintf(buf, "%d", count);
    CountTextbox->setText(buf);
    *c = count;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Show the look-up table dialog. This one is nasty, mostly because there are
// two ways to enter a look-up table: as a table, or as a string. Presumably
// I should convert between those two representations on the fly, as the user
// edit things, so I do.
//-----------------------------------------------------------------------------
void ShowLookUpTableDialog(ElemLeaf *l)
{
    ElemLookUpTable *t = &(l->d.lookUpTable);

    // First copy over all the stuff from the leaf structure; in particular,
    // we need our own local copy of the table entries, because it would be
    // bad to update those in the leaf before the user clicks okay (as he
    // might cancel).

    ControlCount = t->count;
    asString = t->editAsString;
    memset(ValuesCache, 0, sizeof(ValuesCache));
    int i;
    for(i = 0; i < ControlCount; i++) {
            ValuesCache[i] = t->vals[i];
    }

    // Now create the dialog's fixed controls, plus the changing (depending
    // on show style/entry count) controls for the initial configuration.

    LutDialog = CreateWindowClient(_("Look-Up Table"),
        100, 100, 320, 375, MainWindow);
    LutPackingBox = new QGridLayout();
    LutGrid = new QGridLayout(LutDialog);
    LutDialog->setWindowTitle(_("Look-Up Table"));
    MakeFixedControls(FALSE);
    MakeLutControls(asString, ControlCount, FALSE);
    DestTextbox->setText(t->dest);
    IndexTextbox->setText(t->index);
    char buf[30];
    sprintf(buf, "%d", t->count);
    CountTextbox->setText(buf);
    AsStringCheckbox->setChecked(asString);
    DestTextbox->setFocus();
    PrevTableAsString[0] = NULL;
    int ret = LutDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            strncpy(t->dest,
                DestTextbox->text().toStdString().c_str(),16);
            strncpy(t->index,
                IndexTextbox->text().toStdString().c_str(),16);
            int count = ControlCount;
            DestroyLutControls();
            // The call to DestroyLutControls updated ValuesCache, so just read
            // them out of there (whichever mode we were in before).
            int i;
            for(i = 0; i < count; i++) {
                t->vals[i] = ValuesCache[i];
                // printf("Value:%d\n", ValuesCache[i]);
            }
            t->count = count;
            t->editAsString = asString;
            // printf("Count:%d\n", count);
        }
        break;
        case  QDialog::Rejected:
        break;
    }
    DestroyWindow();
}

// The function to handle checkbox event separated from 
// ShowLookUpTableDialog function to improve performance in Qt

void CheckBoxFunction(int state)
{
    // printf("CheckBoxFunctionCalled\n");
    asString = state;
    DestroyLutControls();
    const char* buf;
            buf = CountTextbox->text().toStdString().c_str();
    MakeLutControls(asString, atoi(buf), FALSE);
}

// The function to handle change in CountTextbox separated from 
// ShowLookUpTableDialog function to improve performance in Qt
void CountFunction(QString text)
{
    // printf("TextFunction called:%s\n",text.toStdString().c_str());
    const char* buf;
            buf = text.toStdString().c_str();
        if(atoi(buf) != ControlCount && !asString) {
            ControlCount = atoi(buf);
            if(ControlCount < 0 || ControlCount > 32) {
                ControlCount = 0;
                CountTextbox->setText("");
            }
            DestroyLutControls();
            MakeLutControls(asString, ControlCount, FALSE);
        }
}

// The function to handle change in StringTextbox separated from 
// ShowLookUpTableDialog function to improve performance in Qt
void StringFunction(QString text)
{
    char* scratch = (char*)text.toStdString().c_str();
            if(strcmp(scratch, PrevTableAsString)!=0) {
                if(StringToValuesCache(scratch, &ControlCount)) {
                    strcpy(PrevTableAsString, scratch);
                } else {
                    StringTextbox->setText(PrevTableAsString);
                }
            }
}

//-----------------------------------------------------------------------------
// Show the piecewise linear table dialog. This one can only be edited in
// only a single format, which makes things easier than before.
// //-----------------------------------------------------------------------------
void ShowPiecewiseLinearDialog(ElemLeaf *l)
{
    ElemPiecewiseLinear *t = &(l->d.piecewiseLinear);

    // First copy over all the stuff from the leaf structure; in particular,
    // we need our own local copy of the table entries, because it would be
    // bad to update those in the leaf before the user clicks okay (as he
    // might cancel).
    int count = t->count;
    memset(ValuesCache, 0, sizeof(ValuesCache));
    int i;
    for(i = 0; i < count*2; i++) {
        ValuesCache[i] = t->vals[i];
    }

    // Now create the dialog's fixed controls, plus the changing (depending
    // on show style/entry count) controls for the initial configuration.
    LutDialog = CreateWindowClient(_("Piecewise Linear Table"),
        100, 100, 320, 375, MainWindow);
    LutPackingBox = new QGridLayout();
    LutGrid = new QGridLayout(LutDialog);
    LutDialog->setWindowTitle(_("Piecewise Linear Table"));
    MakeFixedControls(TRUE);
    MakeLutControls(FALSE, ControlCount, TRUE);
    DestTextbox->setText(t->dest);
    IndexTextbox->setText(t->index);
    char buf[30];
    sprintf(buf, "%d", t->count);
    CountTextbox->setText(buf);
    int ret = LutDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            strncpy(t->dest,
                DestTextbox->text().toStdString().c_str(),16);
            strncpy(t->index,
                IndexTextbox->text().toStdString().c_str(),16);
            int count = ControlCount;
            DestroyLutControls();
            // The call to DestroyLutControls updated ValuesCache, so just read
            // them out of there (whichever mode we were in before).
            int i;
            for(i = 0; i < count; i++) {
                t->vals[i] = ValuesCache[i];
            }
            t->count = count;
        }
        break;
        case  QDialog::Rejected:
        break;
    }
    DestroyWindow();
}