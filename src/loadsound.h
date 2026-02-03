#ifndef LOADSOUND_H
#define LOADSOUND_H
#include <gtk/gtk.h>

#include "soundlib.h"

typedef struct {
        char* filename;
        char* display_name;
        AudioData audio;
        GtkWidget* button;
} SoundButton;

extern SoundButton* sounds;
extern int num_sounds;
extern int sounds_capacity;
extern GtkWidget* main_grid;

void load_sound_clicked(GtkWidget* widget, gpointer data);
void add_sound(const char* filepath);


#endif
