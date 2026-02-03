#include <gtk/gtk.h>
#include <pthread.h>

#include "config.h"
#include "loadsound.h"
#include "soundlib.h"

extern GtkWidget* main_grid;

int main(int argc, char** argv) {
        PaError err;

        err = Pa_Initialize();
        if (err != paNoError) {
                printPaError(err);
        }

        gtk_init(&argc, &argv);

        GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Soundboard");
        gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                       GTK_POLICY_AUTOMATIC,
                                       GTK_POLICY_AUTOMATIC);

        GtkWidget* grid = gtk_grid_new();
        main_grid = grid;
        gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

        gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        GtkWidget* load_btn = gtk_button_new_with_label("+ Load Sound");
        g_signal_connect(load_btn, "clicked", G_CALLBACK(load_sound_clicked),
                         window);

        GtkWidget* kill_sound_btn =
            gtk_button_new_with_label("Kill All Sounds");
        g_signal_connect(kill_sound_btn, "clicked",
                         G_CALLBACK(kill_sound_clicked), window);
        gtk_box_pack_start(GTK_BOX(vbox), load_btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), kill_sound_btn, FALSE, FALSE, 0);

        gtk_widget_set_size_request(scroll, 400, 300);
        gtk_widget_set_size_request(grid, 400, 300);

        gtk_container_add(GTK_CONTAINER(scroll), grid);
        gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);
        ;

        gtk_widget_show_all(window);
        load_config();
        gtk_main();

        err = Pa_Terminate();
        if (err != paNoError) {
                printPaError(err);
        }
        return 0;
}
