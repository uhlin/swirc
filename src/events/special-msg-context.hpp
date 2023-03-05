#ifndef SPECIAL_MSG_CONTEXT_HPP
#define SPECIAL_MSG_CONTEXT_HPP

struct special_msg_context {
	char	*nick;
	char	*user;
	char	*host;
	char	*dest;
	char	*msg;

	special_msg_context() : nick(NULL)
	    , user(NULL)
	    , host(NULL)
	    , dest(NULL)
	    , msg(NULL)
	{
		/* empty */;
	}

	special_msg_context(char *p_nick, char *p_user, char *p_host,
	    char *p_dest, char *p_msg) : nick(p_nick)
	    , user(p_user)
	    , host(p_host)
	    , dest(p_dest)
	    , msg(p_msg)
	{
		/* empty */;
	}
};

#endif
