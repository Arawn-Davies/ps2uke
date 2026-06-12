#!/usr/bin/env bash
#
# Build the PlayStation 2 port of Duke Nukem 3D inside the ps2dev Docker
# toolchain (see Dockerfile). The PS2 port lives in ps2/ (the original icculus
# desktop/SDL source is in source/).
#
# Usage:
#   ./build.sh                  # build ps2/duke3d.elf  (default make target)
#   ./build.sh clean            # remove build artifacts
#   ./build.sh shell            # interactive shell inside the toolchain (cwd=ps2/)
#
# Raw make args still work directly, e.g.:  ./build.sh SHAREWARE=1
#
# Artifacts (objects in ps2/build/, the ELF as ps2/duke3d.elf) are owned by your
# host user, not root.
set -euo pipefail

IMAGE="ps2uke-dock:local"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build the local image (ps2dev + make/bash) on first use.
if ! docker image inspect "${IMAGE}" >/dev/null 2>&1; then
  echo ">> building ${IMAGE} ..."
  docker build -t "${IMAGE}" "${HERE}"
fi

# Mount the repo at /work and run in ps2/ as the host user.
common=(--rm -u "$(id -u):$(id -g)" -v "${HERE}:/work" -w /work/ps2)

if [[ "${1:-}" == "shell" ]]; then
  exec docker run -it "${common[@]}" "${IMAGE}" /bin/bash
fi

# Everything else is passed straight to make (default target = duke3d.elf).
exec docker run "${common[@]}" "${IMAGE}" make "$@"
