# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
### Added ###
- Support for multiple encodings: UTF-8, ISO-8859-1 and ISO-8859-15.
- Command /list
- Event 321-323 (numeric replies for /list).
- Event 412 `ERR_NOTEXTTOSEND`
- Event 481 `ERR_NOPRIVILEGES`
- Event 341 `RPL_INVITING`
- Event 742 (MODE cannot be set due to channel having an active MLOCK
  restriction policy). ATM not documented in the RFC.
- Event 334. Undocumented. At Undernet, it prints out usage describing
  parameters for the LIST command.
- Event 467 `ERR_KEYSET`
- Event 471-474

### Changed ###
- The default theme. Color codes that consisted of only one digit were
  changed to use two digits in some cases. For example: ^C3 changed to
  ^C03, ^C5 changed to ^C05, etc. Why? Consider a function call like
  printtext(..., "%s`one or more digits`", Theme("primary_color")). If
  the primary color, in this case, consists of only one digit and is
  directly followed by `one or more digits` the result is: one digit
  is lost and a random color will be displayed.

### Fixed ###
- As a fallback, instead of printing out "Printtext Internal Error",
  continue to convert characters even if some are lost due to an
  invalid multibyte sequence.
- Interpretation of color codes in the printtext module. For example:
  ^C123 didn't display '3' before as it should.

## [1.0b] - 2016-07-30 ##
- FIRST OFFICIAL VERSION OF SWIRC!
