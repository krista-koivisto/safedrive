#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "debug.h"

enum
{
	TYPE_BOOL,
	TYPE_INT,
	TYPE_STR
};

struct SBackup
{
    int  is_recursive;
    int  do_compress;
	int  do_ignore_tildes;
	int  keep_revisions;
    char *name;
    char *dir;
    char *files;
    char *types;
} typedef SBackup;

struct SConfig
{
	int  num_backups;
	int  trust_server;
	char *passphrase;
    char filename[256];
    
    SBackup *backups[256];
} typedef SConfig;

SConfig *init_config(void);
int find_config(SConfig *conf);
void get_setting(void *conf, int type, char *category, char *setting, char *def, unsigned char *buffer);
void read_backups(SConfig *conf, unsigned char *buffer);
void parse_backup(SConfig *conf, unsigned char *buffer, size_t offset);
void parse_setting(char *category, char *setting, char *def, unsigned char *buffer, unsigned char **ret);
void get_setting_value(unsigned char *line, unsigned char **ret);
size_t find_setting(char *setting, unsigned char *buffer);
size_t find_category(char *category, unsigned char *buffer);
size_t get_line(size_t offset, unsigned char *buffer, unsigned char **ret);
int trim(unsigned char *buffer, unsigned char **ret);
void clean_config(SConfig *conf);

#endif

