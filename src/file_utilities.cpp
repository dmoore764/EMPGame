void GetFileBaseName(char *basename, const char *filename)
{
	const char *lastdot = filename;
	while (lastdot && strchr(lastdot, '.'))
		lastdot = strchr(lastdot, '.') + 1;
	sprintf(basename, "%.*s", (int)(lastdot - 1 - filename), filename);
}

const char* GetFileName(const char *fullPath) {
	// Get the position of the last forward and back slash
	const char *lastForward = strrchr(fullPath, '/');
	const char *lastBack = strrchr(fullPath, '\\');

	// Increase the position by one, as long as it isn't null
	lastForward = lastForward ? lastForward + 1 : 0;
	lastBack = lastBack ? lastBack + 1 : 0;
	
	//Just the file name, no path was sent
	if (lastForward == NULL && lastBack == NULL)
		return fullPath;

	return lastForward > lastBack ? lastForward : lastBack;
}

#ifdef _WIN32
const char* SLASH_FORMAT = "%s\\%s";
#else
const char* SLASH_FORMAT = "%s/%s";
#endif

const void CombinePath(char* target, const char* basePath, const char* more) {
	sprintf(target, SLASH_FORMAT, basePath, more);
}

void GetPath(char *dst, const char *basePath, ...)
{
    va_list args;
    va_start(args, basePath);

	char temp1[1024];
	char temp2[1024];

	strcpy(temp1, basePath);

	while (char *value = va_arg(args, char *))
	{
		CombinePath(temp2, temp1, value);
		strcpy(temp1, temp2);
	}

	va_end(args);

	strcpy(dst, temp1);
}

char *ReadFileIntoString(const char *filename)
{
#ifndef _WIN32
    char *result = NULL;
    FILE *file = fopen(filename, "r");
    
    if (file) {
        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0L, SEEK_SET);
        
        result = (char *)malloc(size + 1);
        fread(result, size, 1, file);
        result[size] = '\0';
        
        fclose(file);
    }
    
    return result;
#else
	HANDLE fileHandle = CreateFile(filename,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		// Error
		return NULL;
	}

	DWORD size = GetFileSize(fileHandle, NULL);
	DWORD bytesRead = 0;

	char *result = (char *)malloc(size + 1);

	if (FALSE == ReadFile(fileHandle, result, size - 1, &bytesRead, NULL))
	{
		// Error
		DWORD err = GetLastError();
		CloseHandle(fileHandle);
		return NULL;
	}

	result[bytesRead] = '\0';

	CloseHandle(fileHandle);
	return result;
#endif
}

void *ReadBinaryFile(const char *filename, size_t *length)
{
#ifndef _WIN32
    void *result = NULL;
    FILE *file = fopen(filename, "r");
    
    if (file) {
        fseek(file, 0L, SEEK_END);
        size_t size = ftell(file);
        *length = size;
        fseek(file, 0L, SEEK_SET);
        
        result = malloc(size);
        fread(result, size, 1, file);
        fclose(file);
    }
    
    return result;
#else
	HANDLE fileHandle = CreateFile(filename,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL, // normal file
		NULL);                 // no attr. template

	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		// Error
		return NULL;
	}

	DWORD size = GetFileSize(fileHandle, NULL);
	DWORD bytesRead = 0;

	char *result = (char *)malloc(size);

	if (FALSE == ReadFile(fileHandle, result, size, &bytesRead, NULL))
	{
		// Error
		DWORD err = GetLastError();
		CloseHandle(fileHandle);
		return NULL;
	}

	CloseHandle(fileHandle);
	return result;
#endif
}
