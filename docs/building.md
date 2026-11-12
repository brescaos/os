# Building the image

`make image` builds the BrescaOS image locally with Docker or Podman. You
need macOS or Linux, on arm64 or x86_64.

```bash
make image VERSION=0.1.0
```

The result is the local image `brescaos:<VERSION>-<ARCH>`.

## Variables

Defaults are in [`scripts/settings/settings.sh`](../scripts/settings/settings.sh).
Pass them on the command line, for example
`make image VERSION=0.1.0 CONTAINER_ENGINE=podman`.

| Variable | Default | Meaning |
|---|---|---|
| `VERSION` | (required) | BrescaOS version: `X.Y.Z` or `X.Y.Z-<alpha\|beta\|rc>.N` |
| `ARCH` | the host's | `aarch64` or `x86_64`. Any other than the host's runs emulated, which is slow |
| `FEDORA_VERSION` | `44` | Fedora release the image is based on |
| `IMAGE_NAME` | `brescaos` | Local image name |
| `CONTAINER_ENGINE` | `docker` | `docker` or `podman` |
| `NO_CACHE` | `0` | `1` builds without the layer cache, so packages come from today's repositories |
| `REGISTRY` | (required by `publish`) | Registry to push to: `host[:port][/path]`, e.g. `quay.io/<user>` |
| `REGISTRY_TLS` | (required by `publish`) | `1` if `REGISTRY` speaks HTTPS, `0` for plain HTTP |
| `REGISTRY_PORT` | `5050` | Port of the local registry (`publish-dev`, `registry-start`, `vm`) |

## What the build does

[`scripts/image.sh`](../scripts/image.sh) copies the root `LICENSE` into
`image/` and builds
[`image/Containerfile`](../image/Containerfile) with `--pull`, so a newer
`quay.io/fedora/fedora-bootc` base rebuilds everything on top of it.
With Docker, a warning from its build checks fails the build. A first stage,
on `quay.io/fedora/fedora`, compiles the translations of `image/src/`
([Translations](translations.md)); the image only takes its result.

## Publishing

For development there is a local registry on `127.0.0.1:5050`, a container
from Docker's official `registry` image with its data in a volume:

```bash
make publish-dev VERSION=0.1.0   # starts the local registry if needed and pushes there
make registry-stop               # stops it; the data is kept
make registry-start              # starts it again
make clean                       # deletes it and its data (local brescaos:* images are kept)
```

To try the image in a QEMU virtual machine, see [Development VM](vm.md).

## Tests

```bash
make test            # builds the image as 0.0.0 and runs every component's tests in it
make check-welcome   # compiles Welcome and runs its unit and QML tests, without the image (part of make check)
make check-autotile  # compiles autotiling and runs the tests of its KWin script and its state, without the image (part of make check)
make check-distrobox # compiles the distroboxes' terminal colors and runs their tests, without the image (part of make check)
make check-bresca-krunner # compiles the KRunner plugin and runs its tests, without the image (part of make check)
```

A component of `image/src/` has tests when it has `tests/Containerfile`:
[`scripts/test.sh`](../scripts/test.sh) builds that test image on top of
`brescaos:0.0.0-<ARCH>`, from the component's `tests/` folder, and runs it;
the image's command runs the tests and fails if one does. Each component's
README says what they cover. The CI runs them on both architectures. The
test images (`brescaos-tests-<component>:0.0.0-<ARCH>`) stay, like the
`brescaos:*` ones.

## Troubleshooting

| Symptom | Try |
|---|---|
| `docker not found` | Install Docker or Podman, or set `CONTAINER_ENGINE` |
| `docker is installed but not running` | Start Docker Desktop or the daemon |
| `image brescaos:… not found` | Run `make image` with the same `VERSION` (and `ARCH`) first |
| `push to … failed` | Check that the repository exists and you can push to it |
| `the local registry doesn't answer` | Port 5050 may be in use: set `REGISTRY_PORT` |
| `build/system.sh: ... missing, check fedora-release-common's file list` | Fedora changed its release package: update `image/build/system.sh` |
| `make check` reports a shfmt diff | Run `make format` |
| `make check`: `… is stale` or `… is out of sync with the code` | Run `make pot`, then translate what's new ([Translations](translations.md)) |
| `make check`: `… untranslated` or `… fuzzy` | Translate or review those strings in Poedit ([Translations](translations.md)) |
