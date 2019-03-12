from spt3g import core
from datetime import datetime
import os

class StreamListener:
    def __init__(self, data_dir="data/", port=4536, time_per_file=60*60):
        """
            A class that listens to a socket for G3Frames, and writes them to disk.

            Args:
                data_dir (string): Directory where data files will be written
                port (int):  Port to listen to G3 frames
                time_per_file (int): Time before switching output files (seconds)
        """
        self.ddir = data_dir
        if not os.path.exists(self.ddir):
            os.makedirs(self.ddir)

        self.reader = core.G3Reader(f"tcp://localhost:{port}")
        self.writer = None
        self.time_per_file = time_per_file

    def end_file(self):
        if self.writer is not None:
            self.writer(core.G3Frame(core.G3FrameType.EndProcessing))
        self.writer = None

    def run(self):
        while True:
            if self.writer is None:
                file_start_time = datetime.utcnow()
                ts = file_start_time.timestamp()
                fname = file_start_time.strftime("%Y-%m-%d-%H-%M-%S.g3")
                fpath = os.path.join(self.ddir, fname)
                self.writer = core.G3Writer(filename=fpath)

            frames = self.reader.Process(None)
            for f in frames:
                self.writer(f)

            if (datetime.utcnow().timestamp() - ts) > self.time_per_file:
                self.end_file()

if __name__ == '__main__':
    listener = StreamListener()
    try:
        listener.run()
    except KeyboardInterrupt:
        listener.end_file()
