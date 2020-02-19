from spt3g import core

import pyrogue.gui
import argparse
import sosmurf
import sys
import threading
import shlex 
import pysmurf.core.devices
import pysmurf.core.server_scripts.Common as pysmurf_common


def main():
    parser = pysmurf_common.make_parser()

    parser.add_argument('--stream-port', type=int, default=4536)
    parser.add_argument('--stream-id', type=str)

    modified_args = sosmurf.util.setup_server()

    args = parser.parse_args(shlex.split(modified_args))
    pysmurf_common.process_args(args)

    from pysmurf.core.roots.CmbPcie import CmbPcie

    if args.ip_addr: 
        pysmurf_common.verify_ip(args)


    builder = sosmurf.SmurfBuilder()
    transmitter = sosmurf.SmurfTransmitter(
        builder, name="SOSmurfTransmitter", debug_meta=False, debug_data=False
    )

    pipe = core.G3Pipeline()
    pipe.Add(builder)
    pipe.Add(sosmurf.SessionManager.SessionManager, stream_id = args.stream_id)
    pipe.Add(core.Dump)
    pipe.Add(core.G3NetworkSender, hostname='*', port=args.stream_port,
                                   max_queue_size=1000)

    vgs = {
        'root.FpgaTopLevel.AppTop.AppCore.enableStreaming': 
            {'groups': ['publish', 'stream'], 'pollInterval': None},
        'root.RogueVersion'     : {'groups' : ['publish','stream'], 'pollInterval': None},
        'root.RogueDirectory'   : {'groups' : ['publish','stream'], 'pollInterval': None},
        'root.SmurfApplication' : {'groups' : ['publish','stream'], 'pollInterval': None},
        'root.SmurfProcessor'   : {'groups' : ['publish','stream'], 'pollInterval': None},
    }

    pcie_kwargs = sosmurf.util.get_kwargs(args, 'pcie', comm_type='pcie-rssi-interleaved')
    root_kwargs = sosmurf.util.get_kwargs(args,'cmb_pcie', txDevice = transmitter, VariableGroups=vgs)
    
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
