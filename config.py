# config.py


def can_build(env, platform):
    return (env["platform"] == "linuxbsd" or env["platform"] == "windows") and env["arch"] == "x86_64"


def configure(env):
    if "ffmpeg_path" not in env or not env["ffmpeg_path"]:
        raise RuntimeError("ffmpeg path not found")


def get_opts(platform):
    return [
        ("ffmpeg_path", "FFmpeg path", ""),
    ]


def get_doc_path():
    return "doc_classes"
