# sugar
| linux                           | macosx                          |
|---------------------------------|---------------------------------|
| [![Build Status][master]][repo] | [![Build Status][macosx]][repo] |

[master]: https://travis-ci.org/ruslo/sugar.png?branch=master
[macosx]: https://travis-ci.org/ruslo/sugar.png?branch=travis.macosx
[repo]: https://travis-ci.org/ruslo/sugar

### Usage
All you need to do is to include master [Sugar](https://github.com/ruslo/sugar/tree/master/cmake) file:
```cmake
include(/path/to/sugar/cmake/Sugar)
```

*Note*: to run commands which use [python](https://github.com/ruslo/sugar/tree/master/python) you need to install `python3`

### Usage (hunter)
Using [hunter](http://github.com/ruslo/hunter) package manager:
```cmake
include(HunterGate.cmake)
hunter_add_package(Sugar)
include(${SUGAR_ROOT}/cmake/Sugar)
```

### Features
* [Collecting sources](https://github.com/ruslo/sugar/wiki/Collecting-sources)
* [Generating groups](https://github.com/ruslo/sugar/wiki/Generating-groups)
* ~~Build universal iOS library~~ (see CMake ios-universal
[patch](https://github.com/ruslo/CMake/releases) and [wiki](https://github.com/ruslo/sugar/wiki/Building-universal-ios-library))
* [Run gtest on iOS simulator](https://github.com/ruslo/sugar/tree/master/cmake/core#sugar_add_ios_gtest)
* [Cross-platform warning suppression](https://github.com/ruslo/sugar/wiki/Cross-platform-warning-suppression)

### Examples
See [examples](https://github.com/ruslo/sugar/tree/master/examples).
Please [read](https://github.com/ruslo/0/wiki/CMake) coding style and
agreements before start looking through examples (may explain a lot).

### What's next?
* [CMake toolchain collection](https://github.com/ruslo/polly)
* [CMake package manager](https://github.com/ruslo/hunter)
