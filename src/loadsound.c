#include "loadsound.h"

#include <fcntl.h>
#include <string.h>

#include "gtk/gtk.h"
#include "soundlib.h"
#include "config.h"


typedef struct {
        AudioData* audio_data;
        int sound_index;
} ButtonData;

int num_sounds = 0;
int sounds_capacity = 0;
SoundButton* sounds = NULL;
GtkWidget* main_grid = NULL;

static void* thread_start_routine(void* arg) {
        ButtonData* btn_data = (ButtonData*)arg;
        play_soundfile(&sounds[btn_data->sound_index].audio);

        return 0;
}

static void on_button_clicked(GtkWidget* widget, gpointer data) {
        ButtonData* btn_data = (ButtonData*)data;
        pthread_t thread_id;
        int pthread =
            pthread_create(&thread_id, NULL, thread_start_routine, btn_data);
        if (pthread < 0) {
                perror("pthread_create");
                return;
        }
}


void load_sound_clicked(GtkWidget* widget, gpointer data) {
        GtkWidget* dialog;
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        gint res;

        dialog = gtk_file_chooser_dialog_new(
            "Open Audio File", GTK_WINDOW(data), action, "_Cancel",
            GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "Audio Files");
        gtk_file_filter_add_pattern(filter, "*.wav");
        gtk_file_filter_add_pattern(filter, "*.mp3");
        gtk_file_filter_add_pattern(filter, "*.ogg");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

        res = gtk_dialog_run(GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_ACCEPT) {
                char* filename;
                GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
                filename = gtk_file_chooser_get_filename(chooser);

                add_sound(filename);
                create_new_config_entry(filename);

                g_free(filename);
        }

        gtk_widget_destroy(dialog);
}

char* b_strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}


void add_sound(const char* filepath) {
        if (num_sounds >= sounds_capacity) {
                sounds_capacity =
                    sounds_capacity == 0 ? 4 : sounds_capacity * 2;
                sounds = realloc(sounds, sounds_capacity * sizeof(SoundButton));
        }

        AudioData audio = load_wav(filepath);
        if (audio.samples == NULL) {
                printf("Failed to load: %s\n", filepath);
                return;
        }

        const char* slash = strrchr(filepath, '/');
        const char* display_name = slash ? slash + 1 : filepath;

        GtkWidget* button = gtk_button_new_with_label(display_name);
        gtk_widget_set_hexpand(button, TRUE);
        gtk_widget_set_vexpand(button, FALSE);
        gtk_widget_set_size_request(button, 100, 100);

        sounds[num_sounds].filename = b_strdup(filepath);
        sounds[num_sounds].display_name = b_strdup(display_name);
        sounds[num_sounds].audio = audio;
        sounds[num_sounds].button = button;
        

        int col = num_sounds % 3;
        int row = num_sounds / 3;

        ButtonData *btn_data = malloc(sizeof(ButtonData));
        btn_data->audio_data = &sounds[num_sounds].audio;
        btn_data->sound_index = num_sounds;

        g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked),
                         btn_data);
        gtk_grid_attach(GTK_GRID(main_grid), button, col, row, 1, 1);

        gtk_widget_show(button);
        num_sounds++;
}
