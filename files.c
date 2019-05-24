#include "files.h"

int is_file_type(sd_file_t *types, char *file, int ignore_tilde)
{
	if (ignore_tilde && file[strlen(file) - 1] == '~')
		return 0;
	
	char *type = strrchr(file, '.');
	
	// If no type was specified, accept only files without extensions
	if (!types && !type)
		return 1;
	else if (!types)
		return 0;
	
	// Wild cards go wild!!!!!
	if (types[0].name[0] == '*')
		return 1;
	
	if (type)
	{
		// Skip the dot
		*type++;
		
		for (int i = 0; types[i].name[0] != -1; i++)
			if (!strcmp(types[i].name, type)) return 1;
	}
	else
	{
		// Files without type extension accepted?
		for (int i = 0; types[i].name[0] != -1; i++)
			if (types[i].name[0] == 0) return 1;
	}
	
	return 0;
}

int is_file_included(sd_file_t *files, char *file)
{
	if (!files) return 0;
	
	for (int i = 0; files[i].name[0] != -1; i++)
			if (!strcmp(files[i].name, file)) return 1;
	
	return 0;
}

sd_file_t *parse_file_list(char *files_str)
{
	int file_count = 0;
	char *str = files_str;
	for (file_count = 1; str[file_count]; (str[file_count] == ',') ? file_count++ : *str++);

	str = files_str;
	sd_file_t *ret = malloc(sizeof(sd_file_t) * file_count + 1);
	
	char *t = strtok(str, ",");
	
	int c = 0;
	int i = 0;
	int j = 0;
	int len = 0;
	
	while (t != NULL)
	{
		len = strlen(t);
		
		// Trim
		while(t[i] == 10 || t[i] == ' ')
			i++;
			
		while(t[len - j - 1] == 10 || t[len - j - 1] == ' ')
			j++;
			
		t[len - j] = 0;
		
		snprintf(ret[c].name, 256, "%s", &t[i]);
		t = strtok(NULL, ",");
		c++;
		i = 0;
		j = 0;
	}
	
	if (c < file_count)
	{
		// Also accept files without any type
		ret[c].name[0] = 0;
		c++;
	}
	
	ret[c].name[0] = -1;
	
	return ret;
}

void read_directory(char *directory, SBackup *backup, sd_dir_t **ret)
{
	DIR *dp;
	struct dirent *dir;
	int num_files = 0;
	sd_file_t *also = 0;
	sd_file_t *extensions = 0;
	*ret = 0;
	
	if (strlen(backup->files))
		also = parse_file_list(backup->files);
	
	if (strlen(backup->types))
		extensions = parse_file_list(backup->types);
	
	if (strlen(directory) > 4094)
	{
		debug_print("Directory names are limited to 4096 characters, sorry!", DEBUG_ERROR, 1);
	}
	else
	{
		dp = opendir(directory);

		if (dp)
		{
			// Count the number files in the directory
			while ((dir = readdir(dp)) != NULL)
			{
				if (dir->d_type == DT_REG && (is_file_type(extensions, dir->d_name, backup->do_ignore_tildes) || is_file_included(also, dir->d_name)))
				{
					num_files++;
				}
			}
			
			rewinddir(dp);
			
			*ret = malloc(sizeof(sd_dir_t));
			(*ret)->files = malloc(sizeof(sd_file_t) * num_files);
			snprintf((*ret)->name, 4096, "%s", directory);
			(*ret)->num_files = num_files;
			
			int i = 0;
			
			// Get the actual list of files
			while ((dir = readdir(dp)) != NULL)
			{
				if (dir->d_type == DT_REG && (is_file_type(extensions, dir->d_name, backup->do_ignore_tildes) || is_file_included(also, dir->d_name)))
				{
					snprintf((*ret)->files[i].name, 256, "%s", dir->d_name);
					++i;
				}
			}

			closedir(dp);
			
			return;
		}
		else
		{
			debug_print("I am unable to read the directory '%s'!", DEBUG_ERROR, 1, directory);
		}
	}
	
	*ret = malloc(sizeof(sd_dir_t));
	(*ret)->num_files = 0;
	(*ret)->files = 0;
	return;
}

void free_dir(sd_dir_t *dir)
{
	free(dir->files);
	free(dir);
}

unsigned long long read_file(char *filename, unsigned char **ret)
{
	if(access(filename, R_OK) != 0)
	{
		debug_print("I am unable to read '%s'! :( Does it exist?", DEBUG_ERROR, 1, filename);
		return 0;
	}

	size_t bytes_read = 0;
	FILE *fp;
	fp = fopen(filename, "r");
	
	if(fp != NULL)
	{
		int fsize = 0;
		
		fseek(fp, 0, SEEK_END);
		fsize = ftell(fp);
		rewind(fp);
		
		*ret = malloc(sizeof(char) * fsize + 1);
		
		bytes_read = fread(*ret, sizeof(char), fsize, fp);
		
		if (bytes_read == 0)
		{
			debug_print("Failed to read '%s'.", DEBUG_ERROR, 1, filename);
			
			free(*ret);
			*ret = NULL;
			return 0;
		}
		else if (bytes_read != fsize)
		{
			debug_print("It seems I failed to read the entire contents of '%s'. Continuing anyway...", DEBUG_ERROR, 1, filename);
		}
	}
	else
	{
		debug_print("Failed to open '%s' for reading!", DEBUG_ERROR, 1, filename);
		return 0;
	}
	
	return bytes_read;
}

int write_binary_file(char *filename, unsigned char *data, size_t size, int do_append)
{
	if(access(filename, F_OK) != -1 && access(filename, W_OK) != 0)
	{
		debug_print("I am not allowed to write to '%s'! :( Check file and folder permissions?", DEBUG_ERROR, 1, filename);
		return 0;
	}
	
	FILE *fp;
	
	char mode[3] = {'w', 'b', 0};
	if (do_append) mode[0] = 'a';

	fp = fopen(filename, mode);
	
	if(fp != NULL)
	{
		fwrite(data, sizeof(unsigned char), size, fp);
		fclose(fp);
	}
	else
	{
		debug_print("Failed to open '%s' for writing!", DEBUG_ERROR, 1, filename);
		return 0;
	}
	
	return 1;
}

int write_file(char *filename, char *data, int do_append)
{
	if(access(filename, F_OK) != -1 && access(filename, W_OK) != 0)
	{
		debug_print("I am not allowed to write to '%s'! :( Check file and folder permissions?", DEBUG_ERROR, 1, filename);
		return 0;
	}
	
	char mode[2] = {'w', 0};
	if (do_append) mode[0] = 'a';
	
	FILE *fp;
	fp = fopen(filename, mode);
	
	if(fp != NULL)
	{
		fprintf(fp, "%s", data);
		fclose(fp);
	}
	else
	{
		debug_print("Failed to open '%s' for writing!", DEBUG_ERROR, 1, filename);
		return 0;
	}
	
	return 1;
}

