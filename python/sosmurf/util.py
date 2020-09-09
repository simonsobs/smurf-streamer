import argparse
import socket
import subprocess
import os
import pyrogue
import sys
from spt3g import core
import yaml
import re

class stream_dumper:
    """Simple dump module that ignores flow-control keep_alive frames"""
    def __init__(self):
        self.last_frame_idle=False

    def __call__(self, frame):
        if frame.type == core.G3FrameType.none:
            if frame['sostream_flowcontrol'] == 0:
                print('.', end='', flush=True)
                self.last_frame_idle=True
                return

        if self.last_frame_idle:
            print('')

        print(frame, flush=True)


def setup_server(cfg, slot):
    """
        Runs the shell script to setup and configure the smurf server.
        The shell script processes some command line arguments and returns a 
        modified version.
    """
    print("-"*60)
    # Replaces any of these args with defaults from sys_config
    pre_parser = argparse.ArgumentParser()
    pre_parser.add_argument('--shelfmanager', '-S')
    pre_parser.add_argument('--addr', '-a')
    pre_parser.add_argument('--comm-type', '-c')
    args, _ = pre_parser.parse_known_args()
    setup_args = sys.argv[1:]

    if args.shelfmanager is None:
        setup_args.extend(['-S', cfg['shelf_manager'], '-N', str(slot)])

    # elif args.addr is None and args.shelfmanager is None:
    #     setup_args.extend(['--addr', f'10.0.1.{100+slot}'])

    if args.comm_type is None:
        setup_args.extend(['--comm-type', cfg['comm_type']])

    print("Running setup_server.sh {}\n".format(' '.join(setup_args)))

    script = '/usr/local/src/smurf-streamer/scripts/setup_server.sh'
    proc = subprocess.run(
        [script] + setup_args,
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


class VariableGroups:
    """
    This class is to help translate from a ``meta_regsiters.yaml`` file to
    the variable groups object taken by the rogue root. The yaml file should
    be like ordinary yaml, except that the keys can be somewhat templated. The
    values should be either a `group_mask`, or the bitwise-or of the group
    keys it should belong to, or a list `[group_mask, pollInterval]`.

    For instance, the yaml::

        root.FpgaTopLevel.AppTop.AppCore:
            enableStreaming: (3, 1)
            SysgenCryo.Base[{{range(4)}}]:
                etaMagArray: 3

    will expand to ::

        {
            'root.FpgaTopLevel.AppTop.AppCore.enableStreaming':
                {'groups': ['stream', 'publish'], 'pollInterval': 1}
            'root.FpgaTopLevel.AppTop.AppCore.Base[0].etaMagArray':
                {'groups': ['stream', 'publish']}
            'root.FpgaTopLevel.AppTop.AppCore.Base[1].etaMagArray':
                {'groups': ['stream', 'publish']}
            'root.FpgaTopLevel.AppTop.AppCore.Base[2].etaMagArray':
                {'groups': ['stream', 'publish']}
            'root.FpgaTopLevel.AppTop.AppCore.Base[3].etaMagArray':
                {'groups': ['stream', 'publish']}
        }
    """
    group_key = {'stream': 1, 'publish': 2}

    def __init__(self, data):
        self._data = data
        self.data = self._data.copy()
        self.template()
        self.flatten()
        self.process()

    def process(self):
        for k, v in self.data.items():
            self.data[k] = {
                'groups': [],
                'pollInterval': None
            }

            if isinstance(v, list):
                group_mask = v[0]
                self.data[k]['pollInterval'] = v[1]
            else:
                group_mask = v
            for g, i in VariableGroups.group_key.items():
                if group_mask & i:
                    self.data[k]['groups'].append(g)

    @classmethod
    def from_file(cls, filename):
        with open(filename) as f:
            data = yaml.safe_load(f)
        return cls(data)

    def template(self):
        def helper(data):
            rv = {}
            for k_, v_ in data.items():
                k, v = k_, v_
                if isinstance(v, dict):
                    v = helper(v)
                m = re.search('{{(.*?)}}', k)
                if m is not None:
                    x = eval(m.group(1))
                    try:
                        for i in x:
                            rv[k.replace(m.group(0), str(i))] = v
                    except TypeError:
                        rv[k.replace(m.group(0), str(x))] = v
                else:
                    rv[k] = v
            return rv
        self.data = helper(self.data)

    def flatten(self):
        def helper(data):
            rv = {}
            for k, v in data.items():
                if type(v) is not dict:
                    rv[k] = v
                    continue
                for k2, v2 in helper(v).items():
                    rv[f'{k}.{k2}'] = v2
            return rv
        self.data = helper(self.data)


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

    elif dev_type=="cmb_pcie":
         kwargs = {
            'config_file'   :  args.config_file,
            'epics_prefix'  :  args.epics_prefix,
            'polling_en'    :  args.polling_en,
            'pv_dump_file'  :  args.pv_dump_file,
            'disable_bay0'  :  args.disable_bay0,
            'disable_bay1'  :  args.disable_bay1,
            'pcie_rssi_lane':  args.pcie_rssi_lane,
            'pcie_dev_rssi' :  args.pcie_dev_rssi,
            'pcie_dev_data' :  args.pcie_dev_data,
            'configure'     :  args.configure,
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
