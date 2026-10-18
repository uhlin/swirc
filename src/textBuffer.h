#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

#include "atomicops.h"

typedef struct tagTEXTBUF_ELMT {
	char	*text;
	int	 indent;
	struct tagTEXTBUF_ELMT	*prev;
	struct tagTEXTBUF_ELMT	*next;
} TEXTBUF_ELMT, *PTEXTBUF_ELMT;

typedef struct tagTEXTBUF {
	_Atomic(int)	size;
	PTEXTBUF_ELMT	head;
	PTEXTBUF_ELMT	tail;
} TEXTBUF, *PTEXTBUF;

/*lint -sem(textbuf_get_element_by_pos, r_null) */

__SWIRC_BEGIN_DECLS
PTEXTBUF	textbuf_new(void);
PTEXTBUF_ELMT	textbuf_get_element_by_pos(const TEXTBUF *, int pos);
errno_t		textbuf_ins_next(PTEXTBUF, PTEXTBUF_ELMT, const char *text,
		    int indent);
errno_t		textbuf_ins_prev(PTEXTBUF, PTEXTBUF_ELMT, const char *text,
		    int indent);
errno_t		textbuf_remove(PTEXTBUF, PTEXTBUF_ELMT);
void		textbuf_destroy(PTEXTBUF);
void		textbuf_emplace_back(const char *fn, PTEXTBUF, const char *text,
		    int indent);
void		textbuf_pop_head(const char *fn, PTEXTBUF);
__SWIRC_END_DECLS

/* Inline function definitions
   =========================== */

static SW_INLINE int
textbuf_size(const TEXTBUF *buf)
{
	return (buf->size);
}

static SW_INLINE PTEXTBUF_ELMT
textbuf_head(const TEXTBUF *buf)
{
	return (buf->head);
}

static SW_INLINE PTEXTBUF_ELMT
textbuf_tail(const TEXTBUF *buf)
{
	return (buf->tail); // NOLINT: false positive
}

#endif
