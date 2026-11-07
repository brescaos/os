# Security defaults

BrescaOS aims to be safe by default. Here are the list of changes from the base bootc configuration:

| What | Default | Why | To turn it on |
|---|---|---|---|
| Automatic updates (`bootc-fetch-apply-updates.timer`) | Masked | It installs a new version and reboots at once, without asking. Instead, `bresca-updates-check.timer` checks every 6 hours, never installs or reboots, and a tray icon and a notification say what it finds; installing goes through `bresca updates` | — |
| SSH server (`sshd`) | Disabled, still installed | Nothing listens on the network by default | `sudo systemctl enable --now sshd` |
| `sssd` (enterprise logins), `mdmonitor` and `raid-check.timer` (software RAID), `nfs-client.target` (rpcbind, rpc.statd) | Disabled | Not needed on a desktop | `sudo systemctl enable --now <unit>` |
| `rpm-ostree-countme.timer` | Disabled | It counts the system as a Fedora one | — |
| LLMNR in `systemd-resolved` | Off | It answers name queries on every network, which anyone on it can spoof | — |
| Firewall (`firewalld`) | On, zone `default` ("BrescaOS default") for every network without a zone of its own, as every new one is. Of Fedora's zones only `block`, `drop` and `trusted` (firewalld needs them) and `nm-shared` (Wi-Fi hotspot) are kept. Fedora's `gateway-*` policies, for routing other networks, are removed too: they name zones that are gone | Every incoming connection is dropped; only DHCPv6 replies get in | Open a service in Settings > Firewall, or `sudo firewall-cmd --permanent --add-service=<service> && sudo firewall-cmd --reload` (e.g. `ssh`, after enabling `sshd`) |
| Checking for updates without a password (polkit, `50-brescaos-updates-check.rules`) | Only an administrator (group `wheel`) at the computer, only starting `bresca-updates@check` | A check only reads; downloading, installing and anything else with services still ask for a password | — |

The disabled units are also in
`/usr/lib/systemd/system-preset/10-brescaos-disable.preset`, so a system's
first boot, which applies the presets, doesn't turn them back on.

Kept from Fedora: SELinux enforcing, Secure Boot with Fedora's signed kernel and shim,
a locked root account, and administrators in `wheel` with a password for
`sudo`.
