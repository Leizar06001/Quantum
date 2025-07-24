#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_config_str(const char *key, const char *value);
void write_config_int(const char *key, int value);
char *read_config_str(const char *key);
int read_config_int(const char *key, int default_value);

#endif