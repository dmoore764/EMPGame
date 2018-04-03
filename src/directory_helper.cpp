//Get the directory of the executable - will need to be different for Windows
char *GetExecutableDir()
{
	char *result = (char *)malloc(1024);

	#ifndef _WIN32
		int ret;
		pid_t pid; 
		char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
		char temp[PROC_PIDPATHINFO_MAXSIZE];
		
		pid = getpid();
		ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
		if ( ret <= 0 ) {
			ERR("PID %d: proc_pidpath ();\n", pid);
			ERR("    %s\n", strerror(errno));
		} else {
			sprintf(temp, "%.*s", (int)(strrchr(pathbuf, '/') - pathbuf), pathbuf);
			sprintf(result, "%.*s", (int)(strrchr(temp, '/') - temp), temp);
		}
	#else
		HMODULE hModule = GetModuleHandleW(NULL);
		CHAR pathbuf[MAX_PATH];
		CHAR temp[MAX_PATH];
		if (GetModuleFileName(hModule, pathbuf, MAX_PATH) > 0) {
			sprintf(temp, "%.*s", (int)(strrchr(pathbuf, '\\') - pathbuf), pathbuf);
			sprintf(result, "%.*s", (int)(strrchr(temp, '\\') - temp), temp);
		}
	#endif

	return result;
}


dir_folder *GetSubFolder(dir_folder *base, const char *folder)
{
	dir_folder *result = NULL;
	result = (dir_folder *)GetFromHash(&base->folders, folder);
	return result;
}

dir_file *GetFileInDirectory(dir_folder *base, bool recursive, const char *filename)
{
	dir_file *result = NULL;
	result = (dir_file *)GetFromHash(&base->files, filename);
	if (!result && recursive)
	{
		//Iterating through the hash involves going through each slot, 
		//checking to see if a hash item is there, and if it its, 
		//going through the linked list of items at that location
		for (int i = 0; i < base->folders.table_size; i++)
		{
			hash_item *item = base->folders.items[i];
			while (item)
			{
				dir_folder *subfolder = (dir_folder *)item->data;
				result = GetFileInDirectory(subfolder, true, filename);
				if (result)
					break;
				item = item->next;
			}
			if (result)
				break;
		}
	}
	return result;
}

void AddFolderToDir(dir_folder *folder, dir_folder sub_folder)
{
	dir_folder *new_sub_folder = (dir_folder *)calloc(sizeof(dir_folder), 1);
	*new_sub_folder = sub_folder;
	
	AddToHash(&folder->folders, new_sub_folder, new_sub_folder->name);
}

void AddFileToDir(dir_folder *folder, const char *filename)
{
	dir_file *new_file = (dir_file *)calloc(sizeof(dir_file), 1);
#ifndef _WIN32
	sprintf(new_file->folder_path, "%s/", folder->full_path);
#else
	sprintf(new_file->folder_path, "%s\\", folder->full_path);
#endif

	if (strchr(filename, '.') == NULL)
	{
		strcpy(new_file->name, filename);
		sprintf(new_file->ext, "");
	}
	else
	{
		const char *lastdot = strrchr(filename, '.');
		if (lastdot == NULL)
			lastdot = filename;

		sprintf(new_file->basename, "%.*s", (int)(lastdot - filename), filename);
		sprintf(new_file->name, "%s", filename);
		sprintf(new_file->ext,  "%s", lastdot + 1);
	}

	AddToHash(&folder->files, new_file, filename);
}

void ReloadDirectory(dir_folder *folder)
{
	char base_path[1024];
	if (strlen(folder->name) > 0)
		*folder = LoadDirectory(folder->base_path, folder->name);
	else
		*folder = LoadDirectory(folder->base_path);
}

dir_folder LoadDirectory(const char *directory, const char *sub_path)
{
	dir_folder result = {};
	InitHash(&result.folders, 16);
	InitHash(&result.files, 16);

	strcpy(result.base_path, directory);
	char base_path[1024];
	if (sub_path)
	{
		sprintf(result.name, "%s", sub_path);
		CombinePath(base_path, result.base_path, result.name);
	}
	else
	{
		sprintf(result.name, "");
		sprintf(base_path, "%s", result.base_path);
	}
	strcpy(result.full_path, base_path);

	#ifndef _WIN32
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir (base_path)) != NULL) {
			while ((ent = readdir (dir)) != NULL) {
				switch (ent->d_type)
				{
					case DT_DIR:
					{
						if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0))
							AddFolderToDir(&result, LoadDirectory(base_path, ent->d_name));
					} break;

					case DT_REG:
					{
						AddFileToDir(&result, ent->d_name);
					} break;
				}
			}
			closedir (dir);
		} else {
			//perror ("");
		}
	#else
		TCHAR searchPattern[MAX_PATH];
		WIN32_FIND_DATA findFileData;
		HANDLE findHandle;

		CombinePath(searchPattern, base_path, "*");
		findHandle = FindFirstFile(searchPattern, &findFileData);
		if (findHandle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if ((strcmp(findFileData.cFileName, ".") != 0) && (strcmp(findFileData.cFileName, "..") != 0))
					{
						AddFolderToDir(&result, LoadDirectory(base_path, findFileData.cFileName));
					}
				}
				else
				{
					AddFileToDir(&result, findFileData.cFileName);
				}
			}
			while (FindNextFile(findHandle, &findFileData));

			FindClose(findHandle);
		}
	#endif

	return result;
}

void PrintDirectoryChildren(dir_folder *folder, int indent_level)
{
	for (int i = 0; i < folder->folders.table_size; i++)
	{
		hash_item *item = folder->folders.items[i];
		while (item)
		{
			dir_folder *subfolder = (dir_folder *)item->data;
			DBG("%.*s/%s", indent_level*3, "                                                                     ", subfolder->name);
			PrintDirectoryChildren(subfolder, indent_level + 1);
			item = item->next;
		}
	}
	for (int i = 0; i < folder->files.table_size; i++)
	{
		hash_item *item = folder->files.items[i];
		while (item)
		{
			dir_file *subfile = (dir_file *)item->data;
			DBG("%.*s%s", indent_level*3, "                                                                     ", subfile->name);
			item = item->next;
		}
	}
}

#ifndef _WIN32

	//This function is called by OSX to alert the program that one of the specified directories had a file change
	//It allows some user data to be sent, which is the watch_dir_data struct, which contains a pointer to a function
	//that handles the specific change (different functions are sent for the shader folder and the texture folder)
	void DirModCallBack(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
	{
		int i;
		char **paths = (char **)eventPaths;
		watch_dir_data *message = (watch_dir_data *)clientCallBackInfo;

		for (int i=0; i<numEvents; i++) {
			int count;
			// flags are unsigned long, IDs are uint64_t 
			int FileNameLength = strlen(paths[i]);

			//Get the filename of the modified file (minus the directory)
			//E.g. /Users/username/Documents/ninja-game/shaders/basic.vert becomes 'basic.vert'
			char *filename = strrchr(paths[i], '/');
			if (filename)
				filename = filename + 1;
			else
				filename = paths[i];

			if ((eventFlags[i] & kFSEventStreamEventFlagMustScanSubDirs) ||
				(eventFlags[i] & kFSEventStreamEventFlagItemCreated) ||
				(eventFlags[i] & kFSEventStreamEventFlagItemModified) ||
				(eventFlags[i] & kFSEventStreamEventFlagItemRemoved) ||
				(eventFlags[i] & kFSEventStreamEventFlagItemRenamed))
			{
				if (message->callback)
				{
					//Essentially, we don't do anything specific for specific events (like file renamed/modified),
					//we just call the callback.  Also, we send the callback function the filename of the modified
					//file (which is provided by the OS)
					message->callback(message->folder, filename);
				}
			}
		}
	}


	//This function runs in a separate thread, and has an infinite loop that runs while the operating system
	//watches for file changes.  So the thread must be created before (in WatchDirectory)
	void *watch_directory_function( void *ptr )
	{
		watch_dir_data *message;
		message = (watch_dir_data *) ptr;

		//Some crap to wrangle a c-string to what the FileStream API wants
		CFStringRef mypath = CFStringCreateWithCStringNoCopy(NULL, message->folder->full_path, kCFStringEncodingMacRoman, NULL);
		CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mypath, 1, NULL);
		FSEventStreamRef stream;
		CFAbsoluteTime latency = 1.0;

		FSEventStreamContext callbackInfo = {};
		callbackInfo.info = ptr;
		
		//Tell the operating system to start watching the directory, and call back our DirModCallBack function with the data package
		stream = FSEventStreamCreate(NULL, &DirModCallBack, &callbackInfo, pathsToWatch, kFSEventStreamEventIdSinceNow, latency, kFSEventStreamCreateFlagFileEvents);
		FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		FSEventStreamStart(stream);

		//The infinite loop (some weird Mac OS thing)
		CFRunLoopRun();

		return NULL;
	}
#else
	// Based on https://msdn.microsoft.com/en-us/library/windows/desktop/aa365261%28v=vs.85%29.aspx
	int WatchDirectoryFunction(void* lpParam)
	{
		watch_dir_data *message = (watch_dir_data *)lpParam;

		// Get a handle for change notifications on the directory
		HANDLE changeHandle;
		changeHandle = FindFirstChangeNotification(
			message->folder->full_path,    // Directory to watch 
			TRUE,                          // Watch subtree 
			FILE_NOTIFY_CHANGE_LAST_WRITE); // Do not watch last write value

		if (changeHandle == INVALID_HANDLE_VALUE || changeHandle == NULL)
		{
			ERR("FindFirstChangeNotification function failed.");
		}

		// Loop, waiting for changes
		while (TRUE)
		{
			// Wait for notification.
			DBG("Waiting for notification...");

			DWORD waitStatus = WaitForSingleObject(changeHandle, INFINITE);

			switch (waitStatus)
			{
			case WAIT_OBJECT_0:

				// A file was chagned in the directory.
				// Refresh this directory and restart the notification.

				if (message->callback)
				{
					// This watch method doesn't let you get the name of the file that changed, but we aren't using this yet anyway.
					message->callback(message->folder, NULL);
				}

				if (FindNextChangeNotification(changeHandle) == FALSE)
				{
					ERR("FindNextChangeNotification function failed.");
				}
				break;

			case WAIT_TIMEOUT:

				// A timeout occurred, this would happen if some value other 
				// than INFINITE is used in the Wait call and no changes occur.
				// In a single-threaded environment you might not want an
				// INFINITE wait.

				DBG("No changes in the timeout period.");
				break;

			default:
				ERR("Unhandled dwWaitStatus.");
				break;
			}
		}
	}
#endif


//The generic function that our program calls to create a thread and alert the operating system to watch the directories for changes
//The parameters are packaged up into a struct called watch_dir_data, which must be heap allocated so that it can be sent to the other
//thread.
void WatchDirectory(dir_folder *folder, void (*callback)(dir_folder *, char *))
{
	watch_dir_data *data = (watch_dir_data *)malloc(sizeof(watch_dir_data));
	data->callback = callback;
	data->folder = folder;

	#ifndef _WIN32
		pthread_t thread1;
		int iret1;
		iret1 = pthread_create(&thread1, NULL, watch_directory_function, (void*)data);
		if(iret1)
		{
			ERR("Error - pthread_create() return code: %d", iret1);
		}
	#else
		if (SDL_CreateThread(WatchDirectoryFunction, "FileWatchThread", data) == NULL)
		{
			ERR("Failed to create folder watching thread for %s", data->folder);
		}
	#endif
}


void DeleteContentsOfFolder(dir_folder *folder)
{
	void *value;
	size_t iter = HashGetFirst(&folder->folders, NULL, NULL, &value);
	while (value)
	{
		dir_folder *sub_folder = (dir_folder *)value;
		DeleteContentsOfFolder(sub_folder);

#ifdef __APPLE__
		rmdir(sub_folder->full_path);
#else
		RemoveDirectory(sub_folder->full_path);
#endif
		HashGetNext(&folder->folders, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);


	iter = HashGetFirst(&folder->files, NULL, NULL, &value);
	while (value)
	{
		dir_file *file = (dir_file *)value;
		char full_path[1024];
		sprintf(full_path, "%s%s", file->folder_path, file->name);
#ifdef __APPLE__
		unlink(full_path);
#else
		DeleteFile(full_path);
#endif
		HashGetNext(&folder->files, NULL, NULL, &value, iter);
	}
	HashEndIteration(iter);



}
