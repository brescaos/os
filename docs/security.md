# Security defaults

BrescaOS aims to be safe by default. Here are the list of changes from the base bootc configuration:

| What | Default | Why | To turn it on |
|---|---|---|---|
| Automatic updates (`bootc-fetch-apply-updates.timer`) | Masked | It installs a new version and reboots at once, without asking. Updates go through `bresca updates` | — |
| SSH server (`sshd`) | Disabled, still installed | Nothing listens on the network by default | `sudo systemctl enable --now sshd` |
| `sssd` (enterprise logins), `mdmonitor` and `raid-check.timer` (software RAID), `nfs-client.target` (rpcbind, rpc.statd) | Disabled | Not needed on a desktop | `sudo systemctl enable --now <unit>` |
| `rpm-ostree-countme.timer` | Disabled | It counts the system as a Fedora one | — |
| LLMNR in `systemd-resolved` | Off | It answers name queries on every network, which anyone on it can spoof | — |

The disabled units are also in
`/usr/lib/systemd/system-preset/10-brescaos-disable.preset`, so a system's
first boot, which applies the presets, doesn't turn them back on.

Kept from Fedora: SELinux enforcing, Secure Boot with Fedora's signed kernel and shim,
a locked root account, and administrators in `wheel` with a password for
`sudo`.
