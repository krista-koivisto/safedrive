#include "config.h"

int find_config(SConfig *conf)
{
    if(access("safedrive.conf", R_OK) != -1)
    {
    	strncpy(conf->filename, "safedrive.conf", strlen("safedrive.conf"));
    }
    else if(access("/etc/safedrive/safedrive.conf", R_OK) != -1)
    {
    	strncpy(conf->filename, "/etc/safedrive/safedrive.conf", strlen("/etc/safedrive/safedrive.conf"));
    }
    else
    {
        return 0;
    }
    
    return 1;
}

SConfig *init_config()
{
	debug_print("Initializing config...", DEBUG_DEBUG, 0);
	SConfig *conf = malloc(sizeof(SConfig));
    
    if (conf != 0 && find_config(conf))
    {
    	unsigned char *buffer;
    	
		conf->passphrase = NULL;
		conf->trust_server = 0;
    	conf->num_backups = 0;
    	
    	if (read_file(conf->filename, &buffer))
    	{
    		debug_print("Reading config '%s'...", DEBUG_INFO, 0, conf->filename);
    		
    		char *logfile = 0;
    		get_setting(&logfile, TYPE_STR, "Log", "LogFile", "safedrive.log", buffer);
    		set_logfile(logfile);
    		free(logfile);
    		
    		get_setting((void *)&LOG_LEVEL, TYPE_INT, "Log", "Verbosity", "4", buffer);
   			get_setting((void *)&DEBUG_LEVEL, TYPE_INT, "Settings", "Verbosity", "2", buffer);
	    	debug_print("Logger initiated.", DEBUG_DEBUG, 1);
			
			get_setting(&conf->trust_server, TYPE_BOOL, "Settings", "TrustServer", "true", buffer);
			get_setting(&conf->passphrase, TYPE_STR, "Settings", "Passphrase", "", buffer);
			
			if (strlen(conf->passphrase) < 10)
			{
				free(buffer);
				debug_print("Your passphrase is too short! The passphrase has to be at least 10 characters long!", DEBUG_ERROR, 1);
				return NULL;
			}
	    	
	    	read_backups(conf, buffer);
    	}
    	
    	free(buffer);
    }
    else
    {
    	debug_print("Failed to find config file!", DEBUG_ERROR, 0);
    	return NULL;
    }
    
    debug_print("Configuration initiated.", DEBUG_DEBUG, 1);
    
    return conf;
}

void read_backups(SConfig *conf, unsigned char *buffer)
{
	size_t offset = find_category("Backup", buffer);
	size_t new_off = offset;
	
	while(new_off != -1 && conf->num_backups < 256)
	{
		parse_backup(conf, buffer, offset);
		
		new_off = find_category("Backup", &buffer[offset]);
		offset += new_off;
	}
}

void parse_backup(SConfig *conf, unsigned char *buffer, size_t offset)
{
	int id = conf->num_backups;
	conf->backups[id] = malloc(sizeof(SBackup));
	
	// Include the [Backup] tag
	offset -= 9;
	
	get_setting((void *)&conf->backups[id]->is_recursive, TYPE_BOOL, "Backup", "Recursive", "true", &buffer[offset]);
	get_setting((void *)&conf->backups[id]->keep_revisions, TYPE_BOOL, "Backup", "KeepRevisions", "true", &buffer[offset]);
	get_setting((void *)&conf->backups[id]->do_compress, TYPE_BOOL, "Backup", "Compress", "true", &buffer[offset]);
	get_setting((void *)&conf->backups[id]->do_ignore_tildes, TYPE_BOOL, "Backup", "IgnoreTildes", "true", &buffer[offset]);
	
	get_setting(&conf->backups[id]->name, TYPE_STR, "Backup", "Name", "<nameless>", &buffer[offset]);
	get_setting(&conf->backups[id]->dir, TYPE_STR, "Backup", "Directory", "<none>", &buffer[offset]);
	get_setting(&conf->backups[id]->files, TYPE_STR, "Backup", "AlsoThese", "", &buffer[offset]);
	get_setting(&conf->backups[id]->types, TYPE_STR, "Backup", "Filetypes", "*", &buffer[offset]);
	
	conf->num_backups++;
}

void get_setting(void *conf, int type, char *category, char *setting, char *def, unsigned char *buffer)
{
	int len = 0;
	unsigned char *value = 0;
	parse_setting(category, setting, def, buffer, &value);
	
	if (value[0] == -1)
	{
		debug_print("Found an unset value in config, using default.", DEBUG_DEBUG, 1);
		free(value);
		
		size_t def_len = strlen(def);
		value = malloc(sizeof(char) * def_len + 1);
		strncpy((char*)value, def, def_len);
		value[def_len] = 0;
	}
	
	switch (type)
	{
	case TYPE_BOOL:
		*(int *)conf = !strcasecmp((char*)value, "true");
		break;
		
	case TYPE_INT:
		*(int *)conf = atoi((char*)value);
		break;
		
	case TYPE_STR:
		len = strlen((char*)value);
		*(char**)conf = malloc(sizeof(char) * len + 1);
		strncpy(*(char**)conf, (char*)value, len);
		(*(char**)conf)[len] = 0;
		break;
		
	default:
		break;
	}
	
	free(value);
	value = 0;
}

void parse_setting(char *category, char *setting, char *def, unsigned char *buffer, unsigned char **ret)
{
	unsigned char *line = 0;
	size_t offset = find_category(category, buffer);
	
	if (offset != -1)
	{
		size_t setting_offset = find_setting(setting, &buffer[offset]);

		if (setting_offset != -1)
		{
			unsigned char *tmp = 0;
			get_line(setting_offset + offset, buffer, &tmp);
			
			if (strlen((char*)tmp))
			{
				trim(tmp, &line);
			}
			else
			{
				line = malloc(sizeof(char));
				line[0] = -1;
			}
			
			free(tmp);
			
			get_setting_value(line, ret);
			free(line);

			return;
		}
		else
		{
			debug_print("Failed to find setting '%s' in config, using default.", DEBUG_DEBUG, 1, setting);
		}
	}
	else
	{
		debug_print("Failed to find category '%s' in config!", DEBUG_WARNING, 1, category);
	}
	
	size_t size = strlen(def);
	*ret = malloc(sizeof(char) * size + 1);
	memcpy(*ret, def, size + 1);
}

void get_setting_value(unsigned char *line, unsigned char **ret)
{
	size_t line_len = strlen((char*)line);
	size_t offset = line_len;
	unsigned char *ptr = &line[line_len];
	
	while(*ptr != '=' && offset > 0)
	{
		--offset;
		*--ptr;
	}
	
	unsigned char *tmp = 0;
	
	// If no = sign was found, return -1
	if (offset == 0)
	{
		*ret = malloc(sizeof(char));
		*ret[0] = -1;
		return;
	}
	
	// If no value was set, return an empty char array
	if (offset == (line_len - 1))
	{
		*ret = malloc(sizeof(char));
		*ret[0] = 0;
		return;
	}
	
	size_t size = line_len - offset;
	tmp = malloc(sizeof(char*) * size + 1);
	memcpy(tmp, &line[offset + 1], size);
	tmp[size] = 0;
	
	trim(tmp, ret);
	free(tmp);
}

size_t find_setting(char *setting, unsigned char *buffer)
{
	unsigned char *line = 0;
	unsigned char *trimmed = 0;
	size_t line_len = 0;
	size_t offset = 0;
	size_t buf_len = strlen((char*)buffer);
	size_t setting_len = strlen(setting);
	
	while (line == 0 && offset < buf_len)
	{
		offset = get_line(offset, buffer, &line);
		line_len = strlen((char*)line);
		
		if (line_len)
		{
			trim(line, &trimmed);
			free(line);
			line = 0;
			
			size_t trim_len = strlen((char*)trimmed);
			char next_char = trimmed[setting_len];
			
			if (trim_len && (next_char == ' ' || next_char == 9 || next_char == '=' || next_char == '\n') && !strncasecmp(setting, (char*)trimmed, setting_len))
			{
				free(trimmed);
				trimmed = 0;
				
				return offset - line_len;
			}
			
			if ((trim_len && trimmed[0] == '[' && trimmed[trim_len - 1] == ']') || (offset+1) >= buf_len)
			{
				break;
			}
			
			free(trimmed);
			trimmed = 0;
		}
		else
		{
			free(line);
			line = 0;
		}
	}
	
	if (trimmed)
		free(trimmed);
	
	if (line)
			free(line);
	
	return -1;
}

// Returns the offset to the beginning of a category (not including category tag)
size_t find_category(char *category, unsigned char *buffer)
{
	unsigned char *line = 0;
	unsigned char *trimmed = 0;
	size_t offset = 0;
	size_t buf_len = strlen((char*)buffer);
	
	while (line == 0 && offset < buf_len)
	{
		offset = get_line(offset, buffer, &line);
		
		if (strlen((char*)line))
		{
			trim(line, &trimmed);
			free(line);
			line = 0;
			
			size_t trim_len = strlen((char*)trimmed);
			
			if (trim_len && trimmed[0] == '[' && trimmed[trim_len - 1] == ']')
			{
				trimmed[trim_len - 1] = 0;
				
				if (!strcasecmp(category, (char*)&trimmed[1]))
				{
					free(trimmed);
					trimmed = 0;
					
					return offset + 1;
				}
			}
			
			if (offset >= (buf_len - 1))
			{
				free(trimmed);
				trimmed = 0;
				
				break;
			}
		}
		else
		{
			free(line);
			line = 0;
		}
	}
	
	if (line)
			free(line);
	
	return -1;
}

// Gets the rest of the line from offset
size_t get_line(size_t offset, unsigned char *buffer, unsigned char **ret)
{
	size_t size = 0;
	unsigned char *ptr = &buffer[offset];
	
	while(*ptr++ != '\n' && *ptr != 0)
	{
		++size;
	}
	
	// Return an empty string if only a newline was encountered
	if (size == 0)
	{
		*ret = malloc(sizeof(char));
		*ret[0] = 0;
		
		return offset + 1;
	}
	
	*ret = malloc(sizeof(char) * size + 1);
	memcpy(*ret, &buffer[offset], size);
	*(*(ret) + size) = 0;
	
	return offset + size;
}

int trim(unsigned char *buf, unsigned char **ret)
{
	int i = 0;
	int j = 0;
	
	int len = strlen((char*)buf);
		
	while(buf[i] == 9 || buf[i] == ' ' || buf[i] == '\n')
		i++;
		
	while(buf[len - j - 1] == 9 || buf[len - j - 1] == ' '  || buf[len - j -1] == '\n')
		j++;
	
	*ret = malloc(sizeof(char) * (len - j) + 1);
	memcpy(*ret, &buf[i], (len - j));
	*(*(ret) + (len - j)) = 0;
	
	return 1;
}

void clean_config(SConfig *conf)
{
	debug_print("Freeing allocated config memory.", DEBUG_DEBUG, 1);
	
	for (int i = 0; i < conf->num_backups; ++i)
	{
		free(conf->backups[i]->dir);
		free(conf->backups[i]->name);
		free(conf->backups[i]->files);
		free(conf->backups[i]->types);
		free(conf->backups[i]);
	}
	
	free(conf->passphrase);
	free(conf);
}

