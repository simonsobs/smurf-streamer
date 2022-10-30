from spt3g import core
import pysmurf.core.server_scripts.Common as pysmurf_common
import sosmurf
import os

zip_file = '/tmp/fw/rogue_MicrowaveMuxBpEthGen2_v1.1.0.zip'

def main():

    parser = pysmurf_common.make_parser()

    parser.add_argument('--stream-port', type=int, default=4536)
    parser.add_argument('--stream-id', type=str)

    args = parser.parse_args()

    args.zip_file = zip_file
    args.stream_id = 'emulator'
    pysmurf_common.process_args(args)

    stream_root = sosmurf.StreamBase(
        "SOStream", debug_meta=False, debug_data=False, debug_builder=True,
        agg_time=1.0
    )

    g3_dir = '/data/so/timestreams'
    os.makedirs(g3_dir, exist_ok=True)
    file_writer = sosmurf.SOFileWriter("SOFileWriter", g3_dir, file_dur=10*60)
    stream_root.add(file_writer)

    pipe = core.G3Pipeline()
    pipe.Add(stream_root.builder)
    pipe.Add(sosmurf.SessionManager.SessionManager, stream_id=args.stream_id)
    pipe.Add(core.G3NetworkSender,
        hostname='*', port=args.stream_port, max_queue_size=1000
    )
    pipe.Add(file_writer.rotator)

    vgs = None
    default_meta_file = '/usr/local/src/smurf-streamer/meta_registers.yaml'
    if os.path.exists(default_meta_file):
        vgs = sosmurf.util.VariableGroups.from_file(default_meta_file)

    # Import the root device after the python path is updated
    from pysmurf.core.roots.EmulationRoot import EmulationRoot
    args.epics_prefix = 'emulator'

    with EmulationRoot ( config_file    = args.config_file,
                         epics_prefix   = args.epics_prefix,
                         polling_en     = args.polling_en,
                         pv_dump_file   = args.pv_dump_file,
                         disable_bay0   = args.disable_bay0,
                         disable_bay1   = args.disable_bay1,
                         txDevice       = stream_root,
                         VariableGroups = vgs.data) as root:
        print("Loaded Emulation root", flush=True)
        pipe.Run(profile=True)
        
if __name__ == '__main__':
    main()
