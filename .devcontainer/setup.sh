#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config curl git unzip zip \
  libepoxy-dev libgl1-mesa-dev libsdl2-dev libqt5opengl5-dev qtbase5-dev \
  libpng-dev libzip-dev zlib1g-dev libavcodec-dev libavformat-dev libavutil-dev \
  libswresample-dev libswscale-dev libedit-dev libelf-dev \
  mgba-sdl xvfb xdotool xauth stella dosfstools mtools

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker CLI missing. Rebuild the Codespace so the docker-in-docker feature is installed." >&2
  exit 1
fi

for attempt in $(seq 1 30); do
  if docker info >/dev/null 2>&1; then
    break
  fi
  if [[ "$attempt" == 30 ]]; then
    echo "Docker daemon did not become ready after 30 seconds." >&2
    exit 1
  fi
  sleep 1
done

docker pull devkitpro/devkitarm:latest

for tool in cmake ninja /usr/games/mgba xvfb-run xdotool stella mkfs.vfat mcopy fsck.fat; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Required harness tool missing after setup: $tool" >&2
    exit 1
  fi
done
