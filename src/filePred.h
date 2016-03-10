#ifndef FILE_PREDICATES_H
#define FILE_PREDICATES_H

bool	file_exists     (const char *path);
bool	is_directory    (const char *path);
bool	is_device       (const char *path);
bool	is_regular_file (const char *path);

#endif
