#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "loadsound.h"

static const char* config_file = "/home/bmtron/.config/soundboard/sounds.conf";

void load_config() {
        FILE* f = fopen(config_file, "r");
        if (!f) {
                printf("No existing config file found\n");
                perror("fopen");
                return;
        }

        char line[1024];
        while (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = '\0';
                if (strlen(line) == 0) continue;
                add_sound(line);
        }
        fclose(f);
}

void create_new_config_entry(const char* filepath) {
        printf("Creating new config entry...\n");
        FILE* f = fopen(config_file, "a+");
        if (!f) {
                printf(
                    "Error creating new config entry: No existing config file "
                    "found\n");
                perror("fopen");
                return;
        }

        char line[1024];
        while (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = '\0';
                if (strcmp(filepath, line) == 0) {
                        fclose(f);
                        printf("Soundfile already exists in config.\n");
                        return;
                }
        }
        fprintf(f, "%s\n", filepath);
        fclose(f);
}
