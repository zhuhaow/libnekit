# Examples
## Prepare
Every example expect that `SUGAR_ROOT` environment variable is set:
```bash
> git clone https://github.com/ruslo/sugar
> export SUGAR_ROOT="`pwd`/sugar"
> cd sugar/examples/NN-example/
> rm -rf _builds
> cmake -H. -B_builds/variant
> cmake --build _builds/variant
```
Note that in general there is no need to set any environment variable, only include `Sugar` master file:
```cmake
include(/path/to/sugar/cmake/Sugar)
```

## Description
#### 00 (detect)
Create empty project and include [Sugar](https://github.com/ruslo/sugar/blob/master/cmake/Sugar)
master file, print updated variables.

#### 01 (simple)
Introduction to [collecting](https://github.com/ruslo/sugar/tree/master/cmake/collecting) system, used functions:
* [sugar_include](https://github.com/ruslo/sugar/tree/master/cmake/collecting#sugar_include)
* [sugar_files](https://github.com/ruslo/sugar/tree/master/cmake/collecting#sugar_files)

#### 02 (common)
Creating two targets with common sources, first fill sources variables, then create targets.

#### 03 (ios-gtest)
Wrapper for running gtest executable on iOS simulator, used function:
* [sugar_add_ios_gtest](https://github.com/ruslo/sugar/tree/master/cmake/core#sugar_add_ios_gtest)

#### 04 (gtest-universal)
If iOS build detected, use simulator to run gtest executable (see previous example),
otherwise use regular test system, used function:
* [sugar_add_gtest](https://github.com/ruslo/sugar/tree/master/cmake/core#sugar_add_gtest)

#### 05 (groups)
Generating groups for `Xcode` and `Visual Studio`, used function:
* [sugar_groups_generate](https://github.com/ruslo/sugar/tree/master/cmake/core#sugar_groups_generate)

#### 06 (ios)
Building ios application (`Xcode`)
 * `empty_application` (like `Xcode`: `iOS` -> `Application` -> `Empty Application`)
 * `single_view_application` (like `Xcode`: `iOS` -> `Application` -> `Single View Application`)
 * `_universal_library` build/install universal library (i386 + arm, iphoneos + iphonesimulator)
See [wiki](https://github.com/ruslo/sugar/wiki/Building-universal-ios-library) for detailed instructions.
 * `link_package` link universal library using `find_package` command.

#### 07 (cocoa)
Building macosx application (`Xcode`).

#### 08 (doxygen)
Example of adding doxygen generation target, used function:
* [sugar_doxygen_generate](https://github.com/ruslo/sugar/tree/master/cmake/core#sugar_doxygen_generate)

#### TODO: more...

*Note*: mark `used functions` show only first appearance of function in examples
## Run all
`test.py` script can run all examples using different generators (`Make`, `Xcode`, ...)
and configurations (`Debug`, `Release`). See this [wiki](https://github.com/ruslo/sugar/wiki/Examples-testing)
