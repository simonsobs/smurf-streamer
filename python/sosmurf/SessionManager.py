from spt3g import core
import yaml
import time

class SessionManager:
    enable_streams = "AMCc.FpgaTopLevel.AppTop.AppCore.StreamControl.EnableStreams"

    def __init__(self):
        self.session_id = None
        self.end_session = False
        self.frame_num = 0
        self.status = {}

    def flowcontrol_frame(self, start):
        """
        Creates flow control frame.

        Args:
            start (bool): true if start frame, false if end frame
        """
        frame = core.G3Frame(core.G3FrameType.Observation)
        frame['sostream_flowcontrol'] = (1 if start else 2)
        frame['session_id'] = self.session_id
        frame['time'] = core.G3Time.Now()
        frame['frame_num'] = self.frame_num
        self.frame_num += 1
        return frame

    def __call__(self, frame):
        out = [frame]

        # If end_session is flagged, send end frame once an idle frame has
        # been received
        if frame.type == core.G3FrameType.none:
            if self.end_session:
                out.insert(0, self.flowcontrol_frame(start=False))
                self.session_id = None
                self.end_session = False
                self.frame_num = 0
            return out

        if self.session_id is not None:
            frame['session_id'] = self.session_id
            frame['frame_num'] = self.frame_num
            self.frame_num += 1

        # If its a data frame, start a new session if one is not active
        # and insert fc frame
        if frame.type == core.G3FrameType.Scan:
            if self.session_id is None:
                self.session_id = time.time()
                out.insert(0, self.flowcontrol_frame(start=True))

                frame['session_id'] = self.session_id
                frame['frame_num'] = self.frame_num
                self.frame_num += 1

            return out

        # If status frame, update status dict, check to see if enableStreams has
        # changed and act accordingly
        if frame.type == core.G3FrameType.Wiring:
            self.status.update(yaml.safe_load(frame['status']))
            enable = self.status.get(self.enable_streams)

            if enable is None:
                return out

            if enable and (self.session_id is None):
                self.session_id = time.time()
                out.append(self.flowcontrol_frame(True))

            if not enable and (self.session_id is not None):
                self.end_session = True

            return out
