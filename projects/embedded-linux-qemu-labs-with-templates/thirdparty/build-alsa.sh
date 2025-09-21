#!/bin/bash
set -e
CROSS_PREFIX=arm-linux-
STAGING=$PWD/staging
TARGET=$PWD/target

mkdir -p $STAGING $TARGET

echo "Download ALSA sources (manual step). Example commands:"
echo "  wget https://www.alsa-project.org/files/pub/lib/alsa-lib-1.2.13.tar.bz2"
echo "  wget https://www.alsa-project.org/files/pub/utils/alsa-utils-1.2.13.tar.bz2"
