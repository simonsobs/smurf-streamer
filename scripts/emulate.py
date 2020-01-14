from spt3g import core

import pysmurf.core.devices
import pyrogue.gui
import argparse
import sosmurf
import sys
import threading


def main():
    parser = sosmurf.util.make_smurf_parser()
    parser.add_argument('--stream-port', type=int, default=4536)

    args = parser.parse_args()

    # Setup G3 Pipeline
    builder = sosmurf.SmurfBuilder()
    transmitter = sosmurf.SmurfTransmitter(
        builder, name="SOSmurfTransmitter", debug_meta=False, debug_data=False
    )

    pipe = core.G3Pipeline()
    pipe.Add(builder)
    pipe.Add(core.Dump)
    pipe.Add(core.G3NetworkSender, hostname='*', port=args.stream_port,
                                   max_queue_size=1000)

    emulation_kwargs = {
        'config_file':     args.config_file,
        'epics_prefix':    args.epics,
        'polling_en':      (not args.nopoll),
        'pv_dump_file':    args.dump_pvs,
        'disable_bay0':    args.disable_bay0,
        'disable_bay1':    args.disable_bay1,
        'txDevice':        transmitter,
    }

    with EmulationRoot(**emulation_kwargs) as root:
        for i in range(16):
            root._smurf_processor.setTesBias(index=i, val=(i-8))

        if args.gui:
            print("Starting GUI...")
            app_top = pyrogue.gui.application(sys.argv)
            gui_top = pyrogue.gui.GuiTop(incGroups=None,excGroups=None)
            gui_top.setWindowTitle(args.windows_title)
            gui_top.addTree(root)
            gui_top.resize(800,1000)
            app_top.exec_()

        else:
            print("Starting G3Pipeline", flush=True)
            pipe.Run(profile=True)
            print("Closed G3 pipeline")

if __name__ == '__main__':
    main()
