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
// Dialog for setting the properties of a relay coils: negated or not,
// plus the name, plus set-only or reset-only
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <linuxUI.h>
#include <stdio.h>
//#include <commctrl.h>

#include "ldmicro.h"

static QDialog* CoilDialog;

static QRadioButton* SourceInternalRelayRadio;
static QRadioButton* SourceMcuPinRadio;
static QRadioButton* NegatedRadio;
static QRadioButton* NormalRadio;
static QRadioButton* SetOnlyRadio;
static QRadioButton* ResetOnlyRadio;
static QLineEdit*    NameTextbox;

static LONG_PTR PrevNameProc;

static QGridLayout* CoilGrid;

static void MakeControls(void)
{
    QGroupBox* grouper = new QGroupBox(_("Type"));
    QGroupBox* grouper2 = new QGroupBox(_("Source"));
    QGridLayout *TypeGrid = new QGridLayout();
    QGridLayout *SourceGrid = new QGridLayout();
    QGridLayout *NameGrid = new QGridLayout();
    QDialogButtonBox *ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel, Qt::Vertical,
        CoilDialog);
    NormalRadio = new QRadioButton(_("( ) Normal"), CoilDialog);
    NegatedRadio = new QRadioButton(_("(/) Negated"), CoilDialog);
    SetOnlyRadio = new QRadioButton(_("(S) Set-Only"), CoilDialog);
    ResetOnlyRadio = new QRadioButton(_("(R) Reset-Only"), CoilDialog);
    NiceFont(CoilDialog);
    TypeGrid->addWidget(NormalRadio,0,0);
    TypeGrid->addWidget(NegatedRadio,1,0);
    TypeGrid->addWidget(SetOnlyRadio,2,0);
    TypeGrid->addWidget(ResetOnlyRadio,3,0);

    CoilGrid->setSpacing(3);
    TypeGrid->setSpacing(3);
    SourceGrid->setSpacing(3);

    SourceInternalRelayRadio = new QRadioButton(_("Internal Relay"), CoilDialog);
    SourceMcuPinRadio = new QRadioButton(_("Pin on MCU"), CoilDialog);
    SourceGrid->addWidget(SourceInternalRelayRadio,0,0);
    SourceGrid->addWidget(SourceMcuPinRadio,1,0);
    SourceGrid->addItem(
        new QSpacerItem(SetOnlyRadio->width(),
            SetOnlyRadio->height()), 2, 0);
    QLabel* textLabel = new QLabel(_("Name:"));
    NameTextbox = new QLineEdit();
    FixedFont(NameTextbox);
    NameTextbox->setFixedWidth(155);
    NameGrid->addWidget(textLabel,0,0);
    NameGrid->addWidget(NameTextbox,0,1);
    SourceGrid->addLayout(NameGrid,3,0);

    grouper->setLayout(TypeGrid);
    grouper2->setLayout(SourceGrid);
    CoilGrid->addWidget(grouper,0,0);
    CoilGrid->addWidget(grouper2,0,1);
    CoilGrid->addWidget(ButtonBox,0,2);
    QObject::connect(ButtonBox, SIGNAL(accepted()), CoilDialog, SLOT(accept()));
    QObject::connect(ButtonBox, SIGNAL(rejected()), CoilDialog, SLOT(reject()));
}

static inline void DestroyWindow()
{
    delete SourceInternalRelayRadio;
    delete SourceMcuPinRadio;
    delete NormalRadio;
    delete NegatedRadio;
    delete SetOnlyRadio;
    delete ResetOnlyRadio;
    delete NameTextbox;
    delete CoilGrid;
    ProgramChanged();
}

void ShowCoilDialog(BOOL *negated, BOOL *setOnly, BOOL *resetOnly, char *name)
{
    CoilDialog = CreateWindowClient(_("Coil"),
        100, 100, 359, 115, MainWindow);
    CoilGrid = new QGridLayout(CoilDialog);
    CoilDialog->setWindowTitle("Coil");
    // CoilDialog->setFixedSize(359,115);
    MakeControls();
    NameTextbox->setValidator(
        new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[a-zA-Z0-9_]+"))));

    if(name[0] == 'R') {
        SourceInternalRelayRadio->setChecked(TRUE);
    } else {
        SourceMcuPinRadio->setChecked(TRUE);
    }
    NameTextbox->setText(name + 1);
    if(*negated) {
        NegatedRadio->setChecked(TRUE);
    } else if(*setOnly) {
        SetOnlyRadio->setChecked(TRUE);
    } else if(*resetOnly) {
        ResetOnlyRadio->setChecked(TRUE);
    } else {
        NormalRadio->setChecked(TRUE);
    }
    NameTextbox->setFocus();
    int ret = CoilDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            if(SourceInternalRelayRadio->isChecked())
            {
                name[0] = 'R';
            } else {
                name[0] = 'Y';
            }
            strncpy(name +1, NameTextbox->text().toStdString().c_str(),16);
            if(NormalRadio->isChecked()) {
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
        } else if(NegatedRadio->isChecked()) {
            *negated = TRUE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
        } else if(SetOnlyRadio->isChecked()) {
            *negated = FALSE;
            *setOnly = TRUE;
            *resetOnly = FALSE;
        } else if(ResetOnlyRadio->isChecked())
        {
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = TRUE;
        }
        }
        break;
        case QDialog::Rejected:
        break;
    }
    DestroyWindow();
    return;
}
