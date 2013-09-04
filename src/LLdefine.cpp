#include "LLdefine.h"

#ifndef _WINDOWS

void correctSlash(char* path)
{
	char* ptr = path;
	while (*ptr)
	{
		if (*ptr == '\\')
			*ptr = '/';
		ptr++;
	}
}

#endif
