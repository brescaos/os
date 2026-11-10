#!/usr/bin/env bash
# Hot reload for the development VM: copies the files image/Containerfile takes
# from image/src/ onto the running system whenever they change on the host,
# compiles their translations again when they change, and compiles again the
# components image/Containerfile compiles (those with a CMakeLists.txt).
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
# Where the translations are compiled, laid out as in the image, and the
# translated metadata the builds compile in.
TRANSLATIONS=/run/bresca-dev/translations
METADATA=/run/bresca-dev/metadata
# Each compiled component's build folder and what it installs, as
# <component>/build and <component>/install. They persist, so a build after a
# reboot only recompiles what changed.
BUILDS=/var/cache/bresca-dev/build

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

# --- Desktop notification -------------------------------------------------------
# After a pass that copied something, a notification says which projects of
# image/src/ changed (e.g. bresca-cli) and how many files. It
# goes to the logged-in user's session bus, as that user; with no one logged in
# there's none, and the copy goes on. Each one replaces the last.

# The last notification's id, for the next one to replace it.
notification_id=0

# Runs the command $@ as the logged-in user, in their session (its bus and
# its systemd); fails with no one logged in. From uid 1000: the login screen
# has a session bus too.
run_in_user_session() {
    local bus uid
    for bus in /run/user/*/bus; do
        [[ -S $bus ]] || continue
        uid=${bus#/run/user/}
        uid=${uid%/bus}
        if ((uid >= 1000)); then
            runuser -u "$(id -nu "$uid")" -- env DBUS_SESSION_BUS_ADDRESS="unix:path=$bus" \
                XDG_RUNTIME_DIR="/run/user/$uid" "$@"
            return
        fi
    done
    return 1
}

# Shows the summary $1 and the body $2 for $3 milliseconds (5000 when empty; 0,
# until the next one). desktop-entry: Plasma takes the name and the icon from
# bresca-dev-hotreload.desktop.
show_notification() {
    local summary=$1 body=$2 expire_milliseconds=${3:-5000} reply
    reply=$(run_in_user_session gdbus call --session --timeout 5 \
        --dest org.freedesktop.Notifications --object-path /org/freedesktop/Notifications \
        --method org.freedesktop.Notifications.Notify 'Hot reload' "$notification_id" view-refresh \
        "$summary" "$body" '[]' "{'desktop-entry': <'bresca-dev-hotreload'>}" "$expire_milliseconds" \
        2>/dev/null) || return 0
    # "(uint32 42,)"
    reply=${reply#*uint32 }
    notification_id=${reply%%,*}
}

# --- Copying -------------------------------------------------------------------------

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

# Counts the copy of $1 and notes what has to reload for it to take effect.
# For copy_changed_files, whose variables it sets.
note_copied_file() {
    copied_count=$((copied_count + 1))
    case $1 in
        /usr/lib/systemd/system/* | /usr/lib/systemd/user/*) is_unit_changed=true ;;
        /usr/share/plasma/* | /usr/share/locale/*/LC_MESSAGES/plasma_applet_* | /usr/share/knotifications6/*)
            is_plasma_changed=true
            ;;
        /usr/lib64/qt6/plugins/plasma/kcms/systemsettings/*.so) changed_settings_module=$(basename "$1" .so) ;;
        /usr/share/locale/*/LC_MESSAGES/kcm_*.mo) changed_settings_module=$(basename "$1" .mo) ;;
    esac
}

# Copies every file in the folder $1 into the folder $2, with the mode $3 (as
# copy_if_different). For copy_changed_files.
copy_folder() {
    local source=$1 destination=${2%/} mode=$3 file
    while IFS= read -r -d '' file; do
        if copy_if_different "$source/$file" "$destination/$file" "$mode"; then
            note_copied_file "$destination/$file"
        fi
    done < <(cd "$source" && find . -type f -printf '%P\0')
}

# Adds $1 to the projects the notification names, once. For
# copy_changed_files.
note_updated_project() {
    if [[ ", $updated_projects, " != *", $1, "* ]]; then
        updated_projects="${updated_projects:+$updated_projects, }$1"
    fi
}

# The checksum of what the translations are compiled from, when last compiled.
translation_sources_checksum=''

# Compiles the translations into TRANSLATIONS when what they're compiled from
# (po/*.po and the *.in templates of image/src/) changed; fails when it
# didn't, or when they don't compile (the error is in the log, and it's tried
# again on the next change).
compile_changed_translations() {
    local checksum
    checksum=$(cd "$SHARE/image/src" && find . \( -path '*/po/*.po' -o -name '*.in' \) -type f -print0 \
        | sort -z | xargs -0 sha256sum | sha256sum)
    [[ $checksum != "$translation_sources_checksum" ]] || return 1
    translation_sources_checksum=$checksum
    rm -rf "$TRANSLATIONS" "$METADATA"
    bash "$SHARE/image/build/compile_translations.sh" "$SHARE/image/src" "$TRANSLATIONS" "$METADATA"
}

# --- Compiling -----------------------------------------------------------------------

# The checksum of what the component $1 is compiled from, by component, when
# last compiled.
declare -A build_sources_checksums=()

# Prints the checksum of what the component $1 is compiled from: its sources
# (but tests/, po/ and READMEs) and its translated metadata.
print_build_sources_checksum() {
    local component=$1
    find "$SHARE/image/src/$component" \( -name tests -o -name po \) -prune -o -type f ! -name '*.md' -print0 \
        | {
            cat
            [[ ! -d $METADATA/$component ]] || find "$METADATA/$component" -type f -print0
        } \
        | sort -z | xargs -0 sha256sum | sha256sum
}

# Compiles the component $1 as its image/Containerfile stage does, when what
# it's compiled from changed, with a notification while it does, and copies
# what it installs. One job per CPU: a bare --parallel is an unlimited make -j,
# which runs the VM out of memory. When it doesn't compile, the error is in the
# log, and it's tried again on the next change. For copy_changed_files.
build_changed_component() {
    local component=$1 build_dir=$BUILDS/$1 checksum count_before=$copied_count
    checksum=$(print_build_sources_checksum "$component")
    [[ $checksum != "${build_sources_checksums[$component]:-}" ]] || return 0
    build_sources_checksums[$component]=$checksum
    echo "compiling $component"
    show_notification "Hot reload: $component" "Compiling…" 0
    rm -rf "$build_dir/install"
    if ! cmake -S "$SHARE/image/src/$component" -B "$build_dir/build" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr -DTRANSLATED_METADATA_DIR="$METADATA/$component" \
        || ! cmake --build "$build_dir/build" --parallel "$(nproc)" \
        || ! DESTDIR=$build_dir/install cmake --install "$build_dir/build"; then
        show_notification "Hot reload: $component" "Couldn't compile: see journalctl -u bresca-dev-hotreload"
        return 0
    fi
    copy_folder "$build_dir/install" / ''
    if ((copied_count > count_before)); then
        note_updated_project "$component"
    else
        show_notification "Hot reload: $component" "Compiled, no file changed"
    fi
}

# Closes System Settings, if it's open, and opens it again on the module $1,
# so that it loads the module's new build and translations.
reopen_settings() {
    pkill -x systemsettings || return 0
    timeout 5 sh -c 'while pgrep -x systemsettings >/dev/null; do sleep 0.2; done' || true
    if run_in_user_session systemd-run --user --quiet --collect systemsettings "$1"; then
        echo "reopened System Settings on $1"
    fi
}

copy_changed_files() {
    local mode source destination project count_before cmake_lists
    local is_unit_changed=false is_plasma_changed=false changed_settings_module='' copied_count=0
    local updated_projects=''

    while IFS='|' read -r mode source destination; do
        # src/<project>/...
        project=${source#src/}
        project=${project%%/*}
        count_before=$copied_count
        source=$SHARE/image/$source
        if [[ -d $source ]]; then
            copy_folder "$source" "$destination" "$mode"
        elif [[ -f $source ]]; then
            [[ $destination != */ ]] || destination=$destination${source##*/}
            if copy_if_different "$source" "$destination" "$mode"; then
                note_copied_file "$destination"
            fi
        fi
        if ((copied_count > count_before)); then
            note_updated_project "$project"
        fi
    done < <(print_copies)

    count_before=$copied_count
    if compile_changed_translations; then
        copy_folder "$TRANSLATIONS" / ''
    fi
    if ((copied_count > count_before)); then
        note_updated_project translations
    fi

    for cmake_lists in "$SHARE"/image/src/*/CMakeLists.txt; do
        project=${cmake_lists%/CMakeLists.txt}
        build_changed_component "${project##*/}"
    done

    if $is_unit_changed; then
        systemctl daemon-reload
        echo "reloaded systemd's units"
    fi
    if $is_plasma_changed && run_in_user_session systemctl --user restart plasma-plasmashell.service; then
        echo "restarted plasmashell"
    fi
    if [[ -n $changed_settings_module ]]; then
        reopen_settings "$changed_settings_module"
    fi
    if ((copied_count == 1)); then
        show_notification "Hot reload: $updated_projects" "Updated 1 file"
    elif ((copied_count > 1)); then
        show_notification "Hot reload: $updated_projects" "Updated $copied_count files"
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
