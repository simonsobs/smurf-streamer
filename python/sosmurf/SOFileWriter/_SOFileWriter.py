#!/usr/bin/env python3

import pyrogue
from spt3g import core
import os
import time
from sosmurf.SessionManager import FlowControl

class G3Rotator:
    def __init__(self, data_path, file_dur, debug=0):
        self.data_path = data_path
        self._writer = None
        self.seq = 0
        self.cur_session_id = 0
        self.file_start_time = 0
        self.file_dur = file_dur
        self.cur_path = ''
        self.debug = debug
        self.disable = False

    def get_cur_path(self):
        return self.cur_path

    def get_session_id(self):
        return self.cur_session_id

    def get_file_dur(self):
        return self.file_dur

    def set_file_dur(self, value):
        self.file_dur = value

    def get_debug(self):
        return self.debug

    def set_debug(self, debug):
        self.debug = debug

    def get_disable(self):
        return self.disable

    def set_disable(self, value):
        self.disable = value

    def close_writer(self):
        if self._writer is not None:
            self.cur_path = ''
            self.file_start_time = 0
            self.cur_session_id = 0
            self._writer(core.G3Frame(core.G3FrameType.EndProcessing))
        self._writer = None

    def new_writer(self, frame, seq):
        session_id = frame['session_id']
        stream_id = frame['sostream_id']
        subdir = os.path.join(self.data_path, str(session_id)[:5], stream_id)
        fname = f"{session_id}_{seq:03}.g3"
        fpath = os.path.join(subdir, fname)

        if not os.path.exists(subdir):
            os.makedirs(subdir)

        self._writer = core.G3Writer(fpath)
        self.cur_path = fpath
        self.file_start_time = time.time()
        self.cur_session_id = session_id

        return self._writer

    def new_file_condition(self, frame):
        return time.time() > self.file_start_time + self.file_dur

    def get_writer(self, frame):
        """
        Gets appropriate G3 writer for a given frame based on the following
        rules. Will automatically rotate files if `new_file_condition` has
        been met, and will close out files if an FlowControl.End frame is
        received.
        """
        if 'sostream_flowcontrol' in frame:
            if frame['sostream_flowcontrol'] == FlowControl.END.value:
                self.close_writer()
            return None

        if 'session_id' not in frame:
            return None
        sess_id = frame['session_id']

        if self.cur_session_id != sess_id:
            self.close_writer()
            self.seq = 0

            return self.new_writer(frame, self.seq)
        elif self.new_file_condition(frame):
            self.close_writer()
            self.seq += 1
            return self.new_writer(frame, self.seq)
        else:
            return self._writer

    def __call__(self, frame):
        if self.disable:
            if self.debug:
                print(frame)
            return [frame]

        writer = self.get_writer(frame)
        if writer is not None:
            start = time.time()
            writer(frame)
            stop = time.time()
            if self.debug:
                print(f"Wrote frame in {stop - start} sec")
                print(frame)
        return [frame]


class SOFileWriter(pyrogue.Device):
    def __init__(self, name, data_path, file_dur=30*60):
        pyrogue.Device.__init__(self, name=name, description="G3 File Rotator")
        self.rotator = G3Rotator(data_path, file_dur)

        self.add(pyrogue.LocalVariable(
            name="disable",
            description="Disables G3Writer and passes frame to next module",
            mode='RW',
            value=False,
            localGet=self.rotator.get_disable,
            localSet=(lambda value: self.rotator.set_disable(value)),
        ))

        self.add(pyrogue.LocalVariable(
            name="filepath",
            description="Path to the current G3 file",
            mode='RO',
            value='',
            localGet=self.rotator.get_cur_path
        ))

        self.add(pyrogue.LocalVariable(
            name="base_dir",
            description="Path to the current G3 file",
            mode='RO',
            value=data_path,
        ))

        self.add(pyrogue.LocalVariable(
            name="session_id",
            description="Current stream session-id",
            mode='RO',
            value=0,
            localGet=self.rotator.get_session_id
        ))

        self.add(pyrogue.LocalVariable(
            name="file_dur",
            description="Debugs FileWriter",
            mode='RW',
            value=self.rotator.file_dur,
            localSet=lambda value: self.rotator.set_file_dur(value),
            localGet=self.rotator.get_file_dur,
        ))

        self.add(pyrogue.LocalVariable(
            name="debug",
            description="Dumps written frames to stdout",
            mode='RW',
            value=self.rotator.debug,
            localSet=lambda value: self.rotator.set_debug(value),
            localGet=self.rotator.get_debug,
        ))
