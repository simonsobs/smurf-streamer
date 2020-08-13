from spt3g import core
import yaml
import time
from enum import Enum

class FlowControl(Enum):
    """Flow control enumeration."""
    ALIVE = 0
    START = 1
    END = 2
    CLEANSE = 3


class SessionManager:
    #enable_streams = "AMCc.FpgaTopLevel.AppTop.AppCore.StreamControl.EnableStreams"
    enable_streams = "AMCc.FpgaTopLevel.AppTop.AppCore.enableStreaming"

    def __init__(self, stream_id=None):
        self.stream_id = stream_id
        self.session_id = None
        self.end_session_flag = False
        self.frame_num = 0
        self.status = {}

    def flowcontrol_frame(self, fc):
        """
        Creates flow control frame.

        Args:
            fc (int): 
                flow control type
        """
        frame = core.G3Frame(core.G3FrameType.none)
        frame['sostream_flowcontrol'] = fc.value
        return frame

    def status_frame(self):
        frame = core.G3Frame(core.G3FrameType.Wiring)
        if self.stream_id is not None:
            frame['sostream_id'] = self.stream_id
        frame['status'] = yaml.safe_dump(self.status)
        frame['session_id'] = self.session_id
        frame['time'] = core.G3Time.Now()
        frame['frame_num'] = self.frame_num
        frame['dump'] = 1

        self.frame_num += 1

        return frame

    def start_session(self):
        self.session_id = int(time.time())

        frame = core.G3Frame(core.G3FrameType.Observation)

        if self.stream_id is not None:
            frame['sostream_id'] = self.stream_id

        frame['session_id'] = self.session_id
        frame['time'] = core.G3Time.Now()
        frame['frame_num'] = self.frame_num
        self.frame_num += 1

        return frame

    def __call__(self, frame):
        out = [frame]

        #######################################
        # On None frames
        #######################################
        if frame.type == core.G3FrameType.none:

            if self.end_session_flag:
                # Returns [previous, end, obs cleanse, wiring cleanse]
                out = []
                out.append(self.flowcontrol_frame(FlowControl.END))

                f = core.G3Frame(core.G3FrameType.Observation)
                f['sostream_flowcontrol'] = FlowControl.CLEANSE.value
                out.append(f)

                f = core.G3Frame(core.G3FrameType.Wiring)
                f['sostream_flowcontrol'] = FlowControl.CLEANSE.value
                out.append(f)

                self.session_id = None
                self.end_session_flag = False
                self.frame_num = 0

            return out

        #######################################
        # On Scan frames
        #######################################
        elif frame.type == core.G3FrameType.Scan:

            if self.session_id is None:
                # Returns [start, session, data]
                session_frame = self.start_session()
                status_frame = self.status_frame()
                out = [
                    self.flowcontrol_frame(FlowControl.START),
                    session_frame,
                    status_frame,
                    frame
                ]

            frame['session_id'] = self.session_id
            frame['frame_num'] = self.frame_num
            self.frame_num += 1

            return out

        #######################################
        # On Wiring frames
        #######################################
        elif frame.type == core.G3FrameType.Wiring:

            status_update = yaml.safe_load(frame['status'])
            self.status.update(status_update)

            enable = int(status_update.get(self.enable_streams, -1))
            if self.session_id is None:
                if enable == 1:
                    # Returns [start, session, status]
                    session_frame = self.start_session()
                    out = [
                        self.flowcontrol_frame(FlowControl.START),
                        session_frame,
                        self.status_frame()
                    ]

                    return out
                else:
                    # Don't output any status frames if session is not active
                    return []

            else:
                frame['dump'] = 0
                frame['session_id'] = self.session_id
                frame['frame_num'] = self.frame_num
                self.frame_num += 1

                if enable == 0:
                    self.end_session_flag = True
                return out


