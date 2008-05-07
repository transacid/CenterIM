
#include <icqcommon.h>


#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifndef HAVE_SETENV
/* Emulation for IRIX which has no setenv(3)*/
int setenv(const char *name, const char *value, const int overwrite) {
        char *buffer = NULL;
        int size = strlen(name) + strlen(value) + 2;
        if ((buffer = (char*)calloc(size, sizeof(char)))) {
                snprintf(buffer, size, "%s=%s", name, value);
                return putenv(buffer);
        }
        return -1;
}
#endif

