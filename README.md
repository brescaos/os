# BrescaOS

> [!WARNING]
> **BrescaOS is in early development.** There is nothing to install yet.

A desktop Linux distribution built as a versioned, transactional operating
system. Each release is a complete
[bootc](https://bootc-dev.github.io/bootc/) container image based on
[Fedora®](https://fedoraproject.org): an update installs a new version next to
the running one, and you can go back to the previous one.

> BrescaOS is not provided or supported by the Fedora Project. Official,
> unmodified Fedora software is available at <https://fedoraproject.org/>.

## Building

See [docs/building.md](docs/building.md).

## Source code

BrescaOS's own code is in this repository, under the
[GNU General Public License, version 3 or later](LICENSE). Every other package
comes unmodified from Fedora. The source code of each package is in
[Fedora's Koji](https://koji.fedoraproject.org/koji/), the system that builds
Fedora's packages.

## Fedora trademarks

Fedora is a trademark or registered trademark of Red Hat, Inc., or its
subsidiaries in the United States and other countries.
