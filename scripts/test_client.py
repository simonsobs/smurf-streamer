#%% Imports
from pysmurf.client.base.smurf_control import SmurfControl
import os
import numpy as np
import sodetlib as sdl
from tqdm import tqdm, trange

#%%
class Registers:
    _root = 'AMCc:'
    _processor = _root + "SmurfProcessor:"
    _sostream = _processor + "SOStream:"
    _sofilewriter = _sostream + 'SOFileWriter:'
    _source_root = _root + 'StreamDataSource:'
    pysmurf_action =  _sostream + 'pysmurf_action'
    pysmurf_action_timestamp =  _sostream + "pysmurf_action_timestamp"
    stream_tag =  _sostream + "stream_tag"
    open_g3stream =  _sostream + "open_g3stream"
    g3_session_id =  _sofilewriter + "session_id"
    debug_data = _sostream + "DebugData"
    debug_meta = _sostream + "DebugMeta"
    debug_builder = _sostream + "DebugBuilder"
    flac_level = _sostream + "FlacLevel"
    source_enable = _source_root + 'SourceEnable'
    enable_compression = _sostream + 'EnableCompression'
    agg_time = _sostream + 'AggTime'

def set_reg(S: SmurfControl, reg, val):
    print(S.epics_root, reg)
    _reg = ':'.join([S._epics_root, reg])
    return S._caput(_reg, val)

def get_reg(S: SmurfControl, reg):
    _reg = ':'.join([S._epics_root, reg])
    return S._caget(_reg, use_monitor=False)

#%%

epics_root="emulator"
cfg_file = '/usr/local/src/pysmurf/cfg_files/template/template.cfg'
os.makedirs('/data/smurf_data', exist_ok=True)
os.makedirs('/data/smurf_data/tune', exist_ok=True)
os.makedirs('/data/smurf_data/status', exist_ok=True)
S = SmurfControl(epics_root=epics_root, cfg_file=cfg_file)

#%%
set_reg(S, Registers.debug_builder, 1)

#%%
S.set_channel_mask(np.arange(2000))
#%%
S.set_stream_data_source_period(1e-6)
#%%
sdl.stream_g3_on(S, emulator=True)
#%%
S.set_postdata_emulator_type('Noise')

#%%
from epics import caget, caput
#%%
set_reg(S, Registers.agg_time, 10)
    
# %%
