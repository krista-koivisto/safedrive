#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>

#include "debug.h"
#include "files.h"
#include "encrypt.h"

#define SDB_FILE_EXISTS -2
#define SDB_FILE_UNIQUE -1

struct sdb_file_t
{
	unsigned char checksum[20];
	unsigned char timestamp[8];
	int revision;
	char *name;
	char *dir;
} typedef sdb_file_t;

struct sdb_t
{
	int is_sorted;
	int version;
	int num_files;
	unsigned char checksum[20];
	sdb_file_t **files;
} typedef sdb_t;

int load_sdb(char *filename, sdb_t **db);
int save_sdb(char *filename, sdb_t *db);
int is_unique_sdb(sdb_t *db, char *name, char *dir, unsigned char *checksum);
void add_row_sdb(sdb_t **db, char *name, char *dir, unsigned char *checksum, int keep_revisions);
void sort_sdb(sdb_t **db);
void free_sdb(sdb_t *db);

#endif
