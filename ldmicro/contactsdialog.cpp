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

static HWID ContactsDialog;

static HWID NegatedCheckbox;
static HWID SourceInternalRelayRadio;
static HWID SourceInputPinRadio;
static HWID SourceOutputPinRadio;
static HWID NameTextbox;
static HWID OkButton;
static HWID CancelButton;

static LONG_PTR PrevNameProc;
static HWID ContactsGrid;
static HWID ContactsPackingBox;
char* tmpname;
BOOL* tmpnegated;
//-----------------------------------------------------------------------------
// Don't allow any characters other than A-Za-z0-9_ in the name.
//-----------------------------------------------------------------------------
// static LRESULT CALLBACK MyNameProc(HWND hwnd, UINT msg, WPARAM wParam,
//     LPARAM lParam)
// {
//     if(msg == WM_CHAR) {
//         if(!(isalpha(wParam) || isdigit(wParam) || wParam == '_' ||
//             wParam == '\b'))
//         {
//             return 0;
//         }
//     }

//     return CallWindowProc((WNDPROC)PrevNameProc, hwnd, msg, wParam, lParam);
// }

void ContactsDialogMyNameProc (GtkEditable *editable, gchar *NewText, gint length, 
    gint *position, gpointer data){
    // gtk_widget_set_sensitive (MainWindow, TRUE);
    for (int i = 0; i < length; i++){
        if (!(isalpha (NewText[i]) || NewText[i] == '_' || isdigit (NewText[i])
                                     || NewText[i] == '\b' )){
            g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");
            return;
        }
    }
}

static void MakeControls(void)
{
    SourceInternalRelayRadio = gtk_radio_button_new_with_label (NULL, "Internal Relay");

    SourceInputPinRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (SourceInternalRelayRadio), "Input pin");
    
    SourceOutputPinRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (SourceInternalRelayRadio), "Output pin");
    
    HWID textLabel = gtk_label_new ("Name:");
    
    NameTextbox = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (NameTextbox), 0);
    
    NegatedCheckbox = gtk_check_button_new_with_label ("|/| Negated");

    OkButton = gtk_button_new_with_label ("OK");
    CancelButton = gtk_button_new_with_label ("Cancel");

    gtk_grid_attach (GTK_GRID (ContactsGrid), SourceInternalRelayRadio, 1, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), SourceInputPinRadio, 1, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), SourceOutputPinRadio, 1, 4, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), textLabel, 2, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), NegatedCheckbox, 2, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), NameTextbox, 3, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), OkButton, 4, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ContactsGrid), CancelButton, 4, 3, 1, 1);

    gtk_grid_set_column_spacing (GTK_GRID (ContactsGrid), 1);
    gtk_box_pack_start(GTK_BOX(ContactsPackingBox), ContactsGrid, TRUE, TRUE, 0);

//     PrevNameProc = SetWindowLongPtr(NameTextbox, GWLP_WNDPROC, 
//         (LONG_PTR)MyNameProc);
}

void ContactsDialogGetData (BOOL* negated, char* name){
    if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (NegatedCheckbox))) {
            *negated = TRUE;
        }
        else {
            *negated = FALSE;
        }
        if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                (SourceInternalRelayRadio))) {
            name[0] = 'R';
        }
        else if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                (SourceInputPinRadio))) {
            name[0] = 'X';
        }
        else {
            name[0] = 'Y';
        }
        strcpy (name+1, gtk_entry_get_text (GTK_ENTRY (NameTextbox)));

    DestroyWindow (ContactsDialog);
    ProgramChanged();
    gtk_widget_set_sensitive (MainWindow, TRUE);
}

// Mouse click callback
void ContactsDialogMouseClick(HWID widget, gpointer data){
    ContactsDialogGetData(tmpnegated, tmpname);
}

// Checks for the required key press
gboolean ContactsDialogKeyPress (HWID widget, GdkEventKey* event, gpointer data){
    if (event -> keyval == GDK_KEY_Return){
        ContactsDialogGetData(tmpnegated, tmpname);
    }
    else if (event -> keyval == GDK_KEY_Escape){
        DestroyWindow (ContactsDialog);
        ProgramChanged();
        gtk_widget_set_sensitive (MainWindow, TRUE);
    }
    return FALSE;
}

void ContactsCallDestroyWindow (HWID widget, gpointer data){
    DestroyWindow (ContactsDialog);
    ProgramChanged();
    gtk_widget_set_sensitive (MainWindow, TRUE);
}

void ShowContactsDialog(BOOL *negated, char *name)
{
    ContactsGrid = gtk_grid_new();
    ContactsPackingBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    ContactsDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ContactsDialog), "Contacts");
    gtk_window_set_default_size(GTK_WINDOW(ContactsDialog), 100, 50);
    gtk_window_set_resizable (GTK_WINDOW (ContactsDialog), FALSE);
    gtk_container_add(GTK_CONTAINER(ContactsDialog), ContactsPackingBox);
    gtk_widget_add_events (ContactsDialog, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events (ContactsDialog, GDK_BUTTON_PRESS_MASK);

    MakeControls();
   
    if(name[0] == 'R') {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SourceInternalRelayRadio), TRUE);
    }
    else if(name[0] == 'Y') {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SourceOutputPinRadio), TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SourceInputPinRadio), TRUE);
    }
    if(*negated) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (NegatedCheckbox), TRUE);
    }
    gtk_entry_set_text (GTK_ENTRY (NameTextbox), name + 1);

    gtk_widget_set_sensitive (MainWindow, FALSE);
    gtk_widget_show_all (ContactsDialog);
    gtk_widget_grab_focus (NameTextbox);
    tmpname = name;
    tmpnegated = negated;

    g_signal_connect (G_OBJECT(NameTextbox), "insert-text",
		     G_CALLBACK(ContactsDialogMyNameProc), NULL);
    g_signal_connect (G_OBJECT (ContactsDialog), "key-press-event",
                    G_CALLBACK(ContactsDialogKeyPress), NULL);
    g_signal_connect (G_OBJECT (OkButton), "clicked",
                    G_CALLBACK(ContactsDialogMouseClick), NULL);
    g_signal_connect (G_OBJECT (CancelButton), "clicked",
                    G_CALLBACK(ContactsCallDestroyWindow), NULL);

}
