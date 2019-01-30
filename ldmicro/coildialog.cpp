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

static HWID CoilDialog;

static HWID SourceInternalRelayRadio;
static HWID SourceMcuPinRadio;
static HWID NegatedRadio;
static HWID NormalRadio;
static HWID SetOnlyRadio;
static HWID ResetOnlyRadio;
static HWID NameTextbox;
static HWID OkButton;
static HWID CancelButton;

static LONG_PTR PrevNameProc;

static HWID CoilGrid;
static HWID CoilPackingBox;
static bool* tmpnegated;
static bool* tmpsetOnly ;
static bool* tmpresetOnly;

//-----------------------------------------------------------------------------
// Don't allow any characters other than A-Za-z0-9_ in the name.
//-----------------------------------------------------------------------------

void CoilDialogMyNameProc (GtkEditable *editable, gchar *NewText, gint length, 
    gint *position, gpointer data){
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
    NormalRadio = gtk_radio_button_new_with_label (NULL, "( ) Normal");
    NegatedRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (NormalRadio), "(/) Negated");
    SetOnlyRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (NormalRadio), "(S) Set-Only");
    ResetOnlyRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (NormalRadio), "(R) Reset-Only");
    
    SourceInternalRelayRadio = gtk_radio_button_new_with_label (NULL, "Internal Relay");
    SourceMcuPinRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (SourceInternalRelayRadio), "Pin on MCU");
    
    HWID textLabel = gtk_label_new ("Name:");
    
    NameTextbox = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (NameTextbox), 0);

    OkButton = gtk_button_new_with_label ("OK");
    CancelButton = gtk_button_new_with_label ("Cancel");

    gtk_grid_attach (GTK_GRID (CoilGrid), NormalRadio, 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), NegatedRadio, 0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), SetOnlyRadio, 0, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), ResetOnlyRadio, 0, 4, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), SourceInternalRelayRadio, 1, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), SourceMcuPinRadio, 1, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), textLabel, 1, 4, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), NameTextbox, 2, 4, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), OkButton, 4, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (CoilGrid), CancelButton, 4, 3, 1, 1);

    gtk_grid_set_column_spacing (GTK_GRID (CoilGrid), 1);
    gtk_box_pack_start(GTK_BOX(CoilPackingBox), CoilGrid, TRUE, TRUE, 0);

    g_signal_connect (G_OBJECT (NameTextbox), "insert-text",
                    G_CALLBACK (CoilDialogMyNameProc), NULL);
}

void CoilDialogGetData (char* name){

    bool* negated = tmpnegated;
    bool* resetOnly = tmpresetOnly;
    bool* setOnly = tmpsetOnly;

    if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
    (SourceInternalRelayRadio))) {
            name[0] = 'R';
        }
        else {
            name[0] = 'Y';
        }
        strcpy (name+1, gtk_entry_get_text (GTK_ENTRY (NameTextbox)));

        if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
        (NormalRadio))) {
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
        }
        else if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
        (NegatedRadio))) {
            *negated = TRUE;
            *setOnly = FALSE;
            *resetOnly = FALSE;
        }
        else if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
        (SetOnlyRadio))) {
            *negated = FALSE;
            *setOnly = TRUE;
            *resetOnly = FALSE;
        }
        else if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
        (ResetOnlyRadio))){
            *negated = FALSE;
            *setOnly = FALSE;
            *resetOnly = TRUE;
        }

    gtk_widget_set_sensitive (MainWindow, TRUE);
    DestroyWindow (CoilDialog);
    ProgramChanged();
}

// Mouse click callback
void CoilDialogMouseClick (HWID widget, gpointer data){
    CoilDialogGetData((char*)data);
}

// Checks for the required key press
gboolean CoilDialogKeyPress (HWID widget, GdkEventKey* event, gpointer data){
    if (event -> keyval == GDK_KEY_Return){
        CoilDialogGetData((char*)data);
    }
    else if (event -> keyval == GDK_KEY_Escape){
        DestroyWindow (CoilDialog);
        ProgramChanged();
        gtk_widget_set_sensitive (MainWindow, TRUE);
    }
    return FALSE;
}

// Calls DestroyWindow
void CoilCallDestroyWindow (HWID widget, gpointer data){
    DestroyWindow (CoilDialog);
    ProgramChanged();
    gtk_widget_set_sensitive (MainWindow, TRUE);
}

void ShowCoilDialog(BOOL *negated, BOOL *setOnly, BOOL *resetOnly, char *name)
{
    CoilGrid = gtk_grid_new();
    CoilPackingBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    CoilDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(CoilDialog), "Coil");
    gtk_window_set_default_size(GTK_WINDOW(CoilDialog), 100, 50);
    gtk_window_set_resizable (GTK_WINDOW (CoilDialog), FALSE);
    gtk_container_add(GTK_CONTAINER(CoilDialog), CoilPackingBox);
    gtk_widget_add_events (CoilDialog, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events (CoilDialog, GDK_BUTTON_PRESS_MASK);

    MakeControls();
   
    if(name[0] == 'R') {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SourceInternalRelayRadio), TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SourceMcuPinRadio), TRUE);
    }
    gtk_entry_set_text (GTK_ENTRY (NameTextbox), name+1);
    if(*negated) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (NegatedRadio), TRUE);
    }
    else if(*setOnly) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (SetOnlyRadio), TRUE);
    }
    else if(*resetOnly) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ResetOnlyRadio), TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (NormalRadio), TRUE);
    }

    gtk_widget_set_sensitive (MainWindow, FALSE);
    gtk_widget_show_all (CoilDialog);
    gtk_widget_grab_focus (NameTextbox);

    tmpnegated = negated;
    tmpresetOnly = resetOnly;
    tmpsetOnly = setOnly;

    g_signal_connect (G_OBJECT (CoilDialog), "key-press-event",
                    G_CALLBACK(CoilDialogKeyPress), (gpointer)name);
    g_signal_connect (G_OBJECT (OkButton), "clicked",
                    G_CALLBACK(CoilDialogMouseClick), (gpointer)name);
    g_signal_connect (G_OBJECT (CancelButton), "clicked",
                    G_CALLBACK(CoilCallDestroyWindow), NULL);

    return;
}
