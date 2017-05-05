Main directory of `cmake` modules.

### Manual add
Modules can be included manually:
```cmake
set(SUGAR_ROOT "/path/to/sugar/root/directory")
list(APPEND CMAKE_MODULE_PATH "${SUGAR_ROOT}/cmake/core")

include(sugar_foo) # sugar_foo.cmake searched in core directory
```

*Note*:
* in this case you may get an error that some modules not found because modules from
different directories can include each other. For example module from `core` can include module from `print`
* some functions expect that `SUGAR_ROOT` variable is defined

### Master file
Modules can be included automatically by master file [Sugar](https://github.com/ruslo/sugar/blob/master/cmake/Sugar):
```cmake
# assuming that SUGAR_ROOT environment variable is set
include($ENV{SUGAR_ROOT}/cmake/Sugar)

include(sugar_foo) # sugar_foo.cmake searched in all directories: core, print, utility, ...
```
in this case
* all directories (core, utility, print, ...) will be added to [CMAKE_MODULE_PATH](http://www.cmake.org/cmake/help/v2.8.11/cmake.html#variable:CMAKE_MODULE_PATH)
* `SUGAR_ROOT` *cmake* variable will be setted
* `SUGAR_ROOT` variable will be printed using `message` command:
<pre>
-- The C compiler identification is Clang 5.0.0
-- The CXX compiler identification is Clang 5.0.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++
-- Check for working CXX compiler: /usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
[sugar] SUGAR_ROOT: /.../sugar
-- Configuring done
-- Generating done
-- Build files have been written to: /.../sugar/examples/00-detect/_builds/make-default
</pre>
