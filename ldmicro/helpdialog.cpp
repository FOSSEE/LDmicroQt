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
// Window to show our internal help. Roll my own using a rich edit control so
// that I don't have to distribute a separate Windows Help or HTML file; the
// manual is compiled in to the exe. Do pretty syntax highlighting style
// colours.
// Jonathan Westhues, Dec 2004
//-----------------------------------------------------------------------------
#include "linuxUI.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include <commctrl.h>
//#include <richedit.h>

#include "ldmicro.h"

extern char *HelpText[];
extern char *HelpTextDe[];
extern char *HelpTextFr[];
extern char *HelpTextTr[];

static char *AboutText[] = {
"",
"ABOUT LDMICRO",
"=============",
"",
"LDmicro is a ladder logic editor, simulator and compiler for 8-bit",
"microcontrollers. It can generate native code for Atmel AVR and Microchip",
"PIC16 CPUs from a ladder diagram.",
"",
"This program is free software: you can redistribute it and/or modify it",
"under the terms of the GNU General Public License as published by the",
"Free Software Foundation, either version 3 of the License, or (at your",
"option) any later version.",
"",
"This program is distributed in the hope that it will be useful, but",
"WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY",
"or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License",
"for more details.",
"",
"You should have received a copy of the GNU General Public License along",
"with this program. If not, see <http://www.gnu.org/licenses/>.",
"",
"The source code for LDmicro is available at",
"",
"    http://cq.cx/ladder.pl",
"",
"Copyright 2005-2010 Jonathan Westhues",
"Release 2.2, built " __TIME__ " " __DATE__ ".",
"",
"Email: user jwesthues, at host cq.cx",
"",
NULL
};

static char **Text[] = {
#if defined(LDLANG_EN) || \
    defined(LDLANG_ES) || \
    defined(LDLANG_IT) || \
    defined(LDLANG_PT)
    HelpText,
#elif defined(LDLANG_DE)
    HelpTextDe,
#elif defined(LDLANG_FR)
    HelpTextFr,
#elif defined(LDLANG_TR)
    HelpTextTr,
#else
#   error "Bad language"
#endif

    // Let's always keep the about text in English.
    AboutText
};

static HWID HelpDialog[2];
static QPlainTextEdit* RichEdit[2];

static BOOL HelpWindowOpen[2];

static int TitleHeight;

HWID PackBoxHelp;
HWID TextView;

#define RICH_EDIT_HEIGHT(h)  \
    ((((h) - 3 + (FONT_HEIGHT/2)) / FONT_HEIGHT) * FONT_HEIGHT)

static void MakeControls(int a)
{
    RichEdit[a] = new QPlainTextEdit();
    RichEdit[a]->setReadOnly(TRUE);
    QPalette pal = RichEdit[a]->palette();
    pal.setColor(QPalette::Base, (*(HBRUSH)GetStockObject(BLACK_BRUSH)));
    RichEdit[a]->setPalette(pal);

    int i;
    BOOL nextSubHead = FALSE;
    QTextCharFormat cf;
    QFont qtfont = cf.font();
    qtfont.setFamily(FixedWidthFont->lpszFace);
    qtfont.setPixelSize(FixedWidthFont->nHeight - 3);
    qtfont.setFixedPitch(TRUE);
    qtfont.setStyle(FixedWidthFont->fdwItalic ?
        QFont::StyleItalic : QFont::StyleNormal);
    qtfont.setWeight(FixedWidthFont->fnWeight == FW_BOLD ?
        QFont::Bold : QFont::Normal);
    for(i = 0; Text[a][i]; i++) {
        char *s = Text[a][i];
        cf.setFontWeight(cf.fontWeight()* 2);

        cf.setFont(qtfont);
        if((s[0] == '=') ||
           (Text[a][i+1] && Text[a][i+1][0] == '='))
        {
            COLORREF color = RGB(255, 255, 110);
            cf.setForeground(QBrush(color));
            RichEdit[a]->setCurrentCharFormat(cf);
        }
        else if(s[3] == '|' && s[4] == '|') {
            COLORREF color = RGB(255, 110, 255);
            cf.setForeground(QBrush(color));
            RichEdit[a]->setCurrentCharFormat(cf);
        }
        else if(s[0] == '>' || nextSubHead) {
            // Need to make a copy because the strings we are passed aren't
            // mutable.
            char copy[1024];
            if(strlen(s) >= sizeof(copy)) oops();
            strcpy(copy, s);

            int j;
            for(j = 1; copy[j]; j++) {
                if(copy[j] == ' ' && copy[j-1] == ' ')
                    break;
            }
            BOOL justHeading = (copy[j] == '\0');
            copy[j] = '\0';
            COLORREF color = RGB(110, 255, 110);
            cf.setForeground(QBrush(color));
            RichEdit[a]->appendPlainText(
                QString::fromStdString((const char*)copy));
            RichEdit[a]->setCurrentCharFormat(cf);
            // Special case if there's nothing except title on the line
            if(!justHeading) {
                copy[j] = ' ';
            }
            s += j;

            nextSubHead = !nextSubHead;
        }
        else {
            COLORREF color = RGB(255, 255, 255);
            cf.setForeground(QBrush(color));
            RichEdit[a]->setCurrentCharFormat(cf);
        }

        RichEdit[a]->appendPlainText(QString::fromStdString((const char*)s));

        if(Text[a][i+1]) {
            // gtk_text_buffer_insert (TextBuffer, TextIter, "\n", -1);
            // RichEdit[a]->appendPlainText("\n");
        }
    }

}

void ShowHelpDialog(BOOL about)
{
    int a = about ? 1 : 0;

    const char *s = about ? "About LDmicro" : "LDmicro Help";

    MakeControls(a);

    QVBoxLayout* PackBoxHelp = new QVBoxLayout;
    PackBoxHelp->addWidget(RichEdit[a]);
    HelpDialog[a] = new QDialog(MainWindow);
    HelpDialog[a]->resize(650, (300+10*FONT_HEIGHT));
    HelpDialog[a]->setWindowTitle(s);
    HelpDialog[a]->setLayout(PackBoxHelp);
   
    HelpDialog[a]->show();
    RichEdit[a]->verticalScrollBar()->setValue(
        RichEdit[a]->verticalScrollBar()->minimum());

    HelpWindowOpen[a] = TRUE;
}
