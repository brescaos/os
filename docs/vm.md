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

1. Builds the image as `brescaos:0.0.0-<ARCH>`.
2. Builds the VM's image on it, `brescaos-dev:0.0.0-<ARCH>`
   ([`dev/vm/image/`](../dev/vm/image/)): BrescaOS plus what only the VM
   needs.
3. Starts the local registry on `127.0.0.1:5050` and pushes the VM's image
   there.
4. Installs it on a new disk, `.vm/disk.qcow2`. The VM follows it for its
   updates.
5. Boots the disk in a QEMU window. The boot log goes to `.vm/serial.log`.

It boots to Plasma's login screen: log in as `bresca`, password `bresca`.
This user is an administrator.

> [!WARNING]
> Each `make vm` replaces the disk, so the VM starts over. To boot the
> existing disk as you left it, use `make vm-start`.

## Updating the VM

To try a change to the image as an update, without a new disk:

```bash
make vm-update                                     # on the host: rebuilds both images, pushes the VM's
sudo bresca updates install --download --no-tls    # in the VM (--no-tls: the local registry is HTTP)
```

`make publish-dev` pushes the BrescaOS image itself, which the VM doesn't
follow.

## Shared clipboard

Text you copy on the host can be pasted in the VM, and the other way round.
It needs QEMU 8.1 or newer (`make vm-start` warns otherwise): QEMU's
`qemu-vdagent` talks to `spice-vdagent` in the VM's image. `spice-vdagent`
only reads X11's clipboard, so `clipboard-bridge`, started at login, copies
Plasma's (Wayland) clipboard to it. Text only.

## Screenshots

Press `Ctrl+Shift+S` in the VM to save the whole screen, including the pointer
and any menu, tooltip or dialog open at that moment, on the host as
`.vm/screenshots/<YYYY-MM-DD_HH-MM-SS>.png`. Spectacle takes the capture.
`make vm-clean` deletes the screenshots along with the rest of `.vm/`.

`make vm-start` creates the folder with mode `0703`. 9p keeps the host's
modes, and the VM's user, who counts as "others" there, needs `w` and `x` to
create files. It can't list the folder, and neither can other users on the
host.

## Hot reload

While the VM runs, what you change in `image/src/` reaches it within a second
(what's compiled, once it compiles), without rebuilding the image: today, the
whole `bresca` CLI (`image/src/bresca-cli/`), the updates applet and the
System Settings module.
See what it copies with:

```bash
journalctl -fu bresca-dev-hotreload   # in the VM
```

- After copying, a notification says which projects of `image/src/` it
  updated (e.g. `bresca-cli`) and how many files.
- What reloads is what `image/Containerfile` copies from `src/`, to the same
  places; a new `COPY` there reloads too.
- Translations reload too: when a `po/*.po` or a `*.in` template of
  `image/src/` changes, they're compiled again with
  `image/build/compile_translations.sh`, as the image's build does, and the result is
  copied (shown as `translations` in the notification). The CLI uses them on
  its next run; the applet's restart plasmashell. A `.po` that doesn't compile
  leaves the last ones in place, with the error in the log.
- What `image/Containerfile` compiles (today, System Settings > BrescaOS
  Updates, `image/src/updates-kcm/`, autotiling, `image/src/autotile/`, and
  Welcome, `image/src/welcome/`:
  every `image/src/` component with a `CMakeLists.txt`) is compiled again in the VM when its sources or its
  translations change, as the image's stage does, into
  `/var/cache/bresca-dev/build/<component>/`; it persists, so only what
  changed recompiles. A notification says it's compiling and stays until it
  finishes; if it doesn't compile, it says so, with the error in the log.
  A changed System Settings module, or its translations, closes System
  Settings if it's open and opens it again on that module.
- A changed systemd unit (a service, a timer…) is reloaded (`systemctl
  daemon-reload`); a service that is already running keeps the old one until
  it restarts. A changed Plasma applet, or a QML module it imports, restarts
  plasmashell. A changed KWin script is unloaded and KWin loads it again if
  it's on.
- `/usr` is only writable until the next boot (`bootc usr-overlay`): after a
  reboot the image's files come back and the changed ones are copied again.
  Files you delete on the host stay in the VM until then.
- Changes to anything else (packages, `image/build/`, the hot reload itself)
  need `make vm-update` or `make vm`.

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
| `QEMU_AUDIO_BACKEND` | `coreaudio` (macOS), `pa` (Linux) | Where the VM's sound plays on the host; also applies to `make vm-start` |
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
- Its image is `brescaos-dev`, BrescaOS plus `spice-vdagent`, `wl-clipboard`,
  `xclip`, `gettext` (for the hot reload of translations), `cmake`,
  `extra-cmake-modules`, `gcc-c++` and `kf6-kcmutils-devel` (for the hot
  reload of what's compiled), `spectacle` (for screenshots) and three services: the hot reload
  ([`hotreload.sh`](../dev/vm/image/hotreload.sh)), which mounts this
  repository read-only at `/run/bresca-dev/src` (QEMU's 9p), the clipboard
  bridge, and `bresca-dev-screenshots`, which mounts `.vm/screenshots/`
  writable at `/run/bresca-dev/screenshots`.
- Its sound card (Intel HDA) plays through the host's sound and records from
  the host's microphone (on macOS, QEMU asks for it the first time).
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
