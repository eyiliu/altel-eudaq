#include <cstdlib>
#include <cctype>

char* __cxa_demangle(const char* mangled_name,
                     char* buf,
                     size_t* n,
                     int* status);

typedef void* (*malloc_func_t) (size_t);
typedef void (*free_func_t) (void*);


char* __unDName(char* buffer,
                const char* mangled,
                int buflen,
                malloc_func_t memget,
                free_func_t memfree,
                unsigned short int flags);
