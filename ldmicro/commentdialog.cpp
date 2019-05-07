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
// Dialog to enter the text of a comment; make it long and skinny to
// encourage people to write it the way it will look on the diagram.
// Jonathan Westhues, Jun 2005
//-----------------------------------------------------------------------------
#include <linuxUI.h>
#include <stdio.h>
//#include <commctrl.h>

#include "ldmicro.h"

static QDialog* CommentDialog;

static QLineEdit* CommentTextbox;

static QGridLayout* CommentGrid;

static void MakeControls(void)
{
    NiceFont(CommentDialog);
    QDialogButtonBox *ButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok
        | QDialogButtonBox::Cancel, Qt::Vertical,
        CommentDialog);
    CommentTextbox = new QLineEdit();
    CommentGrid->addWidget(CommentTextbox, 0, 0);
    CommentGrid->addWidget(ButtonBox, 0, 1);
    QObject::connect(ButtonBox,
        SIGNAL(accepted()),
        CommentDialog,
        SLOT(accept()));
    QObject::connect(ButtonBox,
        SIGNAL(rejected()),
        CommentDialog,
        SLOT(reject()));
    FixedFont(CommentTextbox);
}

static inline void DestroyWindow(){
    delete CommentDialog;
    delete CommentTextbox;
    delete CommentGrid;
    ProgramChanged();
}

void ShowCommentDialog(char *comment)
{    
    CommentDialog = CreateWindowClient(_("Comment"),
        100, 100, 359, 115, MainWindow);
    CommentGrid = new QGridLayout(CommentDialog);
    MakeControls();
    CommentTextbox->setText(comment);
    CommentTextbox->setFocus();
    int ret = CommentDialog->exec();
    switch(ret)
    {
        case QDialog::Accepted:
        {
            strncpy(comment,
                CommentTextbox->text().toStdString().c_str(),
                MAX_COMMENT_LEN -1);
            
        }
        break;
        case QDialog::Rejected:
        break;
    }
    DestroyWindow();
}
