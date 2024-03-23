#ifndef FETCH_DICTIONARY_H
#define FETCH_DICTIONARY_H

#define AFF_SUFFIX_LEN 5
#define DIC_SUFFIX_LEN 5

__SWIRC_BEGIN_DECLS
extern const char	 g_aff_suffix[AFF_SUFFIX_LEN];
extern const char	 g_dic_suffix[DIC_SUFFIX_LEN];

void	cmd_fetchdic(const char *);
__SWIRC_END_DECLS

#endif
