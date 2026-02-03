#ifndef SOUNDLIB_H
#define SOUNDLIB_H
#include <portaudio.h>
#include <sndfile.h>
#include <gtk/gtk.h>

typedef struct {
        float* samples;
        long total_frames;
        long current_frame;
        int sample_rate;
} AudioData;
extern PaStream** active_streams;
extern int num_active_streams;
extern int active_streams_capacity;
void play_soundfile(AudioData* data);
void register_stream(PaStream* stream);
void unregister_stream(PaStream* stream);
void kill_all_sounds();
void kill_sound_clicked(GtkWidget* widget, gpointer data);
AudioData load_wav(const char* path);
void printPaError(PaError err);

#endif
