import argparse
import socket
import subprocess
import os
import pyrogue

def process_args(args):
    """
    Acts on parsed SMuRF args. Sets address based on shelf_manager and slot,
    verifies ip address, and correctly sets args.config_file.

    Args:
        args (namespace):
            args returned by parse_args with parser created by make_smurf_parser.

    Returns:
        pcie_kwargs (dict):
            kwargs to pass to pyrogue.core.devices.PcieCard
        root_kwargs (dict):
            kwargs to pass to RootManager (either DevBoardEth or CmbEth)
    """
    if args.addr is None:
        if (args.shelf_manager is None) or (args.slot is None):
            raise Exception("If address is not set, you must specify "
                            "shelf_manager and slot number.")
        args.addr = find_shelf_ip(args.shelf_manager, args.slot)
        print(f"Set args.address to {args.addr}...")

    print("Checking ip...")
    check_ip(args.addr)

    # Sets config file correctly
    # Fix this, I don't think it works correctly. Maybe specify default config
    # dir etc....
    if args.zip and os.path.exists(args.zip):
        pyrogue.addLibraryPath(os.path.join(args.zip, 'python'))
        # If defaults is relative, use the one in the zip file
        if args.defaults and args.defaults[0] != '/':
            args.config_file = args.zip + '/config/' + args.defaults
        else:
            args.config_file = args.defaults
        print(f"Set args.config_file to {args.config_file}")
    else:
        raise Exception("Zip file not specified or doesn't exist...")

    if args.disable_gc:
        import gc
        gc.disable()
        print("Disabling garbage collection...")

    pcie_kwargs = {
        'lane': args.pcie_rssi_link,
        'comm_type': args.comm_type,
        'ip_addr': args.addr,
        'dev_rssi': args.pcie_dev_rssi,
        'dev_data': args.pcie_dev_data,
    }

    root_kwargs = {
        'ip_addr':         args.addr,
        'config_file':     args.config_file,
        'epics_prefix':    args.epics,
        'polling_en':      (not args.nopoll),
        'stream_pv_size':  args.stream_size,
        'stream_pv_type':  args.stream_type,
        'pv_dump_file':    args.dump_pvs,
        'disable_bay0':    args.disable_bay0,
        'disable_bay1':    args.disable_bay1,
    }

    return pcie_kwargs, root_kwargs

def check_ip(address):
    """
    Verifies connection with FPGA ip address

    args:
        address (str):
            IP address to check connection
    """
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


def find_shelf_ip(shelf_manager, slot):
    """
    Finds ip address of SMuRF card

    Args:
        shelf_manager (str):
            Shelf manager name
        slot (int):
            Slot number
    """

    print(f"Searching for shelf {shelf_manager} with slot {slot}")

    ipmb = 128 + 2 * slot

    # I have no idea what this does. Stolen from:
    # https://github.com/slaclab/pysmurf/blob/0a64ffdb268c87cb59505511360faa6bd602a013/docker/server/scripts/start_server.sh#L84
    crate_id_cmd = f"ipmitool -I lan -H {shelf_manager} -t {ipmb} -b 0 -A NONE raw 0x34 0x04 0xFD 0x02"

    proc = subprocess.run(crate_id_cmd, shell=True, check=True, stdout=subprocess.PIPE)
    crate_id_list = proc.stdout.strip().split()
    crate_id_list.reverse()
    crate_id_list = list(map(lambda x: int(x, 16), crate_id_list))

    ip_addr = f"10.{crate_id_list[0]}.{crate_id_list[1]}.{100 + slot}"

    print(f"Found IP address: {ip_addr}")
    return ip_addr


def make_smurf_parser(parser = None):
    """
    Adds SMuRF command line arguments to parser

    Args:
        parser (argparse.ArgumentParser):
            if not none, arguments will be added to existing parser
    """
    if parser is None:
        parser = argparse.ArgumentParser()

    group = parser.add_argument_group("SMuRF Options")

    group.add_argument('--zip', '-z',
        help="Pyrogue zip file to be included in python path"
    )
    group.add_argument('--addr', '-a',
        help="FPGA IP address. Required when the communication iss based off on Ethernet."
    )
    group.add_argument('--shelf-manager','-S', help="ATCA shelfmanager node name")
    group.add_argument('--slot', '-N', type=int, choices=[1,2,3,4,5,6],
        help="ATCA Crate slot number"
    )

    group.add_argument('--gui', '-g', action='store_true',
        help="Starts the server with a gui"
    )
    group.add_argument('--windows-title', '-w',
        help="Sets the gui windows title. Defaults to name of this script."
    )
    group.add_argument('--epics', '-e',
        help="PV name prefix of EPICS server"
    )
    group.add_argument('--nopoll', '-n', action='store_true',
        help="Disables all polling"
    )
    group.add_argument('--stream-size', '-b', type=int, default=2**19,
        help="Number of stream data points to be exposed as EPICS PV's. "
             "Default is 2^19."
    )
    group.add_argument('--stream-type', '-f', default="Int16",
        choices=["UInt16", "Int16", "UInt32", "Int32"],
        help="Stream data type (UInt16, Int16, UInt32 or Int32). Default is Int16."
    )
    group.add_argument('--defaults', '-d',
        help="Default configuration file. If the path is relative, it refers to "
             "the zip file"
    )
    group.add_argument('--pcie-rssi-link', '-l', type=int, choices=[0,1,2,3,4,5],
        help="PCIe RSSI lane."
    )
    group.add_argument('--dump-pvs', '-u',
        help="File to dump PV list"
    )
    group.add_argument('--disable_bay0', action='store_true',
        help="Disable the instantiation of devices for Bay 0."
    )
    group.add_argument('--disable_bay1', action='store_true',
        help="Disable the instantiation of devices for Bay 1"
    )
    group.add_argument('--disable-gc', action='store_true',
        help="Disables python's garbage collection."
    )
    group.add_argument('--comm_type', default='eth-rssi-interleaved',
        choices=['eth-rssi-interleaved', 'pcie-rssi-interleaved'],
        help="FPGA communication type"
    )
    group.add_argument('--pcie-dev-rssi', default="/dev/datadev_0",
        help="Set the PCIe card device name used for RSSI"
    )
    group.add_argument('--pcie-dev-data', default="/dev/datadev_1",
        help="Sets the PCIe card device name used for data"
    )
    return parser
