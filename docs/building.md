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

## What the build does

[`scripts/image.sh`](../scripts/image.sh) copies the root `LICENSE` into
`image/` and builds
[`image/Containerfile`](../image/Containerfile) with `--pull`, so a newer
`quay.io/fedora/fedora-bootc` base rebuilds everything on top of it.

## Troubleshooting

| Symptom | Try |
|---|---|
| `docker not found` | Install Docker or Podman, or set `CONTAINER_ENGINE` |
| `docker is installed but not running` | Start Docker Desktop or the daemon |
| `build/system.sh: ... missing, check fedora-release-common's file list` | Fedora changed its release package: update `image/build/system.sh` |
