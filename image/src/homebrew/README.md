# Homebrew

`brew`, [Homebrew](https://brew.sh) on demand.

## Layout

| Source | In the image |
| --- | --- |
| `brew` | `/usr/bin/brew` |
| `homebrew.sh` | `/etc/profile.d/homebrew.sh` |
| `userdel` | `/usr/libexec/bresca/homebrew/userdel` |
| `accounts-daemon.conf` | `/usr/lib/systemd/system/accounts-daemon.service.d/50-brescaos-homebrew.conf` |
| `homebrew.tmpfiles` | `/usr/lib/tmpfiles.d/homebrew.conf` |
| `po/<lang>.po` | `/usr/share/locale/<lang>/LC_MESSAGES/homebrew.mo` |

`brew` prints its messages with bresca-cli's `lib/messages.sh`
(`/usr/libexec/bresca/lib/messages.sh`) and translates them in its own
domain, `homebrew` ([`docs/translations.md`](../../../docs/translations.md)).

## How it works

BrescaOS doesn't ship [Homebrew](https://brew.sh), but `brew` is there:
the first time a user runs it, it installs Homebrew for them (without
running that command: their terminal doesn't have Homebrew in `PATH` yet).
From then on it only runs their `brew`.

1. It asks first. Never as root, never without a terminal.
2. `sudo` creates `/var/opt/<uid>`, the user's and only theirs (0700:
   others can't see or run what they install; the only step as root).
3. It downloads Homebrew's official installer
   (`raw.githubusercontent.com/Homebrew/install/HEAD/install.sh`, HTTPS only)
   to a temporary file and runs it as the user with
   `--path=/var/opt/<uid>/.linuxbrew`, `HOMEBREW_NO_SUDO=1` (the folder is
   theirs; it never needs root), `NONINTERACTIVE=1` (they've just said yes)
   and `HOMEBREW_NO_ANALYTICS=1`.
4. `brew analytics off`, then it asks whether to share Homebrew's anonymous
   usage data (`brew analytics on`).

Each step is one line, a green ✓ when it's done; while it runs, a spinner,
its time and, in gray under it, the last lines of its output, which go away
when it ends. What the installer and `brew` print goes only to
`/tmp/homebrew-install-<random>.log`, whose path `brew` prints at the end
(and its last lines on a failure): the installer says analytics are on and
gives next steps that don't fit BrescaOS (`brew shellenv` in `~/.bashrc`,
`dnf`). Ctrl+C stops the step, which runs in the foreground; the spinner
draws in the background.

The prefix is in `/var`, writable and kept across updates (`/opt` is
read-only, as `/usr`), and short: the installer refuses a prefix longer
than its default, `/home/linuxbrew/.linuxbrew`, because Homebrew's bottles
can't move to a longer one. `/var/opt/<uid>/.linuxbrew` fits up to a 6-digit
uid.

`homebrew.sh` puts what Homebrew installs in `PATH` after the system's
commands, so it never takes the place of BrescaOS's own, and adds its
manuals, info pages and `brew`'s bash completion; a terminal opened before
the installation doesn't have them. `userdel` (and so System Settings >
Users) removes a user's Homebrew with them: `userdel` (this folder's) is its
`USERDEL_CMD` (`/etc/login.defs`, set by `build/system.sh`), because a new
user may get the same uid and would find it as their own. System Settings
runs `userdel` through `accounts-daemon`, whose service sees everything it
doesn't list as read-only: `accounts-daemon.conf` lists `/var/opt`, which
`homebrew.tmpfiles` creates at boot so it's there when the service starts.
Nothing checks the installer beyond
its download over HTTPS: [`docs/security.md`](../../../docs/security.md).
