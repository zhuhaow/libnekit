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
import tarfile
from contextlib import contextmanager

from plumbum import FG, local
from plumbum.cmd import git, cmake
from clint.textui import progress
import requests

LIBRARIES = [
    ("google/googletest", "release-1.8.0", "googletest"),
    ("jedisct1/libsodium", "1.0.16", "libsodium"),
    ("zhuhaow/libmaxminddb", "master", "libmaxminddb"),
]

OPENSSL_IOS = ("x2on/OpenSSL-for-iPhone", "master", "openssl")
OPENSSL_LIB = ("openssl/openssl", "OpenSSL_1_1_0h", "openssl")

DOWNLOAD_LIBRARIES = [(
    "https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz",
    "boost",
    "boost_1_67_0",
)]

source_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
install_dir = os.path.abspath(os.path.join(source_dir, "deps"))


def ensure_path_exist(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise


def copytree(src, dst, symlinks=False, ignore=None):
    if not os.path.exists(dst):
        os.makedirs(dst)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            copytree(s, d, symlinks, ignore)
        else:
            if not os.path.exists(
                    d) or os.stat(s).st_mtime - os.stat(d).st_mtime > 1:
                shutil.copy2(s, d)


@contextmanager
def temp_dir():
    tempd = tempfile.mkdtemp()
    yield tempd
    shutil.rmtree(tempd, True)


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
        os.path.join(source_dir,
                     "cmake/toolchain/{}.cmake".format(target_platform)))


def install_path(target_platform):
    return os.path.join(install_dir, target_platform)


def download_repo(path, url, name, tag):
    git["-c", "advice.detachedHead=false", "clone", url, "--branch", tag,
        "--depth", "1", "--recurse-submodules",
        os.path.join(path, name), ] & FG


def download_library(path, url, name, content_dir):
    logging.info("Downloading %s from %s", name, url)

    with tempfile.TemporaryFile() as tempf:
        r = requests.get(url, stream=True)
        totol_length = int(r.headers.get("content-length"))
        for chunk in progress.bar(
                r.iter_content(chunk_size=1024 * 1024),
                expected_size=(totol_length / 1024 / 1024) + 1,
        ):
            if chunk:
                tempf.write(chunk)
                tempf.flush()
        logging.info("Downloaded file")
        with temp_dir() as tempd:
            logging.info("Extracting to %s", tempd)
            tempf.seek(0)
            with tarfile.open(fileobj=tempf, mode="r:gz") as tar:
                tar.extractall(tempd)
                logging.info(
                    "Moving file from %s to %s",
                    os.path.join(tempd, content_dir),
                    os.path.join(path, name),
                )
                shutil.move(
                    os.path.join(tempd, content_dir), os.path.join(path, name))


def mac_sdk_path():
    return local["xcrun"]["--sdk", "macosx", "--show-sdk-path"]()[:-1]


def cmake_compile(source_dir,
                  install_prefix,
                  target_platform,
                  extra_config=None):
    with temp_dir() as tempd:
        config = [
            "-H{}".format(source_dir),
            "-B{}".format(tempd),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_INSTALL_PREFIX={}".format(install_prefix),
        ]

        if target_platform in [Platform.iOS]:
            config.extend([
                "-DIOS_DEPLOYMENT_TARGET=9.0",
            ])

        config.append("-DCMAKE_TOOLCHAIN_FILE={}".format(
            toolchain_path(target_platform)))

        if extra_config:
            config.append(extra_config)

        cmake[config] & FG

        cmake["--build", tempd, "--", "-j4", "install"] & FG


def build_boost(boost_dir, install_prefix, target_platform):
    boost_build_module = "log,system,thread"
    boost_module = (
        "archive,core,boost/asio.hpp,system,log,phoenix,endian,range,assert,pool,thread"
    )

    if Platform.current_platform() in [Platform.OSX, Platform.Linux]:
        with local.cwd(boost_dir):
            # build bcp first
            local[os.path.join(boost_dir, "bootstrap.sh")] & FG
            local[os.path.join(boost_dir, "b2")]["-j4", "tools/bcp"] & FG
            shutil.copy(os.path.join(boost_dir, "dist/bin/bcp"), boost_dir)
            # copy headers
            args = boost_module.split(",")
            args.append(os.path.join(install_prefix, "include"))
            ensure_path_exist(os.path.join(install_prefix, "include"))
            local[os.path.join(boost_dir, "bcp")][args] & FG

    if target_platform == Platform.iOS:
        # The script will convert the space delimiter back to comma.
        with local.env(
                BOOST_SRC=boost_dir,
                BOOST_LIBS=boost_build_module.replace(",", " "),
                OUTPUT_DIR=install_prefix,
        ):
            with local.cwd(os.path.join(source_dir, "scripts")):
                local[local.cwd / "build_boost_ios.sh"] & FG

    elif target_platform in [Platform.OSX, Platform.Linux]:
        with local.cwd(boost_dir):
            with temp_dir() as tempd:
                local[os.path.join(
                    boost_dir, "bootstrap.sh")]["--with-libraries={}".format(
                        boost_build_module)] & FG

                args = [
                    "--stagedir={}".format(os.path.join(tempd, "boost_tmp")),
                    "link=static",
                    "variant=release",
                ]
                if target_platform == Platform.OSX:
                    args.extend([
                        "cxxflags=-isysroot {} -mmacosx-version-min=10.10 -fvisibility=hidden -fvisibility-inlines-hidden -fembed-bitcode".
                        format(mac_sdk_path()),
                        "linkflags=-isysroot {} -mmacosx-version-min=10.10 -fvisibility=hidden -fvisibility-inlines-hidden -fembed-bitcode".
                        format(mac_sdk_path()),
                    ])
                args.extend(["-j4", "stage"])

                local[os.path.join(boost_dir, "b2")][args] & FG

                # Linking all modules into one binary file
                with local.cwd(os.path.join(tempd, "boost_tmp", "lib")):
                    lib_dir = local.path(
                        os.path.join(tempd, "boost_tmp", "lib"))
                    libs = lib_dir // "libboost_*.a"
                    for lib in libs:
                        local["ar"]["-x", lib] & FG
                    object_files = lib_dir // "*.o"
                    for ofile in object_files:
                        local["ar"]["crus", "libboost.a", ofile] & FG

                    lib_install_dir = os.path.join(install_prefix, "lib")
                    ensure_path_exist(lib_install_dir)
                    shutil.copy("libboost.a", lib_install_dir)


def build_openssl(openssl_dir, install_prefix, target_platform):
    if target_platform == Platform.iOS:
        with local.cwd(openssl_dir):
            local[local.cwd / "build-libssl.sh"][
                "--version=1.1.0f", "--verbose-on-error",
                "--ec-nistp-64-gcc-128",
                "--targets=ios64-cross-arm64",
            ] & FG
            copytree("lib", os.path.join(install_prefix, "lib"))
            copytree("include", os.path.join(install_prefix, "include"))

    elif target_platform in [Platform.OSX]:
        with local.cwd(openssl_dir):
            local[local.cwd /
                  "Configure"]["darwin64-x86_64-cc", "no-shared",
                               "enable-ec_nistp_64_gcc_128", "no-comp",
                               "--prefix={}".format(install_prefix), ] & FG
            local["sed"][
                "-ie",
                "s!^CFLAGS=!CFLAGS=-isysroot {} -mmacosx-version-min=10.10 !".
                format(mac_sdk_path()), "Makefile",
            ] & FG
            local["make"]["install_sw"] & FG

    elif target_platform in [Platform.Linux]:
        with local.cwd(openssl_dir):
            local[local.cwd /
                  "Configure"]["linux-x86_64", "no-shared",
                               "enable-ec_nistp_64_gcc_128", "no-comp",
                               "--prefix={}".format(install_prefix), ] & FG
            local["make"]["install_sw"] & FG


def build_libsodium(libsodium_dir, install_prefix, target_platform):
    with local.cwd(libsodium_dir):
        with local.env(PREFIX=install_prefix):
            local["autoreconf"]["-i"] & FG

            if target_platform == Platform.iOS:
                local["sed"]["-i", "''", "s/^export PREFIX.*$//g", "dist-build/ios.sh"] & FG
                local["sed"]["-i", "''", "s/^rm -fr --.*$//g", "dist-build/ios.sh"] & FG
                local["sed"]["-i", "''", "s/\$IOS32_PREFIX\/include/\$IOS64_PREFIX\/include/g", "dist-build/ios.sh"] & FG
                (local["awk"]["BEGIN{f=1};/Build for the simulator/{f=0};/Build for iOS/{f=1}; /32-bit iOS/{f=0};/64-bit iOS/{f=1};/IOS32/{next};/SIMULATOR/{next};f"]["dist-build/ios.sh"] | local["sponge"]["dist-build/ios.sh"])()
                local["cat"]["dist-build/ios.sh"] & FG
                local["dist-build/ios.sh"] & FG
            elif target_platform == Platform.OSX:
                local["sed"]["-i", "''", "s/^export PREFIX.*$//g", "dist-build/osx.sh"] & FG
                local["cat"]["dist-build/osx.sh"] & FG
                local["dist-build/osx.sh"] & FG
            elif target_platform == Platform.Linux:
                local["./configure"]["--prefix={}".format(install_prefix)] & FG
                local["make"] & FG
                local["make"]["install"] & FG


def main():
    logging.getLogger().setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "platform_name",
        type=str,
        nargs="?",
        default=platform.system(),
        help="The target build platform. Mac, iOS, \
        Android, Linux and Windows are supported.",
    )
    args = parser.parse_args()

    target_platform = Platform.from_string(args.platform_name)
    assert check_platform(
        target_platform
    ), "Can't compile dependency for target platform on current platform."

    with temp_dir() as tempd:
        if target_platform == Platform.iOS:
            LIBRARIES.append(OPENSSL_IOS)
        else:
            LIBRARIES.append(OPENSSL_LIB)

        for library in LIBRARIES:
            download_repo(
                tempd,
                "https://github.com/" + library[0] + ".git",
                library[2],
                library[1],
            )

        # Remove built binaries and headers.
        shutil.rmtree(install_path(target_platform), True)
        ensure_path_exist(install_path(target_platform))

        cmake_compile(
            os.path.join(tempd, "libmaxminddb"),
            install_path(target_platform),
            target_platform,
        )

        build_libsodium(
            os.path.join(tempd, "libsodium"),
            install_path(target_platform),
            target_platform,
        )

        build_openssl(
            os.path.join(tempd, "openssl"),
            install_path(target_platform),
            target_platform,
        )

        for library in DOWNLOAD_LIBRARIES:
            download_library(tempd, library[0], library[1], library[2])

        # Compile boost
        build_boost(
            os.path.join(tempd, "boost"), install_path(target_platform),
            target_platform)

        if target_platform not in [Platform.iOS, Platform.Android]:
            # Compile GoogleTest
            cmake_compile(
                os.path.join(tempd, "googletest"),
                install_path(target_platform),
                target_platform,
            )


if __name__ == "__main__":
    main()
