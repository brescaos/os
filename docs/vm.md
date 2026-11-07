# Development VM

`make vm` runs BrescaOS in a QEMU virtual machine, without an installer. It
is for development only: the VM's image is always version `0.0.0` and is only
pushed to the local registry.

## Requirements

On top of what [building](building.md) needs:

- QEMU (`qemu-system-aarch64` or `qemu-system-x86_64`, and `qemu-img`)
- UEFI firmware for QEMU (macOS: comes with `brew install qemu`; Fedora:
  `edk2-ovmf` or `edk2-aarch64`; Debian/Ubuntu: `ovmf` or `qemu-efi-aarch64`)
- About 20 GB free for the VM's disk

## Create and boot the VM

```bash
make vm
```

1. Starts the local registry on `127.0.0.1:5050`.
2. Builds the image as `brescaos:0.0.0-<ARCH>` and pushes it there.
3. Installs it on a new disk, `.vm/disk.qcow2`.
4. Boots the disk in a QEMU window. The boot log goes to `.vm/serial.log`.

Log in as `bresca`, password `bresca`. This user is an administrator.

> [!WARNING]
> Each `make vm` replaces the disk, so the VM starts over. To boot the
> existing disk as you left it, use `make vm-start`.

## Hot reload

While the VM runs, what you change in `image/src/` reaches it within a second,
without rebuilding the image: today, the whole `bresca` CLI
(`image/src/bresca-cli/`). See what it copies with:

```bash
journalctl -fu bresca-dev-hotreload   # in the VM
```

- What reloads is what `image/Containerfile` copies from `src/`, to the same
  places; a new `COPY` there reloads too.
- A changed systemd unit is reloaded (`systemctl daemon-reload`); a service
  that is already running keeps the old one until it restarts.
- `/usr` is only writable until the next boot (`bootc usr-overlay`): after a
  reboot the image's files come back and the changed ones are copied again.
  Files you delete on the host stay in the VM until then.
- Changes to anything else (packages, `image/build/`) still need `make vm`.
- A disk created before hot reload existed doesn't have it: `make vm` once.

## Variables

Defaults are in [`dev/vm/settings.sh`](../dev/vm/settings.sh). For example
`make vm VM_MEMORY=8192 VM_KEYBOARD_LAYOUT=es`.

| Variable | Default | Meaning |
|---|---|---|
| `ARCH` | the host's | `aarch64` or `x86_64`. Any other than the host's runs emulated, which is slow and may fail |
| `CONTAINER_ENGINE` | `docker` | `docker` or `podman` |
| `VM_MEMORY` / `VM_CPUS` | `4096` / `4` | Memory in MiB and CPUs; also apply to `make vm-start` |
| `DISK_SIZE` | `20G` | Size of the VM's disk |
| `VM_USER` / `VM_FULL_NAME` / `VM_PASSWORD` | `bresca` / `Bresca` / `bresca` | The administrator user |
| `VM_KEYBOARD_LAYOUT` / `VM_KEYBOARD_VARIANT` | `us` / (none) | Keyboard as xkb layout and variant, such as `es` / `cat` |
| `VM_TIMEZONE` | `Europe/Madrid` | Time zone |
| `VM_LANG` | `en_US.UTF-8` | Language as a `LANG` value; only the languages the image has (`en`, `es`, `ca`) |
| `QEMU_FIRMWARE` / `QEMU_FIRMWARE_VARS` | found automatically | UEFI firmware code and variables template |

## How it works

[`dev/vm/create.sh`](../dev/vm/create.sh) runs the image itself in a short-lived
privileged container (bootc needs loop devices; on macOS they are Docker
Desktop's VM's, not the Mac's). The container uses the registry container's
network, so it reads the image from `localhost:5000`.
[`dev/vm/install.sh`](../dev/vm/install.sh) runs `bootc install to-disk` on a
raw file in a container volume, then creates the user and sets the time zone,
language and keyboard. The host converts the raw file to `.vm/disk.qcow2`.

[`dev/vm/start.sh`](../dev/vm/start.sh) boots it with hardware acceleration where there is one (HVF
on macOS, KVM on Linux).

## How the VM differs from an installed system

- SELinux is disabled on its disk, because `bootc install` can't label files
  from a host without SELinux.
- It mounts this repository read-only at `/run/bresca-dev/src` (QEMU's
  9p, from `make vm-start` too) and runs `bresca-dev-hotreload.service`
  ([`dev/vm/hotreload.sh`](../dev/vm/hotreload.sh), straight from there):
  both in the disk's `/etc`, not in the image.
- It allows the local registry over plain HTTP, in
  `/etc/containers/registries.conf.d/50-brescaos-vm.conf`. The image itself
  doesn't.

## Remove it

```bash
make vm-clean   # delete .vm/ and the disk's work volume
```

The local registry and the `brescaos:*` images are kept: see `make clean` in
[building](building.md).

## Troubleshooting

When something fails, read the lines above the `error:` line: the real cause
from Docker or bootc is usually there.

| Symptom | Try |
|---|---|
| `qemu-system-… not found` | Install QEMU |
| `no UEFI firmware found` | Install the firmware package the error names, or set `QEMU_FIRMWARE` |
| `make vm` fails in `bootc install` with Podman | Use rootful Podman or Docker: the install needs a privileged container with loop devices |
| `unknown time zone` | Use a name from the tz database, such as `Europe/Madrid` |
| The VM hangs or fails at boot | Read `.vm/serial.log` |
