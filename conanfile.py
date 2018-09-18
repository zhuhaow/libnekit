from conans import ConanFile, CMake, tools


class LibnekitConan(ConanFile):
    name = "libnekit"
    version = "0.0.1"
    license = "MIT"
    url = "https://github.com/zhuhaow/libnekit"
    description = "<Description of Libnekit here>"
    settings = "os", "compiler", "build_type", "arch"
    options = {}
    generators = "cmake"
    scm = {"type": "git", "subfolder": "libnekit", "url": "auto", "revision": "auto"}

    def requirements(self):
        self.requires("OpenSSL/1.1.0g@conan/stable")

        self.requires("libsodium/1.0.16@bincrafters/stable")

        self.requires("libmaxminddb/1.3.2@zhuhaow/stable")

        self.requires("boost/1.68.0@libnekit/stable")

        exclude_module = [
            "math",
            "wave",
            "container",
            "contract",
            "exception",
            "graph",
            "iostreams",
            "locale",
            "program_options",
            "random",
            "mpi",
            "serialization",
            "signals",
            "coroutine",
            "fiber",
            "context",
            "timer",
            "thread",
            "chrono",
            "date_time",
            "atomic",
            "filesystem",
            "graph_parallel",
            "stacktrace",
            "test",
            "type_erasure",
        ]
        for m in exclude_module:
            self.options["boost"].add_option("without_%s" % m, True)

        if tools.get_env("CONAN_RUN_TESTS", True):
            self.requires("gtest/1.8.1@bincrafters/stable")


    def _cmake(self):
        cmake = CMake(self)
        if tools.get_env("CONAN_RUN_TESTS", True):
            cmake.definitions["NE_ENABLE_TEST"]="ON"
        else:
            cmake.definitions["NE_ENABLE_TEST"]="OFF"
        return cmake

    def build(self):
        cmake = self._cmake()
        cmake.configure(source_folder="libnekit")
        cmake.build()
        if tools.get_env("CONAN_RUN_TESTS", True):
            with tools.environment_append({"CTEST_OUTPUT_ON_FAILURE": "1"}):
                cmake.test()

    def package(self):
        cmake = self._cmake();
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["nekit"]
