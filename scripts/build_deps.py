#!/usr/bin/env python

LIBRARIES=[
    ('muflihun/easyloggingpp', 'v9.94.2', 'easyloggingpp'),
    ('google/googletest', 'release-1.8.0', 'googletest')
]

import os, tempfile, shutil, argparse, platform, errno, zipfile

from plumbum import FG, local
from plumbum.cmd import git, cmake, tar

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
        return target_platform in [Platform.OSX, Platform.iOS, Platform.Android]
    elif current_platform == Platform.Linux:
        return target_platform in [Platform.Linux, Platform.Android]
    elif current_platform == Platform.Windows:
        return target_platform in [Platform.Windows, Platform.Android]
    else:
        raise Exception("Compiling platform is unknown.")

def toolchain_path(target_platform):
    return os.path.abspath(os.path.join(source_dir, "cmake/toolchain/{}.cmake".format(target_platform)))

def install_path(target_platform):
    return os.path.join(install_dir, target_platform)

def download_repo(path, url, name, tag):
    git['-c', 'advice.detachedHead=false',
        'clone', url,
        '--branch', tag,
        '--depth', '1',
        '--single-branch',
        os.path.join(path, name)] & FG

def cmake_compile(source_dir, install_prefix, target_platform, extra_config=None):
    temp_dir = tempfile.mkdtemp()

    config = ["-H{}".format(source_dir),
              "-B{}".format(temp_dir),
              "-DCMAKE_BUILD_TYPE=Release",
              "-DCMAKE_INSTALL_PREFIX={}".format(install_prefix)]

    if target_platform in [Platform.iOS, Platform.OSX]:
        config.extend(["-GXcode",
                       "-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO",
                       "-DCMAKE_IOS_INSTALL_COMBINED=YES",
                       "-DIOS_DEPLOYMENT_SDK_VERSION=9.0"])

    config.append("-DCMAKE_TOOLCHAIN_FILE={}".format(toolchain_path(target_platform)))

    if extra_config:
        config.append(extra_config)

    cmake[config] & FG

    if target_platform in [Platform.iOS, Platform.OSX]:
        from plumbum.cmd import xcodebuild
        with local.cwd(os.path.join(temp_dir)):
            xcodebuild["-target", "install",
                       "-configuration", "Release"] & FG
    else:
        cmake["--build",
              temp_dir,
              "--", "install"] & FG

    shutil.rmtree(temp_dir, True)

def build_boost(boost_dir, install_prefix, target_platform):
    boost_module = "system"

    if target_platform == Platform.iOS:
        # The script will convert the space delimiter back to comma.
        with local.env(BOOST_SRC=boost_dir, BOOST_LIBS=boost_module.replace(",", " "), OUTPUT_DIR=install_path(target_platform)):
            with local.cwd(os.path.join(source_dir, "scripts")):
                local[local.cwd / "build_boost_ios.sh"] & FG

    elif target_platform in [Platform.OSX, Platform.Linux]:
        with local.cwd(boost_dir):
            temp_dir = tempfile.mkdtemp()

            local[os.path.join(boost_dir, "bootstrap.sh")][
                  "--prefix={}".format(install_prefix),
                  "--libdir={}".format(temp_dir),
                  "--with-libraries={}".format(boost_module)] & FG
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

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('platform_name', type=str, nargs='?', default=platform.system(), help="The target build platform. Mac, iOS, Android, Linux and Windows are supported.")
    args = parser.parse_args()

    target_platform = Platform.from_string(args.platform_name)
    assert check_platform(target_platform), "Can't compile dependency for target platform on current platform."

    tempdir = tempfile.mkdtemp()
    try:
        for library in LIBRARIES:
            download_repo(tempdir, "https://github.com/" + library[0] + ".git", library[2], library[1])

        # Extract boost source to build temp folder
        with local.cwd(tempdir):
            tar["xvpzf", os.path.join(source_dir, "boost.tar.gz")] & FG

        # Remove built binaries and headers.
        shutil.rmtree(install_path(target_platform), True)
        ensure_path_exist(install_path(target_platform))

        # Compile boost
        build_boost(os.path.join(tempdir, "boost"), install_path(target_platform), target_platform)

        # Compile easylogging++
        cmake_compile(os.path.join(tempdir, "easyloggingpp"), install_path(target_platform), target_platform, "-Dbuild_static_lib=ON")

        # Compile GoogleTest
        cmake_compile(os.path.join(tempdir, "googletest"), install_path(target_platform), target_platform)

    finally:
        shutil.rmtree(tempdir, True)

if __name__ == "__main__":
    main()
