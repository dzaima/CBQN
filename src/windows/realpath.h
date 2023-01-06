#ifndef REALPATH_H
#define REALPATH_H

#define	S_ISLNK(m) (0)
#define	S_ISSOCK(m)	(0)

char* realpath (const char *__restrict path, char *__restrict resolved_path);

#endif /* REALPATH_H */