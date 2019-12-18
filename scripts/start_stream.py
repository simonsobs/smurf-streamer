import pysmurf.core.devices
import pyrogue
import pyrogue.gui
import argparse
import sosmurf

def main():
    parser = sosmurf.util.make_smurf_parser()

    parser.add_argument('--dev', action='store_true',
        help = "If set, use DevBoardEth context manager instead of CmbEth"
    )

    args = parser.parse_args()
    pcie_kwargs, root_kwargs = sosmurf.util.process_args(args)

    streamer = sosmurf.SmurfStreamer("SmurfStreamer", config_file="config.txt")
    root_kwargs['txDevice'] = streamer

    if args.dev:
        from pysmurf.core.roots.DevBoardEth import DevBoardEth as RootManager
    else:
        from pysmurf.core.roots.CmbEth import CmbEth as RootManager

    with pysmurf.core.devices.PcieCard(**pcie_kwargs):
        with RootManager(**root_kwargs) as root:
            if args.gui:
                # Start the GUI
                print("Starting GUI...\n")
                app_top = pyrogue.gui.application(sys.argv)
                gui_top = pyrogue.gui.GuiTop(incGroups=None,excGroups=None)
                gui_top.setWindowTitle(args.windows_title)
                gui_top.addTree(root)
                gui_top.resize(800,1000)
                app_top.exec_()

                print("Gui exited")
                pyrogue.waitCntrlC()
            else:
                print("Running without gui.....")
                pyrogue.waitCntrlC()

            streamer.stop()


if __name__ == '__main__':
    main()
