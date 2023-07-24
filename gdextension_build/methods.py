import os
import sys
import functools
import re
import subprocess


def no_verbose(sys, env):
    colors = {}

    # Colors are disabled in non-TTY environments such as pipes. This means
    # that if output is redirected to a file, it will not contain color codes
    if sys.stdout.isatty():
        colors["blue"] = "\033[0;94m"
        colors["bold_blue"] = "\033[1;94m"
        colors["reset"] = "\033[0m"
    else:
        colors["blue"] = ""
        colors["bold_blue"] = ""
        colors["reset"] = ""

    # There is a space before "..." to ensure that source file names can be
    # Ctrl + clicked in the VS Code terminal.
    compile_source_message = "{}Compiling {}$SOURCE{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    java_compile_source_message = "{}Compiling {}$SOURCE{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    compile_shared_source_message = "{}Compiling shared {}$SOURCE{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    link_program_message = "{}Linking Program {}$TARGET{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    link_library_message = "{}Linking Static Library {}$TARGET{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    ranlib_library_message = "{}Ranlib Library {}$TARGET{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    link_shared_library_message = "{}Linking Shared Library {}$TARGET{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )
    java_library_message = "{}Creating Java Archive {}$TARGET{} ...{}".format(
        colors["blue"], colors["bold_blue"], colors["blue"], colors["reset"]
    )

    env.Append(CXXCOMSTR=[compile_source_message])
    env.Append(CCCOMSTR=[compile_source_message])
    env.Append(SHCCCOMSTR=[compile_shared_source_message])
    env.Append(SHCXXCOMSTR=[compile_shared_source_message])
    env.Append(ARCOMSTR=[link_library_message])
    env.Append(RANLIBCOMSTR=[ranlib_library_message])
    env.Append(SHLINKCOMSTR=[link_shared_library_message])
    env.Append(LINKCOMSTR=[link_program_message])
    env.Append(JARCOMSTR=[java_library_message])
    env.Append(JAVACCOMSTR=[java_compile_source_message])


def disable_warnings(self):
    # 'self' is the environment
    if self["platform"] == "windows" and not self["use_mingw"]:
        # We have to remove existing warning level defines before appending /w,
        # otherwise we get: "warning D9025 : overriding '/W3' with '/w'"
        warn_flags = ["/Wall", "/W4", "/W3", "/W2", "/W1", "/WX"]
        self.Append(CCFLAGS=["/w"])
        self.Append(CFLAGS=["/w"])
        self.Append(CXXFLAGS=["/w"])
        self["CCFLAGS"] = [x for x in self["CCFLAGS"] if not x in warn_flags]
        self["CFLAGS"] = [x for x in self["CFLAGS"] if not x in warn_flags]
        self["CXXFLAGS"] = [x for x in self["CXXFLAGS"] if not x in warn_flags]
    else:
        self.Append(CCFLAGS=["-w"])
        self.Append(CFLAGS=["-w"])
        self.Append(CXXFLAGS=["-w"])


def write_macos_plist(target, binary_name, identifier, name):
    os.makedirs(f"{target}/Resourece/", exist_ok=True)
    f = open(f"{target}/Resourece/Info.plist", "w")

    f.write(f'<?xml version="1.0" encoding="UTF-8"?>\n')
    f.write(f'<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n')
    f.write(f'<plist version="1.0">\n')
    f.write(f"<dict>\n")
    f.write(f"\t<key>CFBundleExecutable</key>\n")
    f.write(f"\t<string>{binary_name}</string>\n")
    f.write(f"\t<key>CFBundleIdentifier</key>\n")
    f.write(f"\t<string>{identifier}</string>\n")
    f.write(f"\t<key>CFBundleInfoDictionaryVersion</key>\n")
    f.write(f"\t<string>6.0</string>\n")
    f.write(f"\t<key>CFBundleName</key>\n")
    f.write(f"\t<string>{name}</string>\n")
    f.write(f"\t<key>CFBundlePackageType</key>\n")
    f.write(f"\t<string>FMWK</string>\n")
    f.write(f"\t<key>CFBundleShortVersionString</key>\n")
    f.write(f"\t<string>1.0.0</string>\n")
    f.write(f"\t<key>CFBundleSupportedPlatforms</key>\n")
    f.write(f"\t<array>\n")
    f.write(f"\t\t<string>MacOSX</string>\n")
    f.write(f"\t</array>\n")
    f.write(f"\t<key>CFBundleVersion</key>\n")
    f.write(f"\t<string>1.0.0</string>\n")
    f.write(f"\t<key>LSMinimumSystemVersion</key>\n")
    f.write(f"\t<string>10.14</string>\n")
    f.write(f"</dict>\n")
    f.write(f"</plist>\n")


def run_in_subprocess(builder_function):
    @functools.wraps(builder_function)
    def wrapper(target, source, env):
        # Convert SCons Node instances to absolute paths
        target = [node.srcnode().abspath for node in target]
        source = [node.srcnode().abspath for node in source]

        # Short circuit on non-Windows platforms, no need to run in subprocess
        if sys.platform not in ("win32", "cygwin"):
            return builder_function(target, source, env)

        # Identify module
        module_name = builder_function.__module__
        function_name = builder_function.__name__
        module_path = sys.modules[module_name].__file__
        if module_path.endswith(".pyc") or module_path.endswith(".pyo"):
            module_path = module_path[:-1]

        # Subprocess environment
        subprocess_env = os.environ.copy()
        subprocess_env["PYTHONPATH"] = os.pathsep.join([os.getcwd()] + sys.path)

        # Keep only JSON serializable environment items
        filtered_env = dict((key, value) for key, value in env.items() if isinstance(value, JSON_SERIALIZABLE_TYPES))

        # Save parameters
        args = (target, source, filtered_env)
        data = dict(fn=function_name, args=args)
        json_path = os.path.join(os.environ["TMP"], uuid.uuid4().hex + ".json")
        with open(json_path, "wt") as json_file:
            json.dump(data, json_file, indent=2)
        json_file_size = os.stat(json_path).st_size

        if env["verbose"]:
            print(
                "Executing builder function in subprocess: "
                "module_path=%r, parameter_file=%r, parameter_file_size=%r, target=%r, source=%r"
                % (module_path, json_path, json_file_size, target, source)
            )
        try:
            exit_code = subprocess.call([sys.executable, module_path, json_path], env=subprocess_env)
        finally:
            try:
                os.remove(json_path)
            except OSError as e:
                # Do not fail the entire build if it cannot delete a temporary file
                print(
                    "WARNING: Could not delete temporary file: path=%r; [%s] %s" % (json_path, e.__class__.__name__, e)
                )

        # Must succeed
        if exit_code:
            raise RuntimeError(
                "Failed to run builder function in subprocess: module_path=%r; data=%r" % (module_path, data)
            )

    return wrapper


def Run(env, function, short_message, subprocess=True):
    from SCons.Script import Action

    if not subprocess:
        return Action(function, "")
    else:
        return Action(run_in_subprocess(function), "")


def CommandNoCache(env, target, sources, command, **args):
    result = env.Command(target, sources, command, **args)
    env.NoCache(result)
    return result


def get_soname(filename):
    out = subprocess.check_output(["objdump", "-p", filename])
    result = re.search("^\s*SONAME\s+(.+)$", out.decode(), re.MULTILINE)
    if result:
        return result.group(1)
    else:
        return ""
