#!/usr/bin/env bash

. /usr/local/src/pysmurf/docker/server/scripts/server_common.sh


usage()
{
    echo "Start the SMuRF server on a specific board."
    echo ""
    echo "usage: ${script_name} [-S|--shelfmanager <shelfmanager_name> -N|--slot <slot_number>]"
    echo "                      [-a|--addr <FPGA_IP>] [-D|--no-check-fw] [-g|--gui] <pyrogue_server-args>"
    echo "    -S|--shelfmanager <shelfmanager_name> : ATCA shelfmanager node name or IP address. Must be used with -N."
    echo "    -N|--slot         <slot_number>       : ATCA crate slot number. Must be used with -S."
    echo "    -a|--addr         <FPGA_IP>           : FPGA IP address. If defined, -S and -N are ignored."
    echo "    -c|--comm-type    <comm_type>         : Communication type ('eth' or 'pcie'). Default is 'eth'."
    echo "    -D|--no-check-fw                      : Disabled FPGA version checking."
    echo "    -H|--hard-boot                        : Do a hard boot: reboot the FPGA and load default configuration."
    echo "    -h|--help                             : Show this message."
    echo "    <pyrogue_server_args> are passed to the SMuRF pyrogue server. "
    echo ""
    echo "If -a if not defined, then -S and -N must both be defined, and the FPGA IP address will be automatically calculated from the crate ID and slot number."
    echo "If -a if defined, -S and -N are ignored."
    echo
    echo "The script will by default check if the firmware githash read from the FPGA via IPMI is the same of the found in the MCS file name."
    echo "If they don't match, then the MCS file will be loaded into the FPGA. If this happens, the FPGA will be rebooted."
    echo "This checking can be disabled with -D. The checking will also be disabled if -a is used instead of -S and -N. If "
    echo
    echo "The script will look for a zip file under '${fw_top_dir}'. If found, it will be passed with the argument -z to the next startup script."
    echo "If not zip file is found, the script will then look for a local checked out repository in the same location; If found, the python directories"
    echo "under it will be added to PYTHONPATH."
    echo
    echo "All other arguments are passed verbatim to the next startup script."
    echo ""
    exit 1
}

#############
# Main body #
#############
# Read inputs arguments
while [[ $# -gt 0 ]]
do
key="$1"

case ${key} in
    -S|--shelfmanager)
    shelfmanager="$2"
    shift
    ;;
    -N|--slot)
    slot="$2"
    shift
    ;;
    -D|--no-check-fw)
    no_check_fw=1
    ;;
    -a|--addr)
    fpga_ip="$2"
    shift
    ;;
    -c|--comm-type)
    comm_type="$2"
    shift
    ;;
    -H|--hard-boot)
    hard_boot=1
    shift
    ;;
    -s|--server)
    ;;
    -h|--help)
    usage
    ;;
    *)
    args="${args} $key"
    ;;
esac
shift
done

# Validate the selected communication type
validateCommType

# Validate the selected slot number
validateSlotNumber

# Get FPGA IP address
getFpgaIpAddr

# Look for pyrogue files
findPyrogueFiles

# Firmware version checking
checkFW

# Do a hard boot, if requested
hardBoot

echo "NEW_ARGS=${args}"