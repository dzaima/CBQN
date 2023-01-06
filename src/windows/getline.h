#ifndef GETLINE_H
#define GETLINE_H

#include <windows.h>
#include <stdio.h>

#define MAX_LINE_LENGTH 8192

ssize_t getline(char **lptr, size_t *n, FILE *fp);

#endif /* GETLINE_H */