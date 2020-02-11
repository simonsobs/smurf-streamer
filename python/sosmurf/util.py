import argparse
import socket
import subprocess
import os
import pyrogue
import sys

def setup_server():
    """
        Runs the shell script to setup and configure the smurf server.
        The shell script processes some command line arguments and returns a 
        modified version.
    """
    print("-"*60)
    print("Running setup_server.sh:\n")

    script = '/usr/local/src/smurf-streamer/scripts/setup_server.sh'
    proc = subprocess.run(
        [script] + sys.argv[1:], 
        stdout=subprocess.PIPE, 
        stderr=subprocess.STDOUT
    )

    print(proc.stdout.decode())

    if proc.returncode != 0:
        print(f"Error in setup_server script! Exited with return code {proc.returncode}")
        sys.exit(1)

    print("-"*60)

    for line in proc.stdout.decode().split('\n'):
        lsplit = line.split('=')
        if lsplit[0] == 'NEW_ARGS':
            args = ''.join(lsplit[1:])

    print(f"Found args: {args}")
    return args


def get_kwargs(args, dev_type, **extra_kwargs):
    """
        Returns the kwargs needed to instantiate specific rogue objects.

        Args:
            args: Argparse namespace
            dev_type (str):
                type of device. Can be 'pcie', 'cmb_eth', or 'dev_board_eth'.

        Any additional keyword arguments will be added to the dict.
    """
    if dev_type=="pcie":
        kwargs = {   
            'lane': args.pcie_rssi_lane,
            'ip_addr': args.ip_addr,
            'dev_rssi': args.pcie_dev_rssi,
            'dev_data': args.pcie_dev_data,
        }

    elif dev_type in ['cmb_eth', 'dev_board_eth']:
        kwargs = {
            'ip_addr':      args.ip_addr,
            'config_file':  args.config_file,
            'epics_prefix': args.epics_prefix,
            'polling_en':   args.polling_en,
            'pv_dump_file': args.pv_dump_file,
            'disable_bay0': args.disable_bay0,
            'disable_bay1': args.disable_bay1,
            'configure':    args.configure
        }

    kwargs.update(extra_kwargs)
    return kwargs