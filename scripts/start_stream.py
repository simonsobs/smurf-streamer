# Script to start smurf-streamer module

import pyrogue
import pyrogue.utilities.fileio
import rogue.interfaces.stream
import pysmurf.core.devices

from smurfstreamer._SmurfStreamer import SmurfStreamer
import argparse
import sys
import os
import socket
import subprocess


def make_parser(parser = None):
    # These parser args are used in every startup script. They should probably
    # be added to pyrogue somehow...
    if parser is None:
        parser = argparse.ArgumentParser()

    parser.add_argument('--zip', '-z',
        help="Pyrogue zip file to be included in python path"
    )
    parser.add_argument('--addr', '-a', required=True,
        help="FPGA IP address. Required when the communication iss based off on Ethernet."
    )
    parser.add_argument('--gui', '-g', action='store_true',
        help="Starts the server with a gui"
    )
    parser.add_argument('--windows-title', '-w',
        help="Sets the gui windows title. Defaults to name of this script."
    )
    parser.add_argument('--epics', '-e',
        help="PV name prefix of EPICS server"
    )
    parser.add_argument('--nopoll', '-n', action='store_true',
        help="Disables all polling"
    )
    parser.add_argument('--stream-size', '-b', type=int, default=2**19,
        help="Number of stream data points to be exposed as EPICS PV's. "
             "Default is 2^19."
    )
    parser.add_argument('--stream-type', '-f', default="Int16",
        choices=["UInt16", "Int16", "UInt32", "Int32"],
        help="Stream data type (UInt16, Int16, UInt32 or Int32). Default is Int16."
    )
    parser.add_argument('--defaults', '-d',
        help="Default configuration file. If the path is relative, it refers to "
             "the zip file"
    )
    parser.add_argument('--pcie-rssi-link', '-l', type=int, choices=[0,1,2,3,4,5],
        help="PCIe RSSI lane."
    )
    parser.add_argument('--dump-pvs', '-u',
        help="File to dump PV list"
    )
    parser.add_argument('--disable_bay0', action='store_true',
        help="Disable the instantiation of devices for Bay 0."
    )
    parser.add_argument('--disable_bay1', action='store_true',
        help="Disable the instantiation of devices for Bay 1"
    )
    parser.add_argument('--disable-gc', action='store_true',
        help="Disables python's garbage collection."
    )
    parser.add_argument('--pcie-dev-rssi', default="/dev/datadev_0",
        help="Set the PCIe card device name used for RSSI"
    )
    parser.add_argument('--pcie-dev-data', default="/dev/datadev_1",
        help="Sets the PCIe card device name used for data"
    )
    return parser


def check_address(address):
    try:
        socket.inet_pton(socket.AF_INET, address)
    except socket.error:
        exit_message("ERROR: Invalid IP Address.")

    print("\nTrying to ping the FPGA....")
    try:
       dev_null = open(os.devnull, 'w')
       subprocess.check_call(["ping", "-c2", address], stdout=dev_null, stderr=dev_null)
       print("    FPGA is online\n")
    except subprocess.CalledProcessError:
       exit_message("    ERROR: FPGA can't be reached!")


def main():
    parser = make_parser()
    args = parser.parse_args()

    if args.zip and os.path.exists(args.zip):
        pyrogue.addLibraryPath(args.zip+"/python")
        # If defaults is relative, use the one in the zip file
        if args.defaults and args.defaults[0] != '/':
            config_file = args.zip + '/config/' + args.defaults
        else:
            config_file = args.defaults

    from pysmurf.core.roots.DevBoardEth import DevBoardEth

    if args.disable_gc:
        import gc
        gc.disable()
        print("GARBAGE COLLECTION DISABLED")

    check_address(args.addr)

    pcie_kwargs = {
        'lane': args.pcie_rssi_link,
        'comm_type': 'eth-rssi-interleaved',
        'ip_addr': args.addr,
        'dev_rssi': args.pcie_dev_rssi,
        'dev_data': args.pcie_dev_data,
    }
    print("", flush=True)
    streamer = SmurfStreamer("SmurfStreamer", config_file="config.txt")

    devboard_kwargs = {
        'ip_addr':         args.addr,
        'config_file':     config_file,
        'epics_prefix':    args.epics,
        'polling_en':      (not args.nopoll),
        'pcie_rssi_lane':  args.pcie_rssi_link,
        'stream_pv_size':  args.stream_size,
        'stream_pv_type':  args.stream_type,
        'pv_dump_file':    args.dump_pvs,
        'disable_bay0':    args.disable_bay0,
        'disable_bay1':    args.disable_bay1,
        'disable_gc':      args.disable_gc,
        'pcie_dev_rssi':   args.pcie_dev_rssi,
        'pcie_dev_data':   args.pcie_dev_data,
        'txDevice':        streamer
    }
    print("", flush=True)

    with pysmurf.core.devices.PcieCard(**pcie_kwargs):
        with DevBoardEth(**devboard_kwargs) as root:
            print("Running streamer.....")
            pyrogue.waitCntrlC()
            streamer.stop()


if __name__ == '__main__':
    main()
