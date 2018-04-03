struct dir_file
{
	char folder_path[1024];
	char basename[128];
	char name[128];
	char ext [16];
	bool modified;
};

struct dir_folder
{
	char full_path[1024];
	char base_path[1024];
	char name[128];

	generic_hash folders;
	generic_hash files;
};

struct watch_dir_data
{
	void (*callback)(dir_folder *, char *);
	dir_folder *folder;
};

char *GetExecutableDir();
dir_folder *GetSubFolder(dir_folder *base, const char *folder);
dir_file *GetFileInDirectory(dir_folder *base, bool recursive, const char *filename);
void AddFolderToDir(dir_folder *folder, dir_folder sub_folder);
void AddFileToDir(dir_folder *folder, const char *filename);
void ReloadDirectory(dir_folder *folder);
dir_folder LoadDirectory(const char *directory, const char *sub_path = NULL);
void PrintDirectoryChildren(dir_folder *folder, int indent_level);
void DeleteContentsOfFolder(dir_folder *folder);

#ifndef _WIN32
	//Includes specific mac parameters
	void DirModCallBack(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);
#endif

void *watch_directory_function( void *ptr );
void WatchDirectory(dir_folder *folder, void (*callback)(dir_folder *folder, char *filename));
