# The 'tidy' target

TIDY = clang-tidy
TIDYFLAGS = -checks=cert-*,-clang-analyzer-security.insecureAPI.strcpy,-clang-analyzer-optin.performance.Padding,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling,performance-*,portability-* -quiet
SW_CLANG_TIDYFLAGS ?=

tidy: $(SRC_DIR)include/swircpaths.h
	$(TIDY) $(COMMANDS_SRCS) $(EVENTS_SRCS) $(SRCS) $(TIDYFLAGS) \
	$(SW_CLANG_TIDYFLAGS) -- $(CPPFLAGS)
