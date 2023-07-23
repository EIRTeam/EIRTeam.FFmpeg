# config.py


def can_build(env, platform):
    return (env["platform"] == "linuxbsd" or env["platform"] == "windows") and env["arch"] == "x86_64"


def configure(env):
    pass


def get_doc_path():
    return "doc_classes"
