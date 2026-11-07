#!/usr/bin/env bash
# Installs BrescaOS on the development VM's raw disk and sets it up.
# It runs in the privileged container create.sh starts.
#
# Environment: DISK_SIZE, SOURCE and TARGET (image refs), SERIAL (console),
#              VM_USER, VM_FULL_NAME, VM_PASSWORD, VM_KEYBOARD_LAYOUT,
#              VM_KEYBOARD_VARIANT, VM_TIMEZONE, VM_LANG, HOST_UID, HOST_GID.
set -euo pipefail

DISK=/var/tmp/disk.raw
TARGET_ROOT=/var/tmp/target

# The registry speaks plain HTTP. Only in this container; the disk gets
# its own setting below.
mkdir -p /etc/containers/registries.conf.d
printf '[[registry]]\nlocation = "%s"\ninsecure = true\n' "${SOURCE%%/*}" \
    >/etc/containers/registries.conf.d/99-brescaos-vm-install.conf

rm -f "$DISK"
truncate -s "$DISK_SIZE" "$DISK"
# --target-imgref: the image the VM updates from, as the VM sees the registry.
# --disable-selinux: bootc can't label files from a host without SELinux.
bootc install to-disk \
    --via-loopback \
    --generic-image \
    --filesystem ext4 \
    --source-imgref "docker://$SOURCE" \
    --target-imgref "$TARGET" \
    --skip-fetch-check \
    --disable-selinux \
    --karg console=tty0 \
    --karg "console=$SERIAL,115200" \
    "$DISK"

# --- Mount the new deployment ------------------------------------------------------
loop=$(losetup -f --show -P "$DISK")
cleanup() {
    umount -R "$TARGET_ROOT" 2>/dev/null || true
    losetup -d "$loop" 2>/dev/null || true
}
trap cleanup EXIT

# Partition nodes show up a moment after losetup -P.
tries=0
until compgen -G "${loop}p*" >/dev/null; do
    [ "$tries" -lt 20 ] || {
        echo "error: no partitions on $loop" >&2
        exit 1
    }
    sleep 0.5
    tries=$((tries + 1))
done
# bootc labels its root partition "root".
root=""
for partition in "$loop"p*; do
    if [ "$(blkid -o value -s LABEL "$partition" 2>/dev/null)" = root ]; then root=$partition; fi
done
[ -n "$root" ] || {
    echo "error: no partition labelled root on $loop" >&2
    exit 1
}

mkdir -p "$TARGET_ROOT"
mount "$root" "$TARGET_ROOT"
deploys=("$TARGET_ROOT"/ostree/deploy/default/deploy/*.0)
[ "${#deploys[@]}" -eq 1 ] && [ -d "${deploys[0]}" ] \
    || {
        echo "error: expected one deployment, found: ${deploys[*]}" >&2
        exit 1
    }
deploy=${deploys[0]}
etc=$deploy/etc
mount --bind "$TARGET_ROOT/ostree/deploy/default/var" "$deploy/var"
mount --bind /dev "$deploy/dev"
mount --bind /proc "$deploy/proc"
mount --bind /sys "$deploy/sys"

# --- User ---------------------------------------------------------------------
# Fedora bootc may keep base groups in /usr/lib/group (nss-altfiles);
# useradd -G only looks at /etc/group.
if ! grep -q '^wheel:' "$etc/group" && grep -q '^wheel:' "$deploy/usr/lib/group" 2>/dev/null; then
    grep '^wheel:' "$deploy/usr/lib/group" >>"$etc/group"
fi
mkdir -p "$deploy/var/home"
chmod 0755 "$deploy/var/home"
chroot "$deploy" useradd -m -G wheel -c "$VM_FULL_NAME" "$VM_USER"
printf '%s:%s\n' "$VM_USER" "$VM_PASSWORD" | chroot "$deploy" chpasswd

# --- Time zone, language, keyboard ------------------------------------------------
[ -f "$deploy/usr/share/zoneinfo/$VM_TIMEZONE" ] \
    || {
        echo "error: unknown time zone $VM_TIMEZONE (VM_TIMEZONE)" >&2
        exit 1
    }
ln -sfn "../usr/share/zoneinfo/$VM_TIMEZONE" "$etc/localtime"

# Only the locales the image's glibc-langpack-* packages bring, which live in
# /usr/lib/locale as e.g. en_US.utf8.
locale_name=${VM_LANG%%.*}
if [ -d "$deploy/usr/lib/locale/$locale_name.utf8" ]; then
    printf 'LANG=%s\n' "$locale_name.UTF-8" >"$etc/locale.conf"
else
    echo "warning: the image has no $locale_name locale: the VM keeps the image's language" >&2
fi

# The console keymap is named like the xkb layout, plus -<variant> (es-cat).
keymap=$VM_KEYBOARD_LAYOUT${VM_KEYBOARD_VARIANT:+-$VM_KEYBOARD_VARIANT}
if [ -n "$(find "$deploy/usr/lib/kbd/keymaps" -name "$keymap.map*" -print -quit 2>/dev/null)" ]; then
    printf 'KEYMAP=%s\n' "$keymap" >"$etc/vconsole.conf"
else
    echo "warning: no console keymap $keymap: the text console keeps the image's" >&2
fi

# --- Updates from the local registry ------------------------------------------------
# The VM follows TARGET, in the host's local registry, which speaks plain
# HTTP. Only on this VM's disk; the image itself allows nothing of the sort.
mkdir -p "$etc/containers/registries.conf.d"
printf '# Development VM: allow the local registry over plain HTTP.\n[[registry]]\nlocation = "%s"\ninsecure = true\n' \
    "${TARGET%%/*}" >"$etc/containers/registries.conf.d/50-brescaos-vm.conf"

# --- Hot reload ---------------------------------------------------------------------
# start.sh shares the repository read-only over 9p; hotreload.sh, run from
# there, copies what changes in image/src/ onto the running system. Only on
# this VM's disk. nofail: without the share the VM still boots.
printf '# Development VM: the repository, shared by dev/vm/start.sh.\nbresca-src /run/bresca-dev/src 9p trans=virtio,version=9p2000.L,ro,nofail 0 0\n' \
    >>"$etc/fstab"
cat >"$etc/systemd/system/bresca-dev-hotreload.service" <<'EOF'
# Development VM: hot reload of image/src/ (dev/vm/hotreload.sh).
[Unit]
Description=BrescaOS development VM: hot reload
RequiresMountsFor=/run/bresca-dev/src

[Service]
ExecStart=/usr/bin/bash /run/bresca-dev/src/dev/vm/hotreload.sh
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
mkdir -p "$etc/systemd/system/multi-user.target.wants"
ln -sfn /etc/systemd/system/bresca-dev-hotreload.service \
    "$etc/systemd/system/multi-user.target.wants/bresca-dev-hotreload.service"

cleanup
trap - EXIT

cp --sparse=always "$DISK" /io/disk.raw
rm -f "$DISK"
chown "$HOST_UID:$HOST_GID" /io/disk.raw
