#ifndef FILES_H
#define FILES_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "debug.h"
#include "config.h"

struct {
	char name[256];
} typedef sd_file_t;

struct {
	char name[4096];
	int num_files;
	sd_file_t *files;
} typedef sd_dir_t;

int is_file_type(sd_file_t *types, char *file, int ignore_tildes);
int is_file_included(sd_file_t *files, char *file);
sd_file_t *parse_file_list(char *files_str);
void read_directory(char *directory, struct SBackup *backup, sd_dir_t **ret);
void free_dir(sd_dir_t *dir);
unsigned long long read_file(char *filename, unsigned char **ret);
int write_binary_file(char *filename, unsigned char *data, size_t size, int do_append);
int write_file(char *filename, char *data, int do_append);

#endif

