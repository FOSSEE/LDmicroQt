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

using namespace std;

static HWID ResetDialog;

static HWID TypeTimerRadio;
static HWID TypeCounterRadio;
static HWID NameTextbox;
static HWID OkButton;
static HWID CancelButton;

static LONG_PTR PrevNameProc;
static HWID ResetGrid;
static HWID ResetPackingBox;

//-----------------------------------------------------------------------------
// Don't allow any characters other than A-Za-z0-9_ in the name.
//-----------------------------------------------------------------------------

void ResetDialogMyNameProc (GtkEditable *editable, gchar *NewText, gint length, 
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
    TypeTimerRadio = gtk_radio_button_new_with_label (NULL, "Timer");

    TypeCounterRadio = gtk_radio_button_new_with_label_from_widget
                        (GTK_RADIO_BUTTON (TypeTimerRadio), "Counter");
    
    HWID textLabel = gtk_label_new ("Name");
    
    NameTextbox = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY (NameTextbox), 0);

    OkButton = gtk_button_new_with_label ("OK");
    CancelButton = gtk_button_new_with_label ("Cancel");

    gtk_grid_attach (GTK_GRID (ResetGrid), TypeTimerRadio, 1, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ResetGrid), TypeCounterRadio, 1, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (ResetGrid), textLabel, 2, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ResetGrid), NameTextbox, 3, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ResetGrid), OkButton, 4, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (ResetGrid), CancelButton, 4, 3, 1, 1);

    gtk_grid_set_column_spacing (GTK_GRID (ResetGrid), 1);
    gtk_box_pack_start(GTK_BOX(ResetPackingBox), ResetGrid, TRUE, TRUE, 0);

    g_signal_connect (G_OBJECT(NameTextbox), "insert-text",
		     G_CALLBACK(ResetDialogMyNameProc), NULL);
}

void ResetDialogGetData (char* name){
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (TypeTimerRadio))){
        name[0] = 'T';
    }
    else {
        name[0] = 'C';
    }
    strcpy (name+1, gtk_entry_get_text (GTK_ENTRY (NameTextbox)));
    gtk_widget_set_sensitive (MainWindow, TRUE);
    DestroyWindow (ResetDialog);
    ProgramChanged();
}

// Mouse click callback
void ResetDialogMouseClick (HWID widget, gpointer data){
    ResetDialogGetData((char*)data);
}

// Checks for the required key press
gboolean ResetDialogKeyPress (HWID widget, GdkEventKey* event, gpointer data){
    if (event -> keyval == GDK_KEY_Return){
        ResetDialogGetData((char*)data);
    }
    else if (event -> keyval == GDK_KEY_Escape){
        DestroyWindow (ResetDialog);
        ProgramChanged();
        gtk_widget_set_sensitive (MainWindow, TRUE);
    }
    return FALSE;
}

void ResetCallDestroyWindow (HWID widget, gpointer data){
    DestroyWindow (ResetDialog);
    ProgramChanged();
    gtk_widget_set_sensitive (MainWindow, TRUE);
}

void ShowResetDialog(char *name)
{
    ResetGrid = gtk_grid_new();
    ResetPackingBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    ResetDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ResetDialog), "Reset");
    gtk_window_set_default_size(GTK_WINDOW(ResetDialog), 100, 50);
    gtk_window_set_resizable (GTK_WINDOW (ResetDialog), FALSE);
    gtk_container_add(GTK_CONTAINER(ResetDialog), ResetPackingBox);
    gtk_widget_add_events (ResetDialog, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events (ResetDialog, GDK_BUTTON_PRESS_MASK);

    MakeControls();
   
    if(name[0] == 'T') {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (TypeTimerRadio), TRUE);
    }
    else {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (TypeCounterRadio), TRUE);
    }
    gtk_entry_set_text (GTK_ENTRY (NameTextbox), name+1);

    gtk_widget_set_sensitive (MainWindow, FALSE);
    gtk_widget_show_all (ResetDialog);
    gtk_widget_grab_focus (NameTextbox);
    
    g_signal_connect (G_OBJECT (ResetDialog), "key-press-event",
                    G_CALLBACK(ResetDialogKeyPress), (gpointer)name);
    g_signal_connect (G_OBJECT (OkButton), "clicked",
                    G_CALLBACK(ResetDialogMouseClick), (gpointer)name);
    g_signal_connect (G_OBJECT (CancelButton), "clicked",
                    G_CALLBACK(ResetCallDestroyWindow), NULL);
}
