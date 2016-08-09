# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
### Added ###
- Support for multiple encodings: UTF-8, ISO-8859-1 and ISO-8859-15.
- Command /list
- Event 321-323 (numeric replies for /list).
- Event 412 ERR_NOTEXTTOSEND

### Fixed ###
- As a fallback, instead of printing out "Printtext Internal Error",
  continue to convert characters even if some are lost due to an
  invalid multibyte sequence.
- Interpretation of color codes in the printtext module. For example:
  ^C123 didn't display '3' before as it should.

## [1.0b] - 2016-07-30 ##
- FIRST OFFICIAL VERSION OF SWIRC!
