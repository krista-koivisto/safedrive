#include "database.h"

const unsigned char SDB_MAGIC[5] = {0x53, 0x44, 0x42, 0x3A, 0x07};
const unsigned char SDB_RESERVED[2] = {0x00, 0x00};
const int SDB_NAME_OFFSET = 32; // 20 bytes for SHA1 checksum, 8 for Unix Timestamp, 4 for revision
const int SDB_ROW_MAXSIZE = 4096 + 256 + 8 + 8 + 20 + 1 + 1; // Dir, name, revision, timestamp, checksum, / and \0

int load_sdb(char *filename, sdb_t **db)
{
	*db = malloc(sizeof(sdb_t));
	
	unsigned char *data = 0;
	unsigned long long size = read_file(filename, &data);
	
	if (size > 7 && !memcmp(SDB_MAGIC, data, 5))
	{
		(*db)->is_sorted = 0;
		(*db)->checksum[0] = 0;
		(*db)->version = data[7];
		sha1(data, size, (*db)->checksum, 1);
		
		// Count number of files in the DB
		(*db)->num_files = 0;
		unsigned char *ptr = &data[SDB_NAME_OFFSET + 8];
		
		while (ptr <= &data[size])
		{
			(*db)->num_files++;
			ptr = memchr(ptr, '\0', SDB_ROW_MAXSIZE);
			ptr += SDB_NAME_OFFSET + 1;
		}
		
		(*db)->files = malloc(sizeof(sdb_file_t*) * (*db)->num_files);
		
		// Load file data
		ptr = &data[8];
		int len = 0;
		
		for (int i = 0; i < (*db)->num_files; i++)
		{
			(*db)->files[i] = malloc(sizeof(sdb_file_t));
			memcpy((*db)->files[i]->checksum, ptr, 20);
			memcpy((*db)->files[i]->timestamp, &ptr[20], 8);
			memcpy(&(*db)->files[i]->revision, &ptr[28], 4);
			
			len = strcspn((char*)&ptr[SDB_NAME_OFFSET], "/");
			(*db)->files[i]->name = malloc(sizeof(char) * len + 2);
			snprintf((*db)->files[i]->name, len + 1, "%s", &ptr[SDB_NAME_OFFSET]);
			
			ptr = &ptr[SDB_NAME_OFFSET + len];
			
			len = strcspn((char*)++ptr, "\0");
			(*db)->files[i]->dir = malloc(sizeof(char) * len + 2);
			snprintf((*db)->files[i]->dir, len + 1, "%s", ptr);
			
			ptr+= len + 1;
		}

		printf("Loaded %d files from DB...\n", (*db)->num_files);
	}
	else
	{
		debug_print("Failed to read database '%s'.", DEBUG_ERROR, 1, filename);
	}
	
	free(data);
	
	return 0;
}

int save_sdb(char *filename, sdb_t *db)
{
	unsigned char data[3] = {SDB_RESERVED[0], SDB_RESERVED[1], db->version};
	write_binary_file(filename, (unsigned char*)SDB_MAGIC, 5, 0);
	write_binary_file(filename, data, 3, 1);
	
	unsigned char *blob = malloc(SDB_ROW_MAXSIZE);
	
	for (int i = 0; i < db->num_files; i++)
	{
		int offset = 0;
		int name_len = strlen(db->files[i]->name);
		int dir_len = strlen(db->files[i]->dir);
		int size = SDB_NAME_OFFSET + name_len + dir_len + 2;
		
		memcpy(blob, db->files[i]->checksum, 20);
		memcpy(&blob[20], db->files[i]->timestamp, 8);
		memcpy(&blob[28], &db->files[i]->revision, 4);
		memcpy(&blob[SDB_NAME_OFFSET], db->files[i]->name, name_len);
		offset = SDB_NAME_OFFSET + name_len;
		blob[offset] = '/';
		memcpy(&blob[offset + 1], db->files[i]->dir, dir_len);
		blob[offset + dir_len + 1] = '\0';
		
		write_binary_file(filename, blob, size, 1);
	}
	
	free(blob);
	
	return 0;
}

int is_unique_sdb(sdb_t *db, char *name, char *dir, unsigned char *checksum)
{
	// @TODO: We can perform a much more efficient search if the DB has been sorted
	for (int i = 0; i < db->num_files; i++)
	{
		if (!strcmp(name, db->files[i]->name) && !strcmp(dir, db->files[i]->dir))
		{
			if (!strcmp((char*)checksum, (char*)db->files[i]->checksum))
			{
				return SDB_FILE_EXISTS;
			}
			else
			{
				return i;
			}
		}
	}
	
	return SDB_FILE_UNIQUE;
}

void _sort_sdb_swap(sdb_file_t **file1, sdb_file_t **file2)
{
    sdb_file_t *tmp_file = *file1;
    *file1 = *file2;
    *file2 = tmp_file;
}

void _sort_sdb(sdb_file_t **files, unsigned int len)
{
	unsigned int pivot = 0;

    if (len <= 1)
	{
        return;
	}

	srand((unsigned)time(NULL));
    _sort_sdb_swap(files + ((unsigned int)rand() % len), files + len - 1);

    for (unsigned int i = 0; i < len - 1; ++i)
    {
        if (strcmp(files[i]->name, files[len - 1]->name) < 0)
            _sort_sdb_swap(files + i, files + pivot++);
    }

    _sort_sdb_swap(files + pivot, files + len - 1);
    _sort_sdb(files, pivot++);
    _sort_sdb(files + pivot, len - pivot);
}

void sort_sdb(sdb_t **db)
{
	_sort_sdb((*db)->files, (*db)->num_files);
	(*db)->is_sorted = 1;
}

void add_row_sdb(sdb_t **db, char *name, char *dir, unsigned char *checksum, int keep_revisions)
{
	int file_index = is_unique_sdb(*db, name, dir, checksum);
	
	if (file_index == SDB_FILE_UNIQUE || (file_index >= 0 && keep_revisions))
	{
		sdb_file_t *f = malloc(sizeof(sdb_file_t));
		f->name = malloc(sizeof(char) * strlen(name) + 1);
		f->dir = malloc(sizeof(char) * strlen(dir) + 1);
		snprintf(f->name, strlen(name) + 1, "%s", name);
		snprintf(f->dir, strlen(dir) + 1, "%s", dir);
		memset(f->timestamp, 0, 8);
		memcpy(f->checksum, checksum, 20);
		
		if (file_index >= 0)
		{
			f->revision = (*db)->files[file_index]->revision + 1;
			debug_print("The file has been modified, adding revision!", DEBUG_INFO, 0);
		}
		else
		{
			f->revision = 1;
		}
		
		(*db)->num_files++;
		(*db)->files = realloc((*db)->files, sizeof(sdb_file_t*) * (*db)->num_files);
		(*db)->files[(*db)->num_files - 1] = f;
		(*db)->is_sorted = 0;
	}
	else if (file_index >= 0 && !keep_revisions)
	{
		debug_print("The file has been modified, updating!", DEBUG_INFO, 0);
		memcpy((*db)->files[file_index]->checksum, checksum, 20);
	}
}

void free_sdb(sdb_t *db)
{
	for (int i = 0; i < db->num_files; i++)
	{
		free(db->files[i]->name);
		free(db->files[i]->dir);
		free(db->files[i]);
	}
	
	free(db->files);
	free(db);
}
