#ifndef CMD_SQUERY_H
#define CMD_SQUERY_H

#define ns_cmds_macro(x)\
	(#x " "),\
	(#x " access "),\
	(#x " access add "),\
	(#x " access del "),\
	(#x " access list"),\
	(#x " access list "),\
	(#x " alist"),\
	(#x " alist "),\
	(#x " cert add "),\
	(#x " cert del "),\
	(#x " cert list"),\
	(#x " cert list "),\
	(#x " confirm "),\
	(#x " drop "),\
	(#x " glist"),\
	(#x " group"),\
	(#x " group "),\
	(#x " identify "),\
	(#x " info"),\
	(#x " info "),\
	(#x " list "),\
	(#x " logout"),\
	(#x " logout "),\
	(#x " recover "),\
	(#x " register "),\
	(#x " resetpass "),\
	(#x " set "),\
	(#x " set autoop "),\
	(#x " set autoop on"),\
	(#x " set autoop off"),\
	(#x " set display "),\
	(#x " set email "),\
	(#x " set greet "),\
	(#x " set hide "),\
	(#x " set hide email "),\
	(#x " set hide email on"),\
	(#x " set hide email off"),\
	(#x " set hide status "),\
	(#x " set hide status on"),\
	(#x " set hide status off"),\
	(#x " set hide usermask "),\
	(#x " set hide usermask on"),\
	(#x " set hide usermask off"),\
	(#x " set hide quit "),\
	(#x " set hide quit on"),\
	(#x " set hide quit off"),\
	(#x " set keepmodes "),\
	(#x " set keepmodes on"),\
	(#x " set keepmodes off"),\
	(#x " set kill "),\
	(#x " set kill on"),\
	(#x " set kill quick"),\
	(#x " set kill immed"),\
	(#x " set kill off"),\
	(#x " set language "),\
	(#x " set password "),\
	(#x " set private "),\
	(#x " set private on"),\
	(#x " set private off"),\
	(#x " set secure "),\
	(#x " set secure on"),\
	(#x " set secure off"),\
	(#x " set url "),\
	(#x " status"),\
	(#x " status "),\
	(#x " ungroup"),\
	(#x " ungroup "),\
	(#x " update")

__SWIRC_BEGIN_DECLS

void cmd_squery(CSTRING) NONNULL;

//lint -sem(get_list_of_matching_squery_commands, r_null)
PTEXTBUF get_list_of_matching_squery_commands(CSTRING);

__SWIRC_END_DECLS

#endif
