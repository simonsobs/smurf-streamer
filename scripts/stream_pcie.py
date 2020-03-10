from spt3g import core

import pyrogue.gui
import argparse
import sosmurf
import sys
import threading
import shlex 
import pysmurf.core.devices
import pysmurf.core.server_scripts.Common as pysmurf_common
from pprint import pprint

def main():
    parser = pysmurf_common.make_parser()

    parser.add_argument('--stream-port', type=int, default=4536)
    parser.add_argument('--stream-id', type=str)

    modified_args = sosmurf.util.setup_server()

    args = parser.parse_args(shlex.split(modified_args))
    pysmurf_common.process_args(args)

    if args.ip_addr: 
        pysmurf_common.verify_ip(args)

    stream_root = sosmurf.StreamBase("SOStream", debug_meta=False, debug_data=False, agg_time=1.0)

    pipe = core.G3Pipeline()
    pipe.Add(stream_root.builder)
    pipe.Add(sosmurf.SessionManager.SessionManager, stream_id = args.stream_id)
    pipe.Add(sosmurf.util.stream_dumper)
    pipe.Add(core.G3NetworkSender, 
        hostname='*', port=args.stream_port, max_queue_size=1000
    )

    # Builds variable groups dict
    app_core = 'root.FpgaTopLevel.AppTop.AppCore'

    meta_registers = [
        f'{app_core}.enableStreaming',
        'root.RogueVersion',
        'root.RoueDirectory',
        'root.SmurfApplication',
    ]

    for i in range(8):
        meta_registers.extend([
            f'{app_core}.SysgenCryo.Base[{i}].band',
            f'{app_core}.SysgenCryo.Base[{i}].etaMagArray',
            f'{app_core}.SysgenCryo.Base[{i}].etaPhaseArray',
            f'{app_core}.SysgenCryo.Base[{i}].amplitudeScaleArray',
            f'{app_core}.SysgenCryo.Base[{i}].centerFrequencyArray'
        ])

    vgs = {
        k: {'groups': ['publish', 'stream'], 'pollInterval': None} for k in meta_registers
    }

    pprint(f"Subscribing to groups: \n{meta_registers}")

    pcie_kwargs = sosmurf.util.get_kwargs(args, 'pcie', comm_type='pcie-rssi-interleaved')
    root_kwargs = sosmurf.util.get_kwargs(args,'cmb_pcie', txDevice = stream_root, VariableGroups=vgs)

    from pysmurf.core.roots.CmbPcie import CmbPcie    

    with pysmurf.core.devices.PcieCard(**pcie_kwargs):
        with CmbPcie(**root_kwargs) as root:
            print("got pysmurf root", flush=True)
            if args.use_gui:
                print("Starting GUI...")
                app_top = pyrogue.gui.application(sys.argv)
                gui_top = pyrogue.gui.GuiTop(incGroups=None,excGroups=None)
                gui_top.setWindowTitle(args.windows_title)
                gui_top.addTree(root)
                gui_top.resize(800,1000)
                app_top.exec_()

            else:
                # pyrogue.waitCntrlC()
                print("Starting G3Pipeline", flush=True)
                pipe.Run(profile=False)
                print("Closed G3 pipeline")

if __name__ == '__main__':
    main()
