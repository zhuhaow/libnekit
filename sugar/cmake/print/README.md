Wrappers for [message](http://www.cmake.org/cmake/help/v2.8.11/cmake.html#command:message) command to control output.
### sugar_status_print
Prints only if [SUGAR_STATUS_PRINT](https://github.com/ruslo/sugar/wiki/Used-variables) option is `TRUE` (default is `TRUE`)

### sugar_status_debug
Prints only if [SUGAR_STATUS_DEBUG](https://github.com/ruslo/sugar/wiki/Used-variables) option is `TRUE` (default is `FALSE`)

### sugar_fatal_error
[wrapper](https://github.com/ruslo/sugar/wiki/Coding-style#note-about-wrappers) for `message(FATAL_ERROR "...")`
