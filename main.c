#include "main.h"

int main(void)
{
	init_debug();
    SConfig *conf = init_config();
    
    if (conf)
    {
		debug_print("", DEBUG_NOTAG, 1);
    	debug_print("Loaded the following backup targets:", DEBUG_NOTAG, 1);
		
		for (int i = 0; i < conf->num_backups; i++)
			debug_print("\t%s (%s)", DEBUG_NOTAG, 1, conf->backups[i]->name, conf->backups[i]->dir);
				
		debug_print("", DEBUG_NOTAG, 1);
    	
    	//encrypt_file(conf, "debian.iso", "test.gpg");
		
		sdb_t *db = 0;
		load_sdb("test2.sdb", &db);
		sort_sdb(&db);
		
		sd_dir_t *dir = 0;
		
		for (int i = 0; i < conf->num_backups; i++)
		{
			debug_print("Reading: %s (%s)", DEBUG_DEBUG, 1, conf->backups[i]->name, conf->backups[i]->dir);
			read_directory(conf->backups[i]->dir, conf->backups[i], &dir);
			debug_print("", DEBUG_NOTAG, 1);
			
			for (int j = 0; j < dir->num_files; j++)
			{
				unsigned char *data;
				unsigned char hash[20];
				size_t size = read_file(dir->files[j].name, &data);
				
				sha1(data, size, hash, 1);
				
				// DB stuff
				printf("%s\n", dir->files[j].name);
				add_row_sdb(&db, dir->files[j].name, dir->name, hash, conf->backups[i]->keep_revisions);
				
				free(data);
			}
			
			free_dir(dir);
		}
		
		//save_sdb("test2.sdb", db);
		free_sdb(db);
    }
    else
    {
    	debug_print("Failed to initialize config! Quitting!", DEBUG_ERROR, 1);
		return 1;
    }
    
    clean_config(conf);
    debug_print("\n\n---------------------------------------\n\t\tExiting!\n---------------------------------------\n", DEBUG_NOTAG, 1);
    clean_debug();
    
    return 0;
}

