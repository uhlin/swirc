/* Text storage with help of a linked list algorithm. ISBN: 978-1-56592-453-6 */

#include "common.h"
#include "assertAPI.h"
#include "errHand.h"
#include "libUtils.h"
#include "strHand.h"
#include "textBuffer.h"

static int
check_args(PTEXTBUF buf, PTEXTBUF_ELMT element, const char *text)
{
	if (buf == NULL || text == NULL || (element == NULL &&
	    textbuf_size(buf) != 0))
		return -1;
	return 0;
}

static PTEXTBUF_ELMT
get_new_elmt(const char *text, int indent)
{
	PTEXTBUF_ELMT	new_element;

	new_element = xcalloc(sizeof *new_element, 1);
	new_element->text = sw_strdup(text);
	new_element->indent = indent;

	return new_element;
}

static void
init_buf(PTEXTBUF buf, PTEXTBUF_ELMT new_element)
{
	buf->head = new_element;
	buf->head->prev = NULL;
	buf->head->next = NULL;
	buf->tail = new_element;
}

PTEXTBUF
textbuf_new(void)
{
	PTEXTBUF	buf = xcalloc(sizeof *buf, 1);

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
textbuf_get_element_by_pos(const TEXTBUF *buf, int pos)
{
	PTEXTBUF_ELMT element;
	int i;

	if (buf == NULL || pos < 0)
		return NULL;

	element = textbuf_head(buf);
	i = 0;

	while (element != NULL) {
		if (i == pos)
			return element;
		element = element->next;
		i++;
	}

	return NULL;
}

errno_t
textbuf_ins_next(PTEXTBUF buf, PTEXTBUF_ELMT element, const char *text,
    int indent)
{
	PTEXTBUF_ELMT	new_element;

	if (check_args(buf, element, text) == -1)
		return EINVAL;

	new_element = get_new_elmt(text, indent);

	if (textbuf_size(buf) == 0) {
		init_buf(buf, new_element);
	} else {
		sw_assert(element != NULL);

		new_element->next = element->next; // NOLINT: false positive
		new_element->prev = element;

		if (element->next == NULL)
			buf->tail = new_element;
		else
			element->next->prev = new_element;

		element->next = new_element;
	}

	(buf->size)++;
	return 0;
}

errno_t
textbuf_ins_prev(PTEXTBUF buf, PTEXTBUF_ELMT element, const char *text,
    int indent)
{
	PTEXTBUF_ELMT	new_element;

	if (check_args(buf, element, text) == -1)
		return EINVAL;

	new_element = get_new_elmt(text, indent);

	if (textbuf_size(buf) == 0) {
		init_buf(buf, new_element);
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

errno_t
textbuf_remove(PTEXTBUF buf, PTEXTBUF_ELMT element)
{
	if (buf == NULL || element == NULL || textbuf_size(buf) == 0)
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
textbuf_destroy(PTEXTBUF buf)
{
	while (textbuf_size(buf) > 0) {
		if ((errno = textbuf_remove(buf, textbuf_tail(buf))) != 0)
			err_sys("%s: textbuf_remove", __func__);
	}

	free(buf);
}

void
textbuf_emplace_back(const char *fn, PTEXTBUF buf, const char *text, int indent)
{
	if (textbuf_size(buf) != 0) {
		if ((errno = textbuf_ins_next(buf, textbuf_tail(buf), text,
		    indent)) != 0)
			err_sys("%s: textbuf_ins_next", fn);
	} else {
		if ((errno = textbuf_ins_next(buf, NULL, text, indent)) != 0)
			err_sys("%s: textbuf_ins_next", fn);
	}
}

void
textbuf_pop_head(const char *fn, PTEXTBUF buf)
{
	if ((errno = textbuf_remove(buf, textbuf_head(buf))) != 0)
		err_sys("%s: textbuf_remove", fn);
}
