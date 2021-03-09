FROM spt3g-pysmurf-server-base

COPY . /usr/local/src/smurf-streamer
WORKDIR /usr/local/src/smurf-streamer/build
RUN cmake ..
RUN make

ENV PYTHONPATH /usr/local/src/smurf-streamer/lib:${PYTHONPATH}
ENV PYTHONPATH /usr/local/src/smurf-streamer/python:${PYTHONPATH}

COPY meta_registers.yaml /tmp/meta_registers.yaml

WORKDIR /usr/local/src/smurf-streamer
