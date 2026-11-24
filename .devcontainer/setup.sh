#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config curl git unzip zip \
  libepoxy-dev libgl1-mesa-dev libsdl2-dev libqt5opengl5-dev qtbase5-dev \
  libpng-dev libzip-dev zlib1g-dev libavcodec-dev libavformat-dev libavutil-dev \
  libswresample-dev libswscale-dev libedit-dev libelf-dev

if command -v docker >/dev/null 2>&1; then
  docker pull devkitpro/devkitarm:latest
else
  echo "Docker is unavailable; use scripts/build-plugin.sh from a Docker host or install devkitARM locally." >&2
fi
