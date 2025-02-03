#ifndef SPECIAL_MSG_CONTEXT_HPP
#define SPECIAL_MSG_CONTEXT_HPP

struct special_msg_context {
	CSTRING nick, user, host;
	CSTRING dest;
	CSTRING msg;

	special_msg_context() : nick(nullptr)
	    , user(nullptr)
	    , host(nullptr)
	    , dest(nullptr)
	    , msg(nullptr)
	{
		/* empty */;
	}

	special_msg_context(CSTRING p_nick, CSTRING p_user, CSTRING p_host,
	    CSTRING p_dest, CSTRING p_msg) : nick(p_nick)
	    , user(p_user)
	    , host(p_host)
	    , dest(p_dest)
	    , msg(p_msg)
	{
		/* empty */;
	}
};

#endif
