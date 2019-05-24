#include "debug.h"

const short int DEBUG_OFF = 0;
const short int DEBUG_ERROR = 1;
const short int DEBUG_WARNING = 2;
const short int DEBUG_INFO = 3;
const short int DEBUG_DEBUG = 4;
const short int DEBUG_NOTAG = 0;

short int INTEGER_MAXLEN = 32;

char *logfile = NULL;
char *DEBUG_LOG_TAG[5] = {"", "[ERROR]\t", "[WARN]\t", "[INFO]\t", "[DEBUG]\t"};
char *DEBUG_TAG[5] = {"", "[\x1b[31mERROR\x1b[0m]\t", "[\x1b[33mWARN\x1b[0m]\t", "[\x1b[0mINFO\x1b[0m]\t", "[\x1b[35mDEBUG\x1b[0m]\t"};
int DEBUG_LEVEL = 4;
int LOG_LEVEL = 4;

void init_debug()
{
	char tmp[256];
	snprintf(tmp, 256, "%d", INT32_MAX);
	INTEGER_MAXLEN = strlen(tmp) + 1;
	
	set_logfile("safedrive.log");
}

void write_log(char *str, int level)
{
    if (logfile && LOG_LEVEL != DEBUG_OFF && level <= LOG_LEVEL)
    {
		char str_time[64];
		strftime(str_time, sizeof(str_time), "%d. %b @ %H:%M:%S", &(*localtime(&(time_t){ time(NULL) })));
   
		char *msg = malloc(sizeof(char)*1024);
		int size = snprintf(msg, 1023, "%s: %s%s\n", str_time, DEBUG_LOG_TAG[level], str);
	
		if (size > 1023) size = 1023;
		msg[size] = 0;
	
	    write_file(logfile, msg, 1);
	    free(msg);
    }
    else if (!logfile)
    {
    	debug_print("Failed to save log! No log file has been set.", DEBUG_ERROR, 0);
    }
}

void debug_print(char *msg, int level, char do_log, ...)
{
	char message[1024] = "";
	char msg_holder[1024] = "";
	char msg_buffer[1024] = "";
	
	if (DEBUG_LEVEL != DEBUG_OFF && level <= DEBUG_LEVEL)
	{
		char *buffer = malloc(sizeof(char) * strlen(msg) + 2);
		memcpy(buffer, msg, strlen(msg));
		buffer[strlen(msg)] = 0;
		
		if (!strchr(buffer, '%'))
		{
			snprintf(message, 1024, "%s", msg);
		}
		else
		{
			va_list ap;
			size_t len = strlen(msg);
			int offset = 0;
			int part_len = 0;
			int diff = 0;
			int valid = 0;
			
			va_start(ap, do_log);
			
			for(int i = 0; (offset + part_len) < len; i++)
			{
				offset += part_len;
				valid = 0;
				diff = 0;
				
				char *str = malloc(sizeof(char) * strlen(msg) + 2);
				memcpy(str, msg, strlen(msg) + 1);
				
				while(!valid && (offset + diff) < len)
				{
					part_len = strcspn(&str[offset + diff], "%\0") + 1;
					diff += part_len;
				
					// Only stop at valid variables (%s, %c, %d) or non-variable strings
					if ((str[offset + diff - 1] == '%') && (str[offset + diff] == 's' || str[offset + diff] == 'c' || str[offset + diff] == 'd'))
					{
						valid = 1;
					}
					else if (str[offset + diff - 1] != '%')
					{
						valid = 1;
					}
				}
				
				part_len = diff;
				
				if (part_len + offset > len) part_len = len - offset;
				
				memcpy(buffer, &str[offset], part_len + 1);
				buffer[part_len + 1] = 0;
				
				snprintf(msg_holder, 1024, "%s", message);
				
				// Let printf interpret the argument
				if (buffer[part_len] == 'd')
				{
					snprintf(msg_buffer, 1024, "%s%s", msg_holder, buffer);
					snprintf(message, 1024, msg_buffer, va_arg(ap, int));
				}
				else if (buffer[part_len] == 's')
				{
					snprintf(msg_buffer, 1024, buffer, va_arg(ap, char *));
					snprintf(message, 1024, "%s%s", msg_holder, msg_buffer);
				}
				else if (buffer[part_len] == 'c')
				{
					snprintf(msg_buffer, 1024, "%s%s", msg_holder, buffer);
					snprintf(message, 1024, msg_buffer, (char)va_arg(ap, int));
				}
				
				// Next loop skip the letter
				offset++;
			}
			
			// Don't copy what remains in the buffer if it was a valid variable %s, %d or %c
			int buf_len = strlen(buffer);
			if (buffer[buf_len - 2] != '%' || (buffer[buf_len - 1] != 's' && buffer[buf_len - 1] != 'c' && buffer[buf_len - 1] != 'd'))
				snprintf(message, 1024, "%s%s", msg_holder, buffer);
			
			va_end(ap);
		}
		
		free(buffer);
		buffer = 0;
	}
	
	printf("%s", DEBUG_TAG[level]);
	printf("%s", message);
	printf("\n");
	
	if (do_log == 1)
	{
		write_log(message, level);
	}
}

void set_levels(int log, int debug)
{
	LOG_LEVEL = log;
    DEBUG_LEVEL = debug;
}

void set_logfile(char *filename)
{
	if (logfile != NULL)
		free(logfile);
		
	int len = strlen(filename)+1;
	logfile = malloc(sizeof(char)*len);
	strncpy(logfile, filename, len);
}

char *get_logfile()
{
	return logfile;
}

void clean_debug()
{
	free(logfile);
	logfile = NULL;
}

