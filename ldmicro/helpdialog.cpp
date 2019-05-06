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
GtkTextBuffer* TextBuffer;
GtkTextIter* TextIter = new GtkTextIter;

#define RICH_EDIT_HEIGHT(h)  \
    ((((h) - 3 + (FONT_HEIGHT/2)) / FONT_HEIGHT) * FONT_HEIGHT)

static void SizeRichEdit(int a)
{
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (RichEdit[a]),
				                          GTK_POLICY_AUTOMATIC, 
				                          GTK_POLICY_ALWAYS);
    gtk_widget_set_hexpand(GTK_WIDGET(RichEdit[a]), TRUE);  
    gtk_widget_set_vexpand(GTK_WIDGET(RichEdit[a]), TRUE);
}

// static BOOL Resizing(RECT *r, int wParam)
// {
//     BOOL touched = FALSE;
//     if(r->right - r->left < 650) {
//         int diff = 650 - (r->right - r->left);
//         if(wParam == WMSZ_RIGHT || wParam == WMSZ_TOPRIGHT ||
//             wParam == WMSZ_BOTTOMRIGHT)
//         {
//             r->right += diff;
//         } else {
//             r->left -= diff;
//         }
//         touched = TRUE;
//     }

//     if(!(wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT)) {
//         int h = r->bottom - r->top - TitleHeight - 5;
//         if(RICH_EDIT_HEIGHT(h) != h) {
//             int diff = h - RICH_EDIT_HEIGHT(h);
//             if(wParam == WMSZ_TOP || wParam == WMSZ_TOPRIGHT ||
//                 wParam == WMSZ_TOPLEFT)
//             {
//                 r->top += diff;
//             } else {
//                 r->bottom -= diff;
//             }
//             touched = TRUE;
//         }
//     }

//     return !touched;
// }

static void MakeControls(int a)
{
    // HMODULE re = LoadLibrary("RichEd20.dll");
    // if(!re) oops();

    RichEdit[a] = new QPlainTextEdit();
    RichEdit[a]->setReadOnly(TRUE);
    QPalette pal = RichEdit[a]->palette();
    pal.setColor(QPalette::Base, (*(HBRUSH)GetStockObject(BLACK_BRUSH)));
    RichEdit[a]->setPalette(pal);
    /*TextView = gtk_text_view_new ();
    TextBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (TextView));
    gtk_text_view_set_editable (GTK_TEXT_VIEW (TextView), FALSE);*/
    // SizeRichEdit(a);
    // gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (TextView), GTK_WRAP_WORD);
    // gtk_text_buffer_get_start_iter (TextBuffer, TextIter);
    // COLORREF color;
    // gtk_text_buffer_create_tag (TextBuffer, "ForegroundColor1",
    //                         "foreground", "blue");

    int i;
    BOOL nextSubHead = FALSE;
    QTextCharFormat cf;
    QFont qtfont = cf.font();
    qtfont.setFamily(FixedWidthFont->lpszFace);
    qtfont.setPixelSize(FixedWidthFont->nHeight - 3);
    qtfont.setFixedPitch(TRUE);
    qtfont.setStyle(FixedWidthFont->fdwItalic ? QFont::StyleItalic : QFont::StyleNormal);
    qtfont.setWeight(FixedWidthFont->fnWeight == FW_BOLD ? QFont::Bold : QFont::Normal);
    // hcr->setFont(qtfont);
    for(i = 0; Text[a][i]; i++) {
        char *s = Text[a][i];
        // f.setBold(TRUE);
        cf.setFontWeight(cf.fontWeight()* 2);

        cf.setFont(qtfont);
        /*RichEdit[a]->appendPlainText("Welcome");
        RichEdit[a]->setCurrentCharFormat(cf);
        RichEdit[a]->appendPlainText("Thank you");*/
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
            RichEdit[a]->appendPlainText(QString::fromStdString((const char*)copy));
            RichEdit[a]->setCurrentCharFormat(cf);
            // Special case if there's nothing except title on the line
            if(!justHeading) {
                copy[j] = ' ';
            }
            s += j;
            // color = RGB(255, 110, 255);
            // gtk_widget_override_color (TextView, GTK_STATE_FLAG_NORMAL, &color);
            nextSubHead = !nextSubHead;
        }
        else {
            COLORREF color = RGB(255, 255, 255);
            cf.setForeground(QBrush(color));
            RichEdit[a]->setCurrentCharFormat(cf);
            // gtk_widget_override_color (TextView, GTK_STATE_FLAG_NORMAL, &color);
        }

    // gtk_text_buffer_insert_with_tags_by_name (TextBuffer, TextIter,
    //                             s, -1, "ForegroundColor1", NULL);
    // gtk_text_buffer_insert (TextBuffer, TextIter, s, -1);
        RichEdit[a]->appendPlainText(QString::fromStdString((const char*)s));

        if(Text[a][i+1]) {
            // gtk_text_buffer_insert (TextBuffer, TextIter, "\n", -1);
            // RichEdit[a]->appendPlainText("\n");
        }
    }
    /*gtk_widget_override_background_color (TextView, GTK_STATE_FLAG_NORMAL,
                        ((HBRUSH)GetStockObject(BLACK_BRUSH)));
    gtk_container_add (GTK_CONTAINER(RichEdit[a]), TextView);*/

}

//-----------------------------------------------------------------------------
// Window proc for the help dialog.
//-----------------------------------------------------------------------------
// static LRESULT CALLBACK HelpProc(HWND hwnd, UINT msg, WPARAM wParam,
//     LPARAM lParam)
// {
//     int a = (hwnd == HelpDialog[0] ? 0 : 1);
//     switch (msg) {
//         case WM_SIZING: {
//             RECT *r = (RECT *)lParam;
//             return Resizing(r, wParam);
//             break;
//         }
//         case WM_SIZE:
//             SizeRichEdit(a);
//             break;

//         case WM_ACTIVATE:
//         case WM_KEYDOWN:
//             SetFocus(RichEdit[a]);
//             break;
    
//         case WM_DESTROY:
//         case WM_CLOSE:
//             HelpWindowOpen[a] = FALSE;
//             // fall through
//         default:
//             return DefWindowProc(hwnd, msg, wParam, lParam);
//     }

//     return 1;
// }

//-----------------------------------------------------------------------------
// Create the class for the help window.
//-----------------------------------------------------------------------------
static void MakeClass(void)
{
//     WNDCLASSEX wc;
//     memset(&wc, 0, sizeof(wc));
//     wc.cbSize = sizeof(wc);

//     wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
//                           CS_DBLCLKS;
//     wc.lpfnWndProc      = (WNDPROC)HelpProc;
//     wc.hInstance        = Instance;
//     wc.lpszClassName    = "LDmicroHelp";
//     wc.lpszMenuName     = NULL;
//     wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
//     wc.hIcon            = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
//                             IMAGE_ICON, 32, 32, 0);
//     wc.hIconSm          = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
//                             IMAGE_ICON, 16, 16, 0);


//     RegisterClassEx(&wc);
}

void ShowHelpDialog(BOOL about)
{
    int a = about ? 1 : 0;
    
    // MakeClass();

    const char *s = about ? "About LDmicro" : "LDmicro Help";

    MakeControls(a);

    // PackBoxHelp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    // gtk_box_pack_start(GTK_BOX(PackBoxHelp), RichEdit[a], FALSE, TRUE, 0);
    QVBoxLayout* PackBoxHelp = new QVBoxLayout;
    PackBoxHelp->addWidget(RichEdit[a]);
    HelpDialog[a] = new QDialog(MainWindow);
    HelpDialog[a]->resize(650, (300+10*FONT_HEIGHT));
    HelpDialog[a]->setWindowTitle(s);
    HelpDialog[a]->setLayout(PackBoxHelp);
   
    HelpDialog[a]->show();
    RichEdit[a]->verticalScrollBar()->setValue(
        RichEdit[a]->verticalScrollBar()->minimum());

    /*if(HelpWindowOpen[a]) {
        gtk_widget_grab_focus (HelpDialog[a]);
        return;
    }*/
    HelpWindowOpen[a] = TRUE;
}
