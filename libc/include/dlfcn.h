#ifndef _DLFCN_H
#define _DLFCN_H

#define RTLD_LAZY   0x001
#define RTLD_NOW    0x002
#define RTLD_GLOBAL 0x100
#define RTLD_LOCAL  0x000
#define RTLD_DEFAULT ((void *)0)

void *dlopen(const char *, int);
int dlclose(void *);
void *dlsym(void *, const char *);
char *dlerror(void);

#endif
