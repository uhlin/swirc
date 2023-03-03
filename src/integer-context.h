#ifndef INTEGER_CONTEXT_H
#define INTEGER_CONTEXT_H

struct integer_context {
	const char	*setting_name; /* Name of the setting or item */
	long int	 lo_limit; /* Low limit */
	long int	 hi_limit; /* High limit */
	long int	 fallback_default; /* If the range is exceeded  --  use this value */

#ifdef __cplusplus
	integer_context() : setting_name("")
	    , lo_limit(0)
	    , hi_limit(0)
	    , fallback_default(0)
	{
		/* null */;
	}

	integer_context(const char *p_setting_name,
	    long int p_lo_limit,
	    long int p_hi_limit,
	    long int p_fallback_default) : setting_name(p_setting_name)
	    , lo_limit(p_lo_limit)
	    , hi_limit(p_hi_limit)
	    , fallback_default(p_fallback_default)
	{
		/* null */;
	}
#endif
};

#endif
