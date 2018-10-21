#ifndef FILE_PREDICATES_H
#define FILE_PREDICATES_H

#ifdef __cplusplus
extern "C" {
#endif

bool	file_exists     (const char *path);
bool	is_directory    (const char *path);
bool	is_device       (const char *path);
bool	is_regular_file (const char *path);

#ifdef __cplusplus
}
#endif

#endif
