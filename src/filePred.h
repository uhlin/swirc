#ifndef FILE_PREDICATES_H
#define FILE_PREDICATES_H

__SWIRC_BEGIN_DECLS
bool	file_exists(const char *);
bool	is_directory(const char *);
bool	is_device(const char *);
bool	is_regular_file(const char *);
__SWIRC_END_DECLS

#endif
