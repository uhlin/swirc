/* Text storage with help of a linked list algorithm. ISBN: 978-1-56592-453-6 */

#include "common.h"
#include "assertAPI.h"
#include "errHand.h"
#include "libUtils.h"
#include "strHand.h"
#include "textBuffer.h"

PTEXTBUF
textBuf_new(void)
{
    PTEXTBUF buf = xcalloc(sizeof *buf, 1);

    buf->size = 0;
    buf->head = NULL;
    buf->tail = NULL;

    return buf;
}

/**
 * Get an element from a textbuffer determined by given position
 *
 * @param buf Buffer
 * @param pos Position
 * @return The element (or NULL if not found)
 */
PTEXTBUF_ELMT
textBuf_get_element_by_pos(const TEXTBUF *buf, int pos)
{
    int			i;
    PTEXTBUF_ELMT	element;

    if (buf == NULL || pos < 0)
	return NULL;

    for (i = 0, element = textBuf_head(buf); element != NULL;
	 i++, element = element->next) {
	if (i == pos)
	    return element;
    }

    return NULL;
}

int
textBuf_ins_next(PTEXTBUF buf, PTEXTBUF_ELMT element,
		 const char *text, int indent)
{
    PTEXTBUF_ELMT new_element;

    if (buf == NULL || text == NULL ||
	(element == NULL && textBuf_size(buf) != 0)) {
	return EINVAL;
    }

    new_element         = xcalloc(sizeof *new_element, 1);
    new_element->text   = sw_strdup(text);
    new_element->indent = indent;

    if (textBuf_size(buf) == 0) {
	buf->head       = new_element;
	buf->head->prev = NULL;
	buf->head->next = NULL;
	buf->tail       = new_element;
    } else {
	sw_assert(element != NULL);
	new_element->next = element->next;
	new_element->prev = element;
	if (element->next == NULL) {
	    buf->tail = new_element;
	} else {
	    element->next->prev = new_element;
	}
	element->next = new_element;
    }

    (buf->size)++;
    return 0;
}

int
textBuf_ins_prev(PTEXTBUF buf, PTEXTBUF_ELMT element, const char *text,
    int indent)
{
	PTEXTBUF_ELMT	new_element;

	if (buf == NULL || text == NULL ||
	    (element == NULL && textBuf_size(buf) != 0))
		return EINVAL;

	new_element = xcalloc(sizeof *new_element, 1);
	new_element->text = sw_strdup(text);
	new_element->indent = indent;

	if (textBuf_size(buf) == 0) {
		buf->head = new_element;
		buf->head->prev = NULL;
		buf->head->next = NULL;
		buf->tail = new_element;
	} else {
		sw_assert(element != NULL);

		new_element->next = element;
		new_element->prev = element->prev;

		if (element->prev == NULL)
			buf->head = new_element;
		else
			element->prev->next = new_element;

		element->prev = new_element;
	}

	(buf->size)++;
	return 0;
}

int
textBuf_remove(PTEXTBUF buf, PTEXTBUF_ELMT element)
{
	if (buf == NULL || element == NULL || textBuf_size(buf) == 0)
		return EINVAL;

	if (element == buf->head) {
		buf->head = element->next;

		if (buf->head == NULL)
			buf->tail = NULL;
		else
			element->next->prev = NULL;
	} else {
		element->prev->next = element->next;

		if (element->next == NULL)
			buf->tail = element->prev;
		else
			element->next->prev = element->prev;
	}

	free(element->text);
	free(element);

	(buf->size)--;
	return 0;
}

void
textBuf_destroy(PTEXTBUF buf)
{
	while (textBuf_size(buf) > 0) {
		if ((errno = textBuf_remove(buf, textBuf_tail(buf))) != 0)
			err_sys("textBuf_remove() returned %d", errno);
	}

	free(buf);
}
