// A macro to either print with printf on Mac or OutputDebugString on windows.
// Has similar argumetns to printf, and includes file and line number in message.
// Final newline is included.
#ifdef __APPLE__
	//Reuse printf functionality so compiler can catch errors in the format string
	#define DBG(format, ...) {printf("%s(%*d): ",__FILE__,4,__LINE__);printf(format, ##__VA_ARGS__);printf("\n");}
#else
	#define DBG(format, ...) _dbg(__FILE__, __LINE__, format, __VA_ARGS__)
#endif

// A macro to either print to standard error on Mac or OutputDebugString on windows.
// Has similar argumetns to printf, and includes file and line number in message.
// Final newline is included.
#ifdef __APPLE__
	#define ERR(format, ...) {fprintf(stderr,"%s(%*d): ",__FILE__,4,__LINE__);fprintf(stderr,format, ##__VA_ARGS__);fprintf(stderr,"\n");}
#else
	#define ERR(format, ...) _err(__FILE__, __LINE__, format, __VA_ARGS__)
#endif

// Implementation of DBG macro
void _dbg(const char* file, int line, const char *format, ...);

// Implementation of ERR macro
void _err(const char* file, int line, const char *format, ...);
