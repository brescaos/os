# shellcheck shell=bash
# Sourced by the bresca updates commands whose operation downloads.

# Starts bresca-updates@<operation> and waits for it, or for the one already
# running (e.g. started by System Settings). On a terminal, it shows
# download-progress.json on one line meanwhile. systemctl's failure message
# only points to systemctl status: the operation's result has ours.
start_operation_showing_download_progress() {
    local operation=$1
    local progress_file=/run/bresca/updates/download-progress.json
    local start_pid progress_line

    systemctl start "bresca-updates@$operation.service" 2>/dev/null &
    start_pid=$!

    if [[ -t 1 ]]; then
        while kill -0 "$start_pid" 2>/dev/null; do
            # Missing before the download starts and after it ends.
            if progress_line=$(jq -r '
                (if .phase == "checking" then "Checking for updates..."
                 elif .phase == "downloading" then "Downloading..."
                 else "Preparing it..." end)
                + (if .percent then " \(.percent)%" else "" end)' "$progress_file" 2>/dev/null); then
                printf '\r\e[K%s' "$progress_line"
            fi
            sleep 0.5
        done
        printf '\r\e[K'
    fi
    wait "$start_pid" || true
}
