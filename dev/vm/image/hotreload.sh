#!/usr/bin/env bash
# Hot reload for the development VM: copies the files image/Containerfile takes
# from image/src/ onto the running system whenever they change on the host.
# In the VM's image (dev/vm/image/Containerfile) as
# /usr/libexec/bresca-dev/hotreload, run as root by bresca-dev-hotreload.service.
# It reads the repository that dev/vm/start.sh shares read-only over 9p.
#
# /usr is read-only: bootc usr-overlay makes it writable until the next boot,
# when the image's files come back and this copies the changed ones again.
# Files deleted on the host stay in the VM until then.
# Logs: journalctl -fu bresca-dev-hotreload
set -euo pipefail

SHARE=/run/bresca-dev/src

# Prints "<--chmod mode or empty>|<source>|<destination>" for each COPY whose
# source is under src/, so what reloads is always what the image copies.
print_copies() {
    awk '$1 == "COPY" {
        mode = ""
        count = 0
        for (i = 2; i <= NF; i++) {
            if ($i ~ /^--chmod=/) mode = substr($i, 9)
            else if ($i !~ /^--/) argument[++count] = $i
        }
        if (count == 2 && argument[1] ~ /^src\//) print mode "|" argument[1] "|" argument[2]
    }' "$SHARE/image/Containerfile"
}

# Copies $1 to $2 with the mode $3 (the source's when empty) unless it's
# already there; succeeds only when it copied. A new file instead of written
# over: a program running the old one keeps it whole.
copy_if_different() {
    local source=$1 destination=$2 mode=${3#0}
    [[ -n $mode ]] || mode=$(stat -c %a "$source")
    # sha256sum, from coreutils: cmp's diffutils may not be in the image.
    if [[ -f $destination && $(sha256sum <"$source") == $(sha256sum <"$destination") &&
    $(stat -c %a "$destination") == "$mode" ]]; then
        return 1
    fi
    mkdir -p "$(dirname "$destination")"
    cp --remove-destination "$source" "$destination"
    chmod "$mode" "$destination"
    echo "updated $destination"
}

copy_changed_files() {
    local mode source destination file
    local is_unit_changed=false

    while IFS='|' read -r mode source destination; do
        source=$SHARE/image/$source
        if [[ -d $source ]]; then
            while IFS= read -r -d '' file; do
                if copy_if_different "$source/$file" "${destination%/}/$file" "$mode" && [[ $file == *.service ]]; then
                    is_unit_changed=true
                fi
            done < <(cd "$source" && find . -type f -printf '%P\0')
        elif [[ -f $source ]]; then
            [[ $destination != */ ]] || destination=$destination${source##*/}
            if copy_if_different "$source" "$destination" "$mode" && [[ $destination == *.service ]]; then
                is_unit_changed=true
            fi
        fi
    done < <(print_copies)

    if $is_unit_changed; then
        systemctl daemon-reload
        echo "reloaded systemd's units"
    fi
}

mkdir -p "$SHARE"
mountpoint -q "$SHARE" || mount -t 9p -o trans=virtio,version=9p2000.L,ro bresca-src "$SHARE"
mountpoint -q /usr || bootc usr-overlay >/dev/null
echo "watching the host's image/src/"
while true; do
    copy_changed_files
    sleep 1
done
