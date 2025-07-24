#include "file.h"

#define CONFIG_FILE "config.txt"
#define MAX_LINE 256

// 🔹 Write (or update) a string key
void write_config_str(const char *key, const char *value) {
    FILE *src = fopen(CONFIG_FILE, "r");
    FILE *tmp = fopen("temp_config.txt", "w");
    char line[MAX_LINE];
    int updated = 0;

    if (src) {
        while (fgets(line, sizeof(line), src)) {
            char *eq = strchr(line, '=');
            if (!eq) {
                fputs(line, tmp);
                continue;
            }

            *eq = '\0';
            char *file_key = line;
            char *file_val = eq + 1;

            // Remove newline from value
            file_val[strcspn(file_val, "\r\n")] = '\0';

            if (strcmp(file_key, key) == 0) {
                fprintf(tmp, "%s=%s\n", key, value);
                updated = 1;
            } else {
                fprintf(tmp, "%s=%s\n", file_key, file_val);
            }
        }
        fclose(src);
    }

    if (!updated) {
        fprintf(tmp, "%s=%s\n", key, value);  // append new key
    }

    fclose(tmp);
    remove(CONFIG_FILE);
    rename("temp_config.txt", CONFIG_FILE);
}

// 🔹 Write (or update) an int key
void write_config_int(const char *key, int value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", value);
    write_config_str(key, buf);
}

// 🔹 Read string value by key
// ⚠️ Caller must free() the returned string
char *read_config_str(const char *key) {
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) return NULL;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *file_key = line;
        char *file_val = eq + 1;

        file_val[strcspn(file_val, "\r\n")] = '\0';

        if (strcmp(file_key, key) == 0) {
            fclose(f);
            return strdup(file_val);
        }
    }

    fclose(f);
    return NULL;
}

// 🔹 Read int value by key, with default fallback
int read_config_int(const char *key, int default_value) {
    char *val = read_config_str(key);
    if (val) {
        int result = atoi(val);
        free(val);
        return result;
    }
    return default_value;
}