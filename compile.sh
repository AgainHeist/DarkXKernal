#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NDK_BUILD="${NDK_BUILD:-}"

# Accept either the ndk-build executable itself or an NDK root directory.
if [[ -n "$NDK_BUILD" && -d "$NDK_BUILD" ]]; then
    NDK_BUILD="$NDK_BUILD/build/ndk-build"
fi

if [[ -z "$NDK_BUILD" ]]; then
    candidates=()

    for ndk_root in "${ANDROID_NDK_HOME:-}" "${ANDROID_NDK_ROOT:-}"; do
        [[ -n "$ndk_root" ]] && candidates+=("$ndk_root/build/ndk-build" "$ndk_root/ndk-build")
    done

    if [[ -n "${ANDROID_HOME:-}" ]]; then
        candidates+=("$ANDROID_HOME/ndk/24.0.8215888/build/ndk-build")
    fi

    candidates+=(
        "$HOME/android-sdk/ndk/24.0.8215888/build/ndk-build"
        "/data/user/0/com.itsaky.androidide/files/home/android-sdk/ndk/24.0.8215888/build/ndk-build"
    )

    for candidate in "${candidates[@]}"; do
        if [[ -x "$candidate" ]]; then
            NDK_BUILD="$candidate"
            break
        fi
    done
fi

if [[ ! -x "$NDK_BUILD" ]]; then
    echo "Android NDK ndk-build not found." >&2
    echo "Install NDK 24.0.8215888 in AndroidIDE, or set ANDROID_NDK_HOME/NDK_BUILD." >&2
    exit 1
fi

exec "$NDK_BUILD" -C "$PROJECT_DIR" "$@"
