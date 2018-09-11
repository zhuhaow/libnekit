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
import zipfile
from contextlib import contextmanager

from plumbum import FG, local
from plumbum.cmd import git, cmake
from clint.textui import progress
import requests

LIBRARIES = [
    ("google/googletest", "master", "googletest"),
    ("zhuhaow/libmaxminddb", "master", "libmaxminddb"),
]

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
            if (url.find(".zip") != -1):
                compre = zipfile.ZipFile(tempf)
            else:
                compre = tarfile.open(fileobj=tempf, mode="r")
            compre.extractall(tempd)
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
                  extra_config=[]):
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

        if target_platform == Platform.Windows:
            config.extend(["-A", "x64"])

        config.append("-DCMAKE_TOOLCHAIN_FILE={}".format(
            toolchain_path(target_platform)))

        if extra_config:
            config.extend(extra_config)

        cmake[config] & FG

        if Platform.current_platform() in [Platform.OSX, Platform.Linux]:
            cmake["--build", tempd, "--target", "install", "--config", "Release", "--", "-j4"] & FG
        else:
            cmake["--build", tempd, "--target", "install", "--config", "Release"] & FG



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

        if target_platform not in [Platform.iOS, Platform.Android]:
            # Compile GoogleTest
            cmake_compile(
                os.path.join(tempd, "googletest"),
                install_path(target_platform),
                target_platform,
                ['-Dgtest_force_shared_crt=ON'] if target_platform == Platform.Windows else []

            )


if __name__ == "__main__":
    main()
