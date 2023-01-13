#ifndef REALPATH_H
#define REALPATH_H

char* realpath(const char *__restrict path, char *__restrict resolved_path);
bool winIsAbsolute(const char* path);

#endif /* REALPATH_H */