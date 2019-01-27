#ifndef LIBRARY_UTILITIES_H
#define LIBRARY_UTILITIES_H

#include <stdio.h> /* FILE */

/*lint -function(fopen, xfopen) */
/*lint -printf(1, say) */
/*lint -printf(2, write_to_stream) */

#ifdef __cplusplus
extern "C" {
#endif

FILE		*fopen_exit_on_error   (const char *path, const char *mode);
FILE		*xfopen                (const char *path, const char *mode);
const char	*current_time          (const char *fmt);
int		 int_diff              (const int, const int);
int		 int_sum               (const int, const int);
int		 size_to_int           (const size_t);
size_t		 size_product          (const size_t elt_count, const size_t elt_size);
size_t		 xmbstowcs             (wchar_t *, const char *, size_t);
void		 fclose_ensure_success (FILE *);
void		 realloc_strcat        (char **dest, const char *src);
void		 say                   (const char *fmt, ...) PRINTFLIKE(1);
void		 write_to_stream       (FILE *, const char *fmt, ...) PRINTFLIKE(2);
void		*xcalloc               (size_t elt_count, size_t elt_size);
void		*xmalloc               (size_t);
void		*xrealloc              (void *ptr, size_t newSize);

#ifdef __cplusplus
}
#endif

/* Inline function definitions
   =========================== */

static SW_INLINE void
free_not_null(void *ptr)
{
    if (ptr != NULL) {
	free(ptr);
    }
}

static SW_INLINE void
free_and_null(char **ptr)
{
    if (*ptr == NULL) {
	return;
    }

    free(*ptr);
    *ptr = NULL;
}

#endif
