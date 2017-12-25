#ifndef INT_UNPARSE_H
#define INT_UNPARSE_H

struct integer_unparse_context {
    char	*setting_name;  /* Name of the setting or item */
    long int	 lo_limit;      /* Low limit */
    long int	 hi_limit;      /* High limit */
    long int	 fallback_default;	/* If the range is exceeded  --  use this value */
};

#endif
