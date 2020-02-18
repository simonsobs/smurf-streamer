from spt3g import core
import yaml
import time

class SessionManager:
    enable_streams = "AMCc.FpgaTopLevel.AppTop.AppCore.StreamControl.EnableStreams"

    def __init__(self, stream_id=None):
        self.stream_id = stream_id
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
        if self.stream_id is not None:
            frame['sostream_id'] = self.stream_id

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
            if self.stream_id is not None:
                frame['sostream_id'] = self.stream_id
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
        # and insert fc frame before data.
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

            # Updates status each wiring frame
            self.status.update(yaml.safe_load(frame['status']))
            enable = self.status.get(self.enable_streams)

            # When no session is active, if enable is specified start a session,
            # and if its not don't transmit frame.
            if self.session_id is None:
                if enable:
                    self.session_id = time.time()
                    out.append(self.flowcontrol_frame(True))
                else:
                    return []

            # If a session is active:
            #   - If enable is not specified, pass frame through
            #   - If enable is false, flag end of session. An end session frame
            #     will be sent the next time an idle frame is received. 
            else:  
                if enable is None:
                    return out
                elif not enable:
                    self.end_session = True
                    return out
