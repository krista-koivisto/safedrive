#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "files.h"

extern const short int DEBUG_OFF;
extern const short int DEBUG_ERROR;
extern const short int DEBUG_WARNING;
extern const short int DEBUG_INFO;
extern const short int DEBUG_DEBUG;
extern const short int DEBUG_NOTAG;

extern int DEBUG_LEVEL;
extern int LOG_LEVEL;

void init_debug();
void write_log(char *str, int level);
void debug_print(char *str, int level, char do_log, ...);
void set_levels(int log, int debug);
void set_logfile(char *filename);
char *get_logfile();
void clean_debug();

#endif

