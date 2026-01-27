# The 'tidy' target

CHKS = cert-*,$\
	-clang-analyzer-security.insecureAPI.strcpy,$\
	-clang-analyzer-optin.performance.Padding,$\
	-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,$\
	hicpp-deprecated-headers,$\
	hicpp-multiway-paths-covered,$\
	hicpp-special-member-functions,$\
	hicpp-use-auto,$\
	hicpp-use-emplace,$\
	performance-*,$\
	portability-*

TIDY = clang-tidy
TIDYFLAGS = -checks=$(CHKS) -quiet
SW_CLANG_TIDYFLAGS ?=

tidy: $(SRC_DIR)include/swircpaths.h
	$(TIDY) $(COMMANDS_SRCS) $(EVENTS_SRCS) $(SRCS) $(TIDYFLAGS) \
	$(SW_CLANG_TIDYFLAGS) -- $(CPPFLAGS)
