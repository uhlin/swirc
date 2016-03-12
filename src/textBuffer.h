#ifndef TEXTBUFFER_H
#define TEXTBUFFER_H

typedef struct tagTEXTBUF_ELMT {
    char	*text;
    int		 indent;
    struct tagTEXTBUF_ELMT *prev;
    struct tagTEXTBUF_ELMT *next;
} TEXTBUF_ELMT, *PTEXTBUF_ELMT;

typedef struct tagTEXTBUF {
    int			size;
    PTEXTBUF_ELMT	head;
    PTEXTBUF_ELMT	tail;
} TEXTBUF, *PTEXTBUF;

/*lint -sem(textBuf_get_element_by_pos, r_null) */

PTEXTBUF	textBuf_new                (void);
PTEXTBUF_ELMT	textBuf_get_element_by_pos (PTEXTBUF, int pos);
int		textBuf_ins_next           (PTEXTBUF, PTEXTBUF_ELMT, const char *text, int indent);
int		textBuf_ins_prev           (PTEXTBUF, PTEXTBUF_ELMT, const char *text, int indent);
int		textBuf_remove             (PTEXTBUF, PTEXTBUF_ELMT);
void		textBuf_destroy            (PTEXTBUF);

/* Inline function definitions
   =========================== */

static SW_INLINE int
textBuf_size(PTEXTBUF buf)
{
    return (buf->size);
}

static SW_INLINE PTEXTBUF_ELMT
textBuf_head(PTEXTBUF buf)
{
    return (buf->head);
}

static SW_INLINE PTEXTBUF_ELMT
textBuf_tail(PTEXTBUF buf)
{
    return (buf->tail);
}

#endif
