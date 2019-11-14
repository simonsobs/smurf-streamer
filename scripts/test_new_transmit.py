import pysmurf.core.devices
import pyrogue.gui
import argparse
import sosmurf
import sys
from spt3g import core
import threading

def run_gui(root, windows_title):
        app_top = pyrogue.gui.application(sys.argv)
        gui_top = pyrogue.gui.GuiTop(incGroups=None,excGroups=None)
        gui_top.setWindowTitle(windows_title)
        gui_top.addTree(root)
        gui_top.resize(800,1000)
        app_top.exec_()

def main():
    parser = sosmurf.util.make_smurf_parser()

    parser.add_argument('--dev', action='store_true',
        help = "If set, use DevBoardEth context manager instead of CmbEth"
    )

    args = parser.parse_args()
    pcie_kwargs, root_kwargs = sosmurf.util.process_args(args)

    builder = sosmurf.SmurfBuilder()
    transmitter = sosmurf.SmurfTransmitter(builder, name="SOSmurfTransmitter")
    root_kwargs['txDevice'] = transmitter

    pipe = core.G3Pipeline()
    print(dir(pipe), flush=True)
    pipe.Add(builder)
    pipe.Add(Dump)

    if args.dev:
        from pysmurf.core.roots.DevBoardEth import DevBoardEth as RootManager
    else:
        from pysmurf.core.roots.CmbEth import CmbEth as RootManager

    with pysmurf.core.devices.PcieCard(**pcie_kwargs):
        with RootManager(**root_kwargs) as root:
            print("got pysmurf root", flush=True)
            if args.gui:
                print("Starting GUI...")
                gui_thread = threading.Thread(target = run_gui,
                                              args = (root, args.windows_title))
                gui_thread.start()

            print("Starting G3_Pipeline")
            pipe.Run(profile=True)
            if args.gui:
                gui_thread.join()

if __name__ == '__main__':
    main()
