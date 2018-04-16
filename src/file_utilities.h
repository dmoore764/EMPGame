//takes off the extension, doesn't remove the path, so it expects only filename.extension
void GetFileBaseName(char *basename, const char *filename);

// Returns the substring of a directory path that contains a the file name, if any.
const char* GetFileName(const char *fullPath);

// Takes a path with no final slash, and combines it with another path without an initial slash, placing a slash in between.
// Adds the correct slash for platform (backslash on Windows, forward slash otherwise)
// Would be nice to support ignoring final and initial slash, as well as relative path combinations,
// but just wanted to add it quick so I could remove an otherwise unnecessary Windows lib that exposes the PathCombine API.
const void CombinePath(char* target, const char* basePath, const char* more);

void GetPath(char *dst, const char *basePath, ...);

//Use c standard library to load file into a string, uses malloc, so the result can be freed later
char *ReadFileIntoString(const char *filename);

//This is for binary files
void *ReadBinaryFile(const char *filename, size_t *length);

void WriteBinaryFile(const char *filename, void *data, size_t length);
