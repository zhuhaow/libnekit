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

        if self.settings.os not in ["iOS"]:
            self.requires("gtest/1.8.1@bincrafters/stable")


    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="libnekit")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="libnekit/include/")
        self.copy("*libnekit.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["libnekit"]
