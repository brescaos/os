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
| `REGISTRY_PORT` | `5050` | Port of the local registry (`publish-dev`, `registry-start`) |

## What the build does

[`scripts/image.sh`](../scripts/image.sh) copies the root `LICENSE` into
`image/` and builds
[`image/Containerfile`](../image/Containerfile) with `--pull`, so a newer
`quay.io/fedora/fedora-bootc` base rebuilds everything on top of it.

## Publishing

For development there is a local registry on `127.0.0.1:5050`, a container
from Docker's official `registry` image with its data in a volume:

```bash
make publish-dev VERSION=0.1.0   # starts the local registry if needed and pushes there
make registry-stop               # stops it; the data is kept
make registry-start              # starts it again
make clean                       # deletes it and its data (local brescaos:* images are kept)
```

## Troubleshooting

| Symptom | Try |
|---|---|
| `docker not found` | Install Docker or Podman, or set `CONTAINER_ENGINE` |
| `docker is installed but not running` | Start Docker Desktop or the daemon |
| `image brescaos:… not found` | Run `make image` with the same `VERSION` (and `ARCH`) first |
| `push to … failed` | Check that the repository exists and you can push to it |
| `the local registry doesn't answer` | Port 5050 may be in use: set `REGISTRY_PORT` |
| `build/system.sh: ... missing, check fedora-release-common's file list` | Fedora changed its release package: update `image/build/system.sh` |
