from spt3g import core

import pysmurf.core.devices
import pyrogue.gui
import argparse
import sosmurf
import sys
import threading


def main():
    parser = sosmurf.util.make_smurf_parser()

    parser.add_argument('--dev', action='store_true',
        help = "If set, use DevBoardEth context manager instead of CmbEth"
    )
    parser.add_argument('--stream-port', type=int, default=4536)

    args = parser.parse_args()
    pcie_kwargs, root_kwargs = sosmurf.util.process_args(args)

    builder = sosmurf.SmurfBuilder()
    transmitter = sosmurf.SmurfTransmitter(
        builder, name="SOSmurfTransmitter", debug_meta=False, debug_data=False
    )
    root_kwargs['txDevice'] = transmitter

    pipe = core.G3Pipeline()
    pipe.Add(builder)
    pipe.Add(core.Dump)
    pipe.Add(core.G3NetworkSender, hostname='*', port=args.stream_port,
                                   max_queue_size=1000)

    if args.dev:
        from pysmurf.core.roots.DevBoardEth import DevBoardEth as RootManager
    else:
        from pysmurf.core.roots.CmbEth import CmbEth as RootManager

    with pysmurf.core.devices.PcieCard(**pcie_kwargs):
        with RootManager(**root_kwargs) as root:
            print("got pysmurf root", flush=True)
            print("HERE!!", flush=True)
            if args.gui:
                print("Starting GUI...")
                app_top = pyrogue.gui.application(sys.argv)
                gui_top = pyrogue.gui.GuiTop(incGroups=None,excGroups=None)
                gui_top.setWindowTitle(args.windows_title)
                gui_top.addTree(root)
                gui_top.resize(800,1000)
                app_top.exec_()

            print("Starting G3Pipeline", flush=True)
            pipe.Run(profile=False)
            print("Closed G3 pipeline")

if __name__ == '__main__':
    main()
