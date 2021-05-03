#ifndef INTEGER_CONTEXT_H
#define INTEGER_CONTEXT_H

struct integer_context {
    char	*setting_name;  /* Name of the setting or item */
    long int	 lo_limit;      /* Low limit */
    long int	 hi_limit;      /* High limit */
    long int	 fallback_default;	/* If the range is exceeded  --  use this value */

#ifdef __cplusplus
    integer_context(char *setting_name, long int lo_limit, long int hi_limit,
		    long int fallback_default)
	{
	    this->setting_name = setting_name;
	    this->lo_limit = lo_limit;
	    this->hi_limit = hi_limit;
	    this->fallback_default = fallback_default;
	}
#endif
};

#endif
