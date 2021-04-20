from spt3g import core

import pyrogue.gui
import pysmurf.core.devices
import pysmurf.core.server_scripts.Common as pysmurf_common

import argparse
import shlex
import sosmurf
import sys


def main():

    parser = pysmurf_common.make_parser()

    parser.add_argument('--stream-port', type=int, default=4536)
    parser.add_argument('--stream-id', type=str)

    args = parser.parse_args()
    pysmurf_common.process_args(args)

    stream_root = sosmurf.StreamBase("SOStream", debug_meta=False, debug_data=False, debug_builder=True, agg_time=1.0)

    pipe = core.G3Pipeline()
    pipe.Add(stream_root.builder)
    pipe.Add(sosmurf.SessionManager.SessionManager, stream_id=args.stream_id)
    pipe.Add(sosmurf.util.stream_dumper)
    pipe.Add(core.G3NetworkSender,
        hostname='*', port=args.stream_port, max_queue_size=1000
    )

    vgs = {
        'root.FpgaTopLevel.AppTop.AppCore.enableStreaming':
            {'groups' : ['publish','stream'], 'pollInterval': 0},
        'root.SmurfProcessor.SOStream.DebugMeta':
            {'groups' : ['publish','stream'], 'pollInterval': 0},
        'root.StreamDataSource.SourceEnable':
            {'groups' : ['publish','stream'], 'pollInterval': 0},
        'root.StreamDataSource.Period':
            {'groups' : ['publish','stream'], 'pollInterval': 0},
    }


    # Import the root device after the python path is updated
    from pysmurf.core.roots.EmulationRoot import EmulationRoot

    with EmulationRoot ( config_file    = args.config_file,
                         epics_prefix   = args.epics_prefix,
                         polling_en     = args.polling_en,
                         pv_dump_file   = args.pv_dump_file,
                         disable_bay0   = args.disable_bay0,
                         disable_bay1   = args.disable_bay1,
                         txDevice       = stream_root,
                         VariableGroups = vgs) as root:
        print("Loaded Emulation root", flush=True)

        pipe.Run(profile=True)
        
if __name__ == '__main__':
    main()
