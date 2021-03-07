#ifndef SPECIAL_MSG_CONTEXT_HPP
#define SPECIAL_MSG_CONTEXT_HPP

struct special_msg_context {
    char *nick;
    char *user;
    char *host;
    char *dest;
    char *msg;

    special_msg_context()
	{
	    this->nick = NULL;
	    this->user = NULL;
	    this->host = NULL;
	    this->dest = NULL;
	    this->msg = NULL;
	}

    special_msg_context(char *nick, char *user, char *host, char *dest,
			char *msg)
	{
	    this->nick = nick;
	    this->user = user;
	    this->host = host;
	    this->dest = dest;
	    this->msg = msg;
	}
};

#endif
