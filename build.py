from conan.packager import ConanMultiPackager

if __name__ == "__main__":
    builder = ConanMultiPackager()
    builder.add_common_builds(pure_c=False)
    builds = []
    for settings, options, env_vars, build_requires, reference in builder.items:
        settings["cppstd"] = 14
        builds.append([settings, options, env_vars, build_requires])
    builder.builds = builds
    builder.run()
