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
// Dialog for setting the properties of a set of contacts: negated or not,
// plus the name
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <linuxUI.h>
#include <stdio.h>
//#include <commctrl.h>

#include "ldmicro.h"

static QDialog* ContactsDialog;

static QCheckBox* NegatedCheckbox;
static QRadioButton* SourceInternalRelayRadio;
static QRadioButton* SourceInputPinRadio;
static QRadioButton* SourceOutputPinRadio;
static QLineEdit*    NameTextbox;

static LONG_PTR PrevNameProc;
static QGridLayout* ContactsGrid;
char* tmpname;
BOOL* tmpnegated;

static void MakeControls(void)
{
    QGroupBox* grouper = new QGroupBox(_("Source"));
    QGridLayout *SourceGrid = new QGridLayout();
    QGridLayout *NameGrid = new QGridLayout();
    QDialogButtonBox *ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel, Qt::Vertical,
        ContactsDialog);
    NiceFont(ContactsDialog);

    ContactsGrid->setSpacing(3);
    SourceGrid->setSpacing(3);

    SourceInternalRelayRadio = new QRadioButton(_("Internal Relay"),
        ContactsDialog);
    SourceInputPinRadio = new QRadioButton(_("Input pin"),
        ContactsDialog);
    SourceOutputPinRadio = new QRadioButton(_("Output pin"),
        ContactsDialog);
    SourceGrid->addWidget(SourceInternalRelayRadio,0,0);
    SourceGrid->addWidget(SourceInputPinRadio,1,0);
    SourceGrid->addWidget(SourceOutputPinRadio,2,0);
    /*SourceGrid->addItem(
        new QSpacerItem(SetOnlyRadio->width(),
            SetOnlyRadio->height()), 2, 0);*/
    QLabel* textLabel = new QLabel(_("Name:"));
    NameTextbox = new QLineEdit();
    NegatedCheckbox = new QCheckBox(_("|/| Negated"), ContactsDialog);
    FixedFont(NameTextbox);
    NameTextbox->setFixedWidth(155);
    NameGrid->addWidget(textLabel,0,0);
    NameGrid->addWidget(NameTextbox,0,1);
    NameGrid->addWidget(NegatedCheckbox, 1,0);
    grouper->setLayout(SourceGrid);
    ContactsGrid->addWidget(grouper,0,0);
    ContactsGrid->addLayout(NameGrid,0,1);
    ContactsGrid->addWidget(ButtonBox,0,2);
    QObject::connect(ButtonBox,
        SIGNAL(accepted()),
        ContactsDialog,
        SLOT(accept()));
    QObject::connect(ButtonBox,
        SIGNAL(rejected()),
        ContactsDialog,
        SLOT(reject()));
}

static inline void DestroyWindow (){
    delete NegatedCheckbox;
    delete SourceInternalRelayRadio;
    delete SourceInputPinRadio;
    delete SourceOutputPinRadio;
    delete NameTextbox;
    delete ContactsDialog;
    ProgramChanged();
}

void ShowContactsDialog(BOOL *negated, char *name)
{
    ContactsDialog = CreateWindowClient(_("Contacts"),
        100, 100, 359, 115, MainWindow);
    ContactsGrid = new QGridLayout(ContactsDialog);
    ContactsDialog->setWindowTitle("Contacts");
    // CoilDialog->setFixedSize(359,115);
    MakeControls();
    NameTextbox->setValidator(
        new QRegularExpressionValidator(
                    QRegularExpression(QStringLiteral("[a-zA-Z0-9_]+"))));
    NameTextbox->setFocus();

    if(name[0] == 'R') {
        SourceInternalRelayRadio->setChecked(TRUE);
    } else if (name[0] == 'Y'){
        SourceOutputPinRadio->setChecked(TRUE);
    }
    else
    {
        SourceInputPinRadio->setChecked(TRUE);
    }
    NameTextbox->setText(name + 1);
    if(*negated) {
        NegatedCheckbox->setChecked(TRUE);
    } else{
        NegatedCheckbox->setChecked(FALSE);
    }

    int ret = ContactsDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            if(SourceInternalRelayRadio->isChecked())
            {
                name[0] = 'R';
            }else if(SourceInputPinRadio->isChecked()){
                name[0] = 'X';
            } else {
                name[0] = 'Y';
            }
            strncpy(name +1, NameTextbox->text().toStdString().c_str(),16);
            if(NegatedCheckbox->isChecked()) {
                *negated = TRUE;
            } else {
                *negated = FALSE;
            }
        }
        break;
        case QDialog::Rejected:
        break;
    }
    DestroyWindow();
}
