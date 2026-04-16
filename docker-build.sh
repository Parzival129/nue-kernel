#!/bin/sh
# Build and run the kernel inside Docker.
# Usage:
#   ./docker-build.sh          # build ISO only
#   ./docker-build.sh run      # build ISO and launch QEMU (curses mode)
#   ./docker-build.sh shell    # drop into a shell inside the container

set -e

IMAGE_NAME="nue-kernel-builder"

# Build the Docker image (cached after first run)
docker build -t "$IMAGE_NAME" .

case "${1:-build}" in
  build)
    docker run --rm -v "$(pwd)":/kernel "$IMAGE_NAME" \
      sh -c './iso.sh'
    echo "Done — nue_kernel.iso is ready."
    ;;
  run)
    docker run --rm -it -v "$(pwd)":/kernel "$IMAGE_NAME" \
      sh -c './iso.sh && qemu-system-i386 -cdrom nue_kernel.iso -display curses'
    ;;
  shell)
    docker run --rm -it -v "$(pwd)":/kernel "$IMAGE_NAME" bash
    ;;
  *)
    echo "Usage: $0 {build|run|shell}"
    exit 1
    ;;
esac
