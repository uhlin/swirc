# Change Log #
All notable changes to this project will be documented in this file.

## [Unreleased] ##
### Added ###
- Support for multiple encodings: UTF-8, ISO-8859-1 and ISO-8859-15.
- Command /list
- Event 321-323 (numeric replies for /list).

### Fixed ###
- As a fallback, instead of printing out "Printtext Internal Error",
  continue to convert characters even if some are lost due to an
  invalid multibyte sequence.

## [1.0b] - 2016-07-30 ##
- FIRST OFFICIAL VERSION OF SWIRC!
