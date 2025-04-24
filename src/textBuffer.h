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

/*lint -sem(textBuf_get_element_by_pos, r_null) */

__SWIRC_BEGIN_DECLS
PTEXTBUF	textBuf_new(void);
PTEXTBUF_ELMT	textBuf_get_element_by_pos(const TEXTBUF *, int pos);
errno_t		textBuf_ins_next(PTEXTBUF, PTEXTBUF_ELMT, const char *text,
		    int indent);
errno_t		textBuf_ins_prev(PTEXTBUF, PTEXTBUF_ELMT, const char *text,
		    int indent);
errno_t		textBuf_remove(PTEXTBUF, PTEXTBUF_ELMT);
void		textBuf_destroy(PTEXTBUF);
void		textBuf_emplace_back(const char *fn, PTEXTBUF, const char *text,
		    int indent);
__SWIRC_END_DECLS

/* Inline function definitions
   =========================== */

static SW_INLINE int
textBuf_size(const TEXTBUF *buf)
{
	return (buf->size);
}

static SW_INLINE PTEXTBUF_ELMT
textBuf_head(const TEXTBUF *buf)
{
	return (buf->head);
}

static SW_INLINE PTEXTBUF_ELMT
textBuf_tail(const TEXTBUF *buf)
{
	return (buf->tail); // NOLINT: false positive
}

#endif
