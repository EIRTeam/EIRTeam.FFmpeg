import os
import pathlib
import shutil
import urllib.request
import tarfile

FFMPEG_DOWNLOAD_WIN64 = "https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2023-06-27-12-50/ffmpeg-N-111281-g285c7f6f6b-win64-lgpl-shared.zip"
FFMPEG_DOWNLOAD_LINUX64 = "https://github.com/EIRTeam/FFmpeg-Builds/releases/download/autobuild-2023-07-23-11-40/ffmpeg-N-111601-g89f5124d0a-linux64-lgpl-godot.tar.xz"

def download_ffmpeg(target, source, env):
    dst = target[0]
    print("FFMPEG UPDATE")
    if os.path.exists(dst):
        shutil.rmtree(dst)

    if env["platform"] == "linuxbsd":
        FFMPEG_DOWNLOAD_URL = FFMPEG_DOWNLOAD_LINUX64
    else:
        FFMPEG_DOWNLOAD_URL = FFMPEG_DOWNLOAD_WIN64

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