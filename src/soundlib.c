#include "soundlib.h"

#include <portaudio.h>
#include <pthread.h>
#include <samplerate.h>

int num_active_streams = 0;
int active_streams_capacity = 0;
PaStream** active_streams = NULL;
float master_volume = 1.0f;

AudioData load_wav(const char* filepath) {
        AudioData data = {NULL, 0, 0, 0};
        SF_INFO info = {0};

        SNDFILE* file = sf_open(filepath, SFM_READ, &info);
        if (!file) {
                printf("sf_open error: %s\n", sf_strerror(file));
                return data;
        }

        float* stereo_samples =
            malloc(info.frames * info.channels * sizeof(float));
        sf_readf_float(file, stereo_samples, info.frames);
        sf_close(file);

        float* mono_samples = malloc(info.frames * sizeof(float));
        if (info.channels == 2) {
                for (long i = 0; i < info.frames; i++) {
                        mono_samples[i] = (stereo_samples[i * 2] +
                                           stereo_samples[i * 2 + 1] * 0.5f);
                }
        } else {
                memcpy(mono_samples, stereo_samples,
                       info.frames * sizeof(float));
        }
        free(stereo_samples);

        // TODO: this should be configurable /td
        int target_rate = 48000;  // sample rate of headset mic/soundboard sink
        if (info.samplerate != target_rate) {
                double ratio = (double)target_rate / (double)info.samplerate;
                long output_frames = (long)(info.frames * ratio);
                float* resampled = malloc(output_frames * sizeof(float));

                SRC_DATA src_data;
                src_data.data_in = mono_samples;
                src_data.data_out = resampled;
                src_data.input_frames = info.frames;
                src_data.output_frames = output_frames;
                src_data.src_ratio = ratio;

                int error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1);
                if (error) {
                        printf("Resample error: %s\n", src_strerror(error));
                        free(mono_samples);
                        free(resampled);
                        return data;
                }
                free(mono_samples);
                data.samples = resampled;
                data.total_frames = src_data.output_frames_gen;
                data.sample_rate = target_rate;
        } else {
                data.samples = mono_samples;
                data.total_frames = info.frames;
                data.sample_rate = info.samplerate;
        }

        printf("About to return: rate=%d, frames=%ld, samples=%p\n",
               data.sample_rate, data.total_frames, data.samples);
        return data;
}

void printPaError(PaError err) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
}

static int paCallback(const void* inputBuffer, void* outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags, void* userData) {
        AudioData* data = (AudioData*)userData;
        static int first_call = 1;
        float* out = (float*)outputBuffer;
        unsigned int i;
        (void)inputBuffer;
        long remaining = data->total_frames - data->current_frame;

        if (remaining <= 0) return paComplete;

        long to_copy =
            framesPerBuffer < remaining ? framesPerBuffer : remaining;
        for (long i = 0; i < to_copy; i++) {
                out[i] = data->samples[data->current_frame + i] * master_volume;
        }
        data->current_frame += to_copy;

        for (long i = to_copy; i < framesPerBuffer; i++) {
                out[i] = 0.0f;
        }

        return paContinue;
}

int soundboard_sink_setup() {
        int soundboard_device = -1;
        for (int i = 0; i < Pa_GetDeviceCount(); i++) {
                const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
                if (info->maxOutputChannels > 0) {
                        //                        printf(" %d: %s (channels:
                        //                        %d)\n", i, info->name,
                        //                              info->maxOutputChannels);
                        if (strstr(info->name, "Soundboard")) {
                                soundboard_device = i;
                                printf("    ^ Found soundboard sink!\n");
                                return soundboard_device;
                        }
                }
        }

        if (soundboard_device == -1) {
                printf("ERROR: soundboard_sink not found!\n");
                printf("Make sure to run the bash script.\n");
                Pa_Terminate();
                return -1;
        }
        return -1;
}

void play_soundfile(AudioData* data) {
        PaStream* stream;
        PaError err;

        AudioData* local_data = malloc(sizeof(AudioData));
        local_data->samples =
            data->samples;  // Share the sample buffer (read-only)
        local_data->total_frames = data->total_frames;
        local_data->sample_rate = data->sample_rate;
        local_data->current_frame = 0;

        AudioData* user_local_data = malloc(sizeof(AudioData));
        user_local_data->samples = data->samples;
        user_local_data->total_frames = data->total_frames;
        user_local_data->sample_rate = data->sample_rate;
        user_local_data->current_frame = 0;

        int soundboard_device = soundboard_sink_setup();
        if (soundboard_device == -1) {
                printf("Error setting up soundboard sink.\n");
                return;
        }
        PaStreamParameters outputParams;
        outputParams.device = soundboard_device;
        outputParams.channelCount = 1;
        outputParams.sampleFormat = paFloat32;
        outputParams.suggestedLatency =
            Pa_GetDeviceInfo(soundboard_device)->defaultLowOutputLatency;
        outputParams.hostApiSpecificStreamInfo = NULL;

        err =
            Pa_OpenStream(&stream, NULL, &outputParams, local_data->sample_rate,
                          2048, paNoFlag, paCallback, local_data);
        register_stream(stream);

        if (err != paNoError) {
                printPaError(err);
        }
        PaStream* user_stream;
        PaStreamParameters user_out_params;
        user_out_params.device = Pa_GetDefaultOutputDevice();
        user_out_params.channelCount = 1;
        user_out_params.sampleFormat = paFloat32;
        user_out_params.suggestedLatency =
            Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())
                ->defaultLowOutputLatency;
        user_out_params.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(&user_stream, NULL, &user_out_params,
                            user_local_data->sample_rate, 2048, paNoFlag, paCallback,
                            user_local_data);
        if (err != paNoError) {
            printPaError(err);
        }

        register_stream(user_stream);

        err = Pa_StartStream(stream);
        if (err != paNoError) {
                printPaError(err);
        }
        err = Pa_StartStream(user_stream);
        if (err != paNoError) {
            printPaError(err);
        }

        long sleep_time =
            local_data->total_frames / (long)local_data->sample_rate;

        Pa_Sleep(sleep_time * 1000);

        if (Pa_IsStreamActive(stream)) {
                err = Pa_StopStream(stream);
                if (err != paNoError) {
                        printPaError(err);
                }
        }

        if (Pa_IsStreamActive(user_stream)) {
            err = Pa_StopStream(user_stream);
            if (err != paNoError) {
                printPaError(err);
            }
        }

        err = Pa_CloseStream(stream);
        if (err != paNoError) {
                printPaError(err);
        }
        err = Pa_CloseStream(user_stream);
        if (err != paNoError) {
            printPaError(err);
        }

        unregister_stream(stream);
        unregister_stream(user_stream);
}

pthread_mutex_t streams_mutex = PTHREAD_MUTEX_INITIALIZER;

void register_stream(PaStream* stream) {
        pthread_mutex_lock(&streams_mutex);
        if (num_active_streams >= active_streams_capacity) {
                active_streams_capacity = active_streams_capacity == 0
                                              ? 4
                                              : active_streams_capacity * 2;
                active_streams =
                    realloc(active_streams,
                            active_streams_capacity * sizeof(PaStream*));
        }
        active_streams[num_active_streams++] = stream;
        pthread_mutex_unlock(&streams_mutex);
}

void unregister_stream(PaStream* stream) {
        pthread_mutex_lock(&streams_mutex);
        for (int i = 0; i < num_active_streams; i++) {
                if (active_streams[i] == stream) {
                        memmove(
                            &active_streams[i], &active_streams[i + 1],
                            (num_active_streams - i - 1) * sizeof(PaStream*));
                        num_active_streams--;
                        pthread_mutex_unlock(&streams_mutex);
                        return;
                }
        }
        pthread_mutex_unlock(&streams_mutex);
}


void kill_all_sounds() {
        for (int i = 0; i < num_active_streams; i++) {
                Pa_StopStream(active_streams[i]);
                Pa_CloseStream(active_streams[i]);
        }
        num_active_streams = 0;
}

void on_volume_changed(GtkRange *range, gpointer data)  {
    master_volume = gtk_range_get_value(range);
    printf("Volume changed to: %.2f\n", master_volume);
}
void kill_sound_clicked(GtkWidget* widget, gpointer data) { kill_all_sounds(); }

