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
// Dialog for setting the properties of a set of a RES reset element: name,
// which can be that of either a timer or a counter.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "linuxUI.h"
#include <stdio.h>
//#include <commctrl.h>
#include <iostream>
#include "ldmicro.h"

static QDialog* ResetDialog;

static QRadioButton* TypeTimerRadio;
static QRadioButton* TypeCounterRadio;
static QLineEdit*    NameTextbox;
static QDialogButtonBox* ButtonBox;

// static LONG_PTR PrevNameProc;
static QGridLayout* ResetGrid;
// static HWID ResetPackingBox;

static void MakeControls(void)
{
    QGroupBox* grouper = new QGroupBox(_("Type"));
    QGridLayout *TypeGrid = new QGridLayout();
    QGridLayout *NameGrid = new QGridLayout();
    ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel, Qt::Vertical,
        ResetDialog);
    NiceFont(ResetDialog);

    ResetGrid->setSpacing(3);
    TypeGrid->setSpacing(3);

    TypeTimerRadio = new QRadioButton(_("Timer"), ResetDialog);
    TypeCounterRadio = new QRadioButton(_("Counter"), ResetDialog);
    TypeGrid->addWidget(TypeTimerRadio,0,0);
    TypeGrid->addWidget(TypeCounterRadio,1,0);
    /*SourceGrid->addItem(
        new QSpacerItem(SetOnlyRadio->width(),
            SetOnlyRadio->height()), 2, 0);*/
    QLabel* textLabel = new QLabel(_("Name:"));
    NameTextbox = new QLineEdit();
    FixedFont(NameTextbox);
    NameTextbox->setFixedWidth(155);
    NameGrid->addWidget(textLabel,0,0);
    NameGrid->addWidget(NameTextbox,0,1);
    grouper->setLayout(TypeGrid);
    ResetGrid->addWidget(grouper,0,0);
    ResetGrid->addLayout(NameGrid,0,1);
    ResetGrid->addWidget(ButtonBox,0,2);
    QObject::connect(ButtonBox, SIGNAL(accepted()), ResetDialog, SLOT(accept()));
    QObject::connect(ButtonBox, SIGNAL(rejected()), ResetDialog, SLOT(reject()));
}

static inline void DestroyWindow (){
    delete TypeTimerRadio;
    delete TypeCounterRadio;
    delete NameTextbox;
    delete ButtonBox;
    delete ResetGrid;
    delete ResetDialog;
    ProgramChanged();
}

void ShowResetDialog(char *name)
{
    ResetDialog = CreateWindowClient(_("Reset"),
        100, 100, 359, 115, MainWindow);
    ResetGrid = new QGridLayout(ResetDialog);
    ResetDialog->setWindowTitle("Reset");
    // CoilDialog->setFixedSize(359,115);
    MakeControls();
    NameTextbox->setValidator(
        new QRegExpValidator(QRegExp("[a-zA-Z0-9_]+")));

    if(name[0] == 'T') {
        TypeTimerRadio->setChecked(TRUE);
    } else{
        TypeCounterRadio->setChecked(TRUE);
    }
    NameTextbox->setText(name + 1);
    NameTextbox->setFocus();

    int ret = ResetDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            if(TypeTimerRadio->isChecked())
            {
                name[0] = 'T';
            }else {
                name[0] = 'C';
            }
            strncpy(name +1, NameTextbox->text().toStdString().c_str(),16);
        }
        break;
        case QDialog::Rejected:
        break;
    }
    DestroyWindow();
}
