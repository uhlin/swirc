#ifndef NETSPLIT_H
#define NETSPLIT_H
/* netsplit.h
   Copyright (C) 2024 Markus Uhlin. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   - Neither the name of the author nor the names of its contributors may be
     used to endorse or promote products derived from this software without
     specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

#include <time.h>

#include <string>
#include <vector>

struct netsplit_context {
	CSTRING		chan;
	CSTRING		serv1, serv2;

	netsplit_context()
	    : chan(nullptr)
	    , serv1(nullptr)
	    , serv2(nullptr)
	{
		/* null */;
	}

	netsplit_context(CSTRING p_chan, CSTRING p_serv1, CSTRING p_serv2)
	    : chan(p_chan)
	    , serv1(p_serv1)
	    , serv2(p_serv2)
	{
		/* null */;
	}
};

class netsplit {
public:
	std::string			channel;
	std::string			server[2];
	std::vector<std::string>	nicks;
	std::vector<std::string>	rem_nicks;

	netsplit();
	netsplit(const struct netsplit_context *, CSTRING);

	time_t
	get_split_time(void) const
	{
		return (this->secs[0]);
	}
	void
	set_split_time(const time_t p_secs)
	{
		this->secs[0] = p_secs;
	}

	time_t
	get_join_time(void) const
	{
		return (this->secs[1]);
	}
	void
	set_join_time(const time_t p_secs)
	{
		this->secs[1] = p_secs;
	}

	bool
	has_announced_split(void) const
	{
		return (this->announced);
	}

	bool
	join_begun(void) const
	{
		return (this->secs[1] != g_time_error);
	}

	void	announce_netjoin(void) const;
	void	announce_split(void);
	bool	find_nick(CSTRING);
	bool	remove_nick(CSTRING);

private:
	/*
	 * True if announce_split() has been called.
	 */
	bool	announced;

	/*
	 * secs[0]: split time
	 * secs[1]: join time, i.e. time once somebody who left in the split has
	 *          returned. (not updated, only set once)
	 */
	time_t	secs[2];
};

//lint -sem(netsplit_find, r_null)
//lint -sem(netsplit_get_split, r_null)

__SWIRC_BEGIN_DECLS
int		 netsplit_create(const struct netsplit_context *, CSTRING);
bool		 netsplit_db_empty(void);
void		 netsplit_destroy_all(void);
netsplit	*netsplit_find(CSTRING, CSTRING);
const std::vector<netsplit *> &
		 netsplit_get_db(void);
netsplit	*netsplit_get_split(const struct netsplit_context *);
void		 netsplit_run_bkgd_task(void);

void	netsplit_init(void);
void	netsplit_deinit(void);
__SWIRC_END_DECLS

#endif
