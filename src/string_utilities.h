int strupr(char *dst, char *src)
{
	char *str=dst;
	for (;*src;str++,src++) {
		if (*src >= 'a' && *src <= 'z')
			*str = *src + ('A' - 'a');
		else
			*str = *src;
	}
	*str = '\0';
	return 0;
}

void strlwr(char *dst, char *src)
{
	char *str=dst;
	for (;*src;str++,src++) {
		if (*src >= 'A' && *src <= 'Z')
			*str = *src + ('a' - 'A');
		else
			*str = *src;
	}
	*str = '\0';
}

int strcap(char *dst, char *src)
{
	char *str=dst;
	bool first = true;
	for (;*src;str++,src++) {
		if (first)
		{
			if (*src >= 'a' && *src <= 'z')
				*str = *src + ('A' - 'a');
			else
				*str = *src;
		}
		else
		{
			if (*src >= 'A' && *src <= 'Z')
				*str = *src - ('A' - 'a');
			else
				*str = *src;
		}
		first = false;
	}
	*str = '\0';
	return 0;
}

char ToUpper(char c) {
	return (c >= 'a' && c <= 'z') ? c + 'A' - 'a' : c;
}

char ToLower(char c) {
	return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A' : c;
}

size_t ToCamelCase(char* dst, char* src, bool capitalizeFirstLetter) {
	bool nextCap = capitalizeFirstLetter;
	size_t length = 0;
	while(*src != 0) {
		if (nextCap)
		{
			*dst = ToUpper(*src);
			nextCap = false;
			src++;
			dst++;
			length++;
		} else {
			if (*src == '_') {
				nextCap = true;
				src++;
			} else {
				*dst = ToLower(*src);
				src++;
				dst++;
				length++;
			}
		}
	}
	
	*dst = 0;

	return length + 1;
}

void ReplaceChar(char* str, char find, char replace) {
	while (*str) {
		if (*str == find) {
			*str = replace;
		}
		str++;
	}
}

void FPrintIndent(FILE *file, int indentLevel)
{
	fprintf(file, "%.*s", indentLevel*4, "                                                                                                                                                           ");
}
