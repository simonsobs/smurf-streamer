from spt3g import core

import rogue
import pyrogue.gui
import argparse
import sosmurf
import sys
import yaml
import os

import pysmurf.core.devices
import pysmurf.core.server_scripts.Common as pysmurf_common
import shlex


def main(): with open(os.path.expandvars('$OCS_CONFIG_DIR/sys_config.yml')) as f:
        cfg = yaml.safe_load(f)

    slot = int(os.environ['SLOT'])
    crate_id = cfg['crate_id']
    comm_type = cfg['comm_type']
    shelf_manager = cfg['shelf_manager']
    slot_cfg = cfg['slots'][f'SLOT[{slot}]']

    parser = pysmurf_common.make_parser()
    parser.add_argument(
        '--stream-port', type=int,
        default=slot_cfg.get('stream_port', 4530 + slot ))
    parser.add_argument(
        '--stream-id', type=str,
        default=slot_cfg.get('stream_id', f'crate{crate_id}slot{slot}'))

    modified_args = sosmurf.util.setup_server(cfg, slot)
    args = parser.parse_args(shlex.split(modified_args))
    pysmurf_common.process_args(args)

    # Sets some reasonable defaults
    if not args.epics_prefix:
        args.epics_prefix = f"smurf_server_s{slot}"
        print(f"Using epics root {args.epics_prefix}")
    if args.config_file is None:
        args.config_file = slot_cfg.get('rogue_defaults')
        print(f"Using config_file {args.config_file}")

    if args.pcie_rssi_lane is None:
        args.pcie_rssi_lane = slot - 2
        print(f"Using pcie lane {args.pcie_rssi_lane}")

    stream_root = sosmurf.StreamBase("SOStream", debug_meta=False,
                                     debug_data=False, agg_time=1.0)
    pipe = core.G3Pipeline()
    pipe.Add(stream_root.builder)
    pipe.Add(sosmurf.SessionManager.SessionManager, stream_id=args.stream_id)
    # pipe.Add(sosmurf.util.stream_dumper)
    pipe.Add(core.G3NetworkSender, hostname='*',
             port=args.stream_port, max_queue_size=1000)

    if comm_type == 'eth':
        from pysmurf.core.roots.CmbEth import CmbEth
        CmbRoot = CmbEth
    elif comm_type == 'pcie':
        from pysmurf.core.roots.CmbPcie import CmbPcie
        CmbRoot = CmbPcie
    else:
        raise ValueError(
            f"comm_type is {comm_type}. Must be either 'eth'' or 'pcie'.")

    pcie_kwargs = {
        'lane': args.pcie_rssi_lane, 'ip_addr': args.ip_addr,
        'dev_rssi': args.pcie_dev_rssi, 'dev_data': args.pcie_dev_data,
        'comm_type': f'{comm_type}-rssi-interleaved'
    }
    root_kwargs = {
        'config_file': args.config_file,
        'epics_prefix': args.epics_prefix, 'polling_en': args.polling_en,
        'pv_dump_file': args.pv_dump_file, 'disable_bay0': args.disable_bay0,
        'disable_bay1': args.disable_bay1, 'configure': args.configure,
        'txDevice': stream_root
    }

    if comm_type == 'eth':
        root_kwargs['ip_addr'] = args.ip_addr

    meta_file = os.path.expandvars(cfg.get('meta_register_file'))
    if meta_file is not None:
        print(f"Loading metadata registers from {meta_file}...")
        vgs = sosmurf.util.VariableGroups.from_file(meta_file)
        root_kwargs['VariableGroups'] = vgs.data

    if comm_type == 'pcie':
        root_kwargs.update({
            'pcie_rssi_lane': args.pcie_rssi_lane,
            'pcie_dev_rssi': args.pcie_dev_rssi,
            'pcie_dev_data': args.pcie_dev_data,
        })

    with pysmurf.core.devices.PcieCard(**pcie_kwargs):
        with CmbRoot(**root_kwargs) as root:
            print("got pysmurf root. Starting G3 Pipeline...", flush=True)
            pipe.Run(profile=False)
            print("Closed G3 pipeline")


if __name__ == '__main__':
    main()
