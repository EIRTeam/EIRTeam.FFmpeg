import os
import pathlib
import shutil
import urllib.request
import tarfile

FFMPEG_DOWNLOAD_WIN64 = "https://github.com/EIRTeam/FFmpeg-Builds/releases/download/autobuild-2023-07-24-08-52/ffmpeg-N-111611-g5b11ee9429-win64-lgpl-godot.tar.xz"
FFMPEG_DOWNLOAD_LINUX64 = "https://github.com/EIRTeam/FFmpeg-Builds/releases/download/autobuild-2023-07-24-08-52/ffmpeg-N-111611-g5b11ee9429-linux64-lgpl-godot.tar.xz"


def get_download_url(env):
    print(env["platform"])
    if env["platform"] == "linuxbsd" or env["platform"] == "linux":
        FFMPEG_DOWNLOAD_URL = FFMPEG_DOWNLOAD_LINUX64
    else:
        FFMPEG_DOWNLOAD_URL = FFMPEG_DOWNLOAD_WIN64
    return FFMPEG_DOWNLOAD_URL


def download_ffmpeg(target, source, env):
    dst = os.path.dirname(target[0])
    if os.path.exists(dst):
        shutil.rmtree(dst)

    FFMPEG_DOWNLOAD_URL = get_download_url(env)

    local_filename, headers = urllib.request.urlretrieve(FFMPEG_DOWNLOAD_URL)

    def rewrite_subfolder_paths(tf, common_path):
        l = len(common_path)
        for member in tf.getmembers():
            if member.path.startswith(common_path):
                member.path = member.path[l:]
                yield member

    with tarfile.open(local_filename, mode="r") as f:
        # Get the first folder
        common_path = os.path.commonpath(f.getnames()) + "/"
        f.extractall(dst, members=rewrite_subfolder_paths(f, common_path))
        os.remove(local_filename)
