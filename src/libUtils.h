#ifndef LIBRARY_UTILITIES_H
#define LIBRARY_UTILITIES_H

#include <stdio.h> /* FILE */
#include <time.h>

__SWIRC_BEGIN_DECLS
extern const char	g_alphabet_upcase[];
extern const char	g_alphabet_downcase[];

extern const size_t	g_conversion_failed;
extern const time_t	g_time_error;
__SWIRC_END_DECLS

/*lint -function(fopen, xfopen) */
/*lint -printf(2, write_to_stream) */

__SWIRC_BEGIN_DECLS
FILE	*fopen_exit_on_error(const char *path, const char *mode);
FILE	*xfopen(const char *path, const char *mode);
bool	 bool_false(const char *str);
bool	 bool_true(const char *str);
bool	 getval_strtol(const char *str, const long int lo, const long int hi,
	     long int *val) PTR_ARGS_NONNULL;
bool	 time_format_ok(const char *);
const char *
	 current_time(const char *fmt);
const char *
	 getuser(void);
int	 int_diff(const int, const int);
int	 int_sum(const int, const int);
int	 size_to_int(const size_t);
size_t	 size_product(const size_t elt_count, const size_t elt_size);
size_t	 xmbstowcs(wchar_t *, const char *, size_t);
unsigned int
	 hash_djb_g(const char *str, const bool lc, const size_t upper_bound);
unsigned int
	 hash_pjw_g(const char *str, const bool lc, const size_t upper_bound);
void	 fclose_ensure_success(FILE *);
void	 realloc_strcat(char **dest, const char *src);
void	 write_setting(FILE *stream, const char *name, const char *value,
	     const bool do_padding_using_tabs, const short int count);
void	 write_to_stream(FILE *, const char *fmt, ...) PRINTFLIKE(2);
void	*xcalloc(size_t elt_count, size_t elt_size);
void	*xmalloc(size_t);
void	*xrealloc(void *ptr, size_t newSize);
__SWIRC_END_DECLS

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

/* Inline function definitions
   =========================== */

static inline void
free_not_null(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}

static inline void
free_and_null(char **ptr)
{
	if (ptr != NULL && *ptr != NULL) {
		free(*ptr);
		*ptr = NULL;
	}
}

#endif
