#!/usr/bin/env bash
# Saves the whole screen, pointer and popups included, into the host's
# .vm/screenshots/, named after the date and time. In the VM's image as
# /usr/libexec/bresca-dev/screenshot, run on Ctrl+Shift+S
# (bresca-dev-screenshot.desktop). bresca-dev-screenshots.service mounts the
# share that dev/vm/start.sh gives.
set -euo pipefail

exec spectacle --background --nonotify --fullscreen --pointer \
    --output "/run/bresca-dev/screenshots/$(date +%Y-%m-%d_%H-%M-%S).png"
