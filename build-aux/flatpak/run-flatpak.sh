#!/bin/bash

read_arg() {
    # $1 = arg name
    # $2 = arg value
    # $3 = arg parameter
    local rematch='^[^=]*=(.*)$'
    if [[ $2 =~ $rematch ]]; then
        read "$1" <<< "${BASH_REMATCH[1]}"
    else
        read "$1" <<< "$3"
        # There is no way to shift our callers args, so
        # return 1 to indicate they should do it instead.
        return 1
    fi
}

set -e

build=0
run=0
clean=0
print_help=0

while (($# > 0)); do
        case "${1%%=*}" in
                build) build=1;;
                run) run=1;;
                clean) clean=1;;
                help) print_help=1;;
                --app-id) read_arg app_id "$@" || shift;;
                --manifest) read_arg manifest "$@" || shift;;
                *) echo -e "\e[1;31mERROR\e[0m: Unknown option '$1'"; exit 1;;
        esac
        shift
done

if [ $print_help == 1 ]; then
        echo "$0 - Build and run Flatpak"
        echo ""
        echo "Usage: $0 <command> [options]"
        echo ""
        echo "Available commands"
        echo ""
        echo "  build --manifest=<FILE> - Build Flatpak <MANIFEST>"
        echo "  run --app-id=<APP>      - Run Flatpak app <APP>"
        echo "  clean --app-id=<APP>    - Uninstall Flatpak app <APP>"
        echo "  help                    - This help message"
        echo ""
        exit 0
fi

FLATPAK_BUILDER=$( which flatpak-builder )
FLATPAK=$( which flatpak )

if [ $build == 1 ]; then
        if [ ! -z $app_id ]; then
                manifest="$app_id.json"
        fi

        if [ -z $manifest ]; then
                echo "Usage: $0 build --manifest=<FILE>"
                exit 1
        fi

        if [ ! -f "$manifest" ]; then
                echo -e "\e[1;31mERROR\e[0m: Manifest not found"
                exit 1
        fi

        echo -e "\e[1;32mBUILDING\e[0m: ${manifest}"

        ${FLATPAK_BUILDER} --force-clean --user --install build "${manifest}"

        exit $?
fi

if [ $run == 1 ]; then
        if [ -z $app_id ]; then
                echo "Usage: $0 run --app-id=<APP>"
                exit 1
        fi

        ${FLATPAK} run ${app_id}

        exit $?
fi

if [ $clean == 1 ]; then
        if [ -z $app_id ]; then
                echo "Usage: $0 clean --app-id=<APP>"
                exit 1
        fi

        ${FLATPAK} uninstall --user $app_id

        exit $?
fi

echo "Usage: $0 <command>"
echo "See: $0 help"
exit 0
