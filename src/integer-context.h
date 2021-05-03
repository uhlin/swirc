#ifndef INTEGER_CONTEXT_H
#define INTEGER_CONTEXT_H

struct integer_context {
    char	*setting_name;  /* Name of the setting or item */
    long int	 lo_limit;      /* Low limit */
    long int	 hi_limit;      /* High limit */
    long int	 fallback_default;	/* If the range is exceeded  --  use this value */
};

#endif
