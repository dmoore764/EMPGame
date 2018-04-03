void _dbg(const char* file, int line, const char *format, ...) {
	char fullFormat[KILOBYTES(1)];
	sprintf(fullFormat, "%s(%d): %s\n", GetFileName(file), line, format);

	char fullMessage[KILOBYTES(2)];
	va_list args;
	va_start(args, format);
	vsprintf(fullMessage, fullFormat, args);
	va_end(args);

#ifndef _WIN32
	printf("%s", fullMessage);
#else
	OutputDebugString(fullMessage);
#endif
}

void _err(const char* file, int line, const char *format, ...) {
	char fullFormat[KILOBYTES(1)];
	sprintf(fullFormat, "%s(%d): ERROR %s\n", GetFileName(file), line, format);

	char fullMessage[KILOBYTES(2)];
	va_list args;
	va_start(args, format);
	vsprintf(fullMessage, fullFormat, args);
	va_end(args);

#ifndef _WIN32
	fprintf(stderr, "%s", fullMessage);
#else
	OutputDebugString(fullMessage);
#endif	
}
