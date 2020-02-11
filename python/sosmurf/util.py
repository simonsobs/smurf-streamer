import argparse
import socket
import subprocess
import os
import pyrogue


def get_kwargs(args, dev_type, **extra_kwargs):
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