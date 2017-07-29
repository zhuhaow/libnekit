#!/usr/bin/env python

# MIT License
#
# Copyright (c) 2017 Zhuhao Wang
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import tempfile
import shutil
import argparse
import platform
import errno
import logging

from plumbum import FG, local
from plumbum.cmd import git, cmake

LIBRARIES = [('muflihun/easyloggingpp', 'v9.94.2', 'easyloggingpp'),
             ('google/googletest', 'release-1.8.0',
              'googletest'), ('openssl/openssl', 'OpenSSL_1_1_0f', 'openssl'),
             ('boostorg/boost', 'boost-1.64.0', 'boost')]

source_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
install_dir = os.path.abspath(os.path.join(source_dir, "deps"))


def ensure_path_exist(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise


class Platform:
    iOS = "ios"
    OSX = "mac"
    Android = "android"
    Linux = "linux"
    Windows = "win"

    @staticmethod
    def from_string(platform_name):
        platform_name = platform_name.lower()

        if platform_name in ["darwin", "mac", "osx"]:
            return Platform.OSX
        elif platform_name == "ios":
            return Platform.iOS
        elif platform_name == "android":
            return Platform.Android
        elif platform_name == "linux":
            return Platform.Linux
        elif platform_name == "windows":
            return Platform.Windows
        else:
            raise Exception("Unknown platform.")

    @staticmethod
    def current_platform():
        return Platform.from_string(platform.system())


def check_platform(target_platform):
    current_platform = Platform.current_platform()

    if current_platform == Platform.OSX:
        return target_platform in [
            Platform.OSX, Platform.iOS, Platform.Android
        ]
    elif current_platform == Platform.Linux:
        return target_platform in [Platform.Linux, Platform.Android]
    elif current_platform == Platform.Windows:
        return target_platform in [Platform.Windows, Platform.Android]
    else:
        raise Exception("Compiling platform is unknown.")


def toolchain_path(target_platform):
    return os.path.abspath(
        os.path.join(source_dir, "cmake/toolchain/{}.cmake".format(
            target_platform)))


def install_path(target_platform):
    return os.path.join(install_dir, target_platform)


def download_repo(path, url, name, tag):
    git['-c', 'advice.detachedHead=false', 'clone', url, '--branch', tag,
        '--depth', '1', '--recurse-submodules',
        os.path.join(path, name)] & FG


def cmake_compile(source_dir,
                  install_prefix,
                  target_platform,
                  extra_config=None):
    temp_dir = tempfile.mkdtemp()

    config = [
        "-H{}".format(source_dir), "-B{}".format(temp_dir),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX={}".format(install_prefix)
    ]

    if target_platform in [Platform.iOS, Platform.OSX]:
        config.extend([
            "-GXcode", "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO",
            "-DCMAKE_IOS_INSTALL_COMBINED=YES",
            "-DIOS_DEPLOYMENT_SDK_VERSION=9.0"
        ])

    config.append(
        "-DCMAKE_TOOLCHAIN_FILE={}".format(toolchain_path(target_platform)))

    if extra_config:
        config.append(extra_config)

    cmake[config] & FG

    if target_platform in [Platform.iOS, Platform.OSX]:
        from plumbum.cmd import xcodebuild
        with local.cwd(os.path.join(temp_dir)):
            xcodebuild["-target", "install", "-configuration", "Release"] & FG
    else:
        cmake["--build", temp_dir, "--", "install"] & FG

    shutil.rmtree(temp_dir, True)


def build_boost(boost_dir, install_prefix, target_platform):
    boost_build_module = "system"
    boost_module = "core,boost/asio.hpp,system"

    if Platform.current_platform() in [Platform.OSX, Platform.Linux]:
        with local.cwd(boost_dir):
            # build bcp first
            local[os.path.join(boost_dir, "bootstrap.sh")] & FG
            local[os.path.join(boost_dir, "b2")]["headers"] & FG
            local[os.path.join(boost_dir, "b2")]["tools/bcp"] & FG
            shutil.copy(os.path.join(boost_dir, "dist/bin/bcp"), boost_dir)
            # copy headers
            args = boost_module.split(',')
            args.append(os.path.join(install_prefix, "include"))
            ensure_path_exist(os.path.join(install_prefix, "include"))
            local[os.path.join(boost_dir, "bcp")][args] & FG

    if target_platform == Platform.iOS:
        # The script will convert the space delimiter back to comma.
        with local.env(
                BOOST_SRC=boost_dir,
                BOOST_LIBS=boost_build_module.replace(",", " "),
                OUTPUT_DIR=install_prefix):
            with local.cwd(os.path.join(source_dir, "scripts")):
                local[local.cwd / "build_boost_ios.sh"] & FG

    elif target_platform in [Platform.OSX, Platform.Linux]:
        with local.cwd(boost_dir):
            temp_dir = tempfile.mkdtemp()

            local[os.path.join(boost_dir, "bootstrap.sh")]["--prefix={}".format(
                os.path.join(temp_dir, "boost_tmp")), "--libdir={}".format(
                    temp_dir), "--with-libraries={}".format(
                        boost_build_module)] & FG
            local[os.path.join(boost_dir, "b2")]["install"] & FG

            # Linking all modules into one binary file
            with local.cwd(temp_dir):
                lib_dir = local.path(temp_dir)
                libs = lib_dir // "libboost_*.a"
                for lib in libs:
                    local["ar"]["-x", lib] & FG
                object_files = lib_dir // "*.o"
                local["ar"]["crus", "libboost.a", " ".join(object_files)] & FG

                lib_install_dir = os.path.join(install_prefix, "lib")
                ensure_path_exist(lib_install_dir)
                shutil.copy("libboost.a", lib_install_dir)

            shutil.rmtree(temp_dir, True)


def build_openssl(openssl_dir, install_prefix, target_platform):
    if target_platform == Platform.iOS:
        with local.env(
                CC="clang",
                CROSS_TOP=
                "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer",
                CROSS_SDK="iPhoneOS.sdk"):
            local.env.path.insert(
                0,
                "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"
            )
            with local.cwd(openssl_dir):
                local[local.cwd /
                      "Configure"]["ios64-cross", "no-shared", "no-dso",
                                   "no-hw", "no-engine", "--prefix={}".format(
                                       install_prefix)] & FG
                local["make"]["install_sw"] & FG

    elif target_platform in [Platform.OSX]:
        with local.cwd(openssl_dir):
            local[local.cwd /
                  "Configure"]["darwin64-x86_64-cc", "no-shared",
                               "enable-ec_nistp_64_gcc_128", "no-comp",
                               "--prefix={}".format(install_prefix)] & FG
            local["make"]["install_sw"] & FG

    elif target_platform in [Platform.Linux]:
        with local.cwd(openssl_dir):
            local[local.cwd /
                  "Configure"]["linux-x86_64", "no-shared",
                               "enable-ec_nistp_64_gcc_128", "no-comp",
                               "--prefix={}".format(install_prefix)] & FG
            local["make"]["install_sw"] & FG


def main():
    logging.getLogger().setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        'platform_name',
        type=str,
        nargs='?',
        default=platform.system(),
        help="The target build platform. Mac, iOS, \
        Android, Linux and Windows are supported.")
    args = parser.parse_args()

    target_platform = Platform.from_string(args.platform_name)
    assert check_platform(
        target_platform
    ), "Can't compile dependency for target platform on current platform."

    tempdir = tempfile.mkdtemp()
    try:
        for library in LIBRARIES:
            download_repo(tempdir, "https://github.com/" + library[0] + ".git",
                          library[2], library[1])

        # Remove built binaries and headers.
        shutil.rmtree(install_path(target_platform), True)
        ensure_path_exist(install_path(target_platform))

        # Compile boost
        build_boost(
            os.path.join(tempdir, "boost"),
            install_path(target_platform), target_platform)

        build_openssl(
            os.path.join(tempdir, "openssl"),
            install_path(target_platform), target_platform)

        # Compile easylogging++
        cmake_compile(
            os.path.join(tempdir, "easyloggingpp"),
            install_path(target_platform), target_platform,
            "-Dbuild_static_lib=ON")

        # Compile GoogleTest
        cmake_compile(
            os.path.join(tempdir, "googletest"),
            install_path(target_platform), target_platform)

    finally:
        shutil.rmtree(tempdir, True)


if __name__ == "__main__":
    main()
