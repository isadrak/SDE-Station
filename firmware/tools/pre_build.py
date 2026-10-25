# Script PlatformIO (extra_scripts) : régénère src/web/assets.h depuis app/
# avant chaque compilation, pour que le firmware embarque toujours la page à jour.
import subprocess
from os.path import join

Import("env")

subprocess.check_call(["node", join(env["PROJECT_DIR"], "tools", "gen_assets.js")])
