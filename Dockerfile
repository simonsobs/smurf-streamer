FROM spt3g-pysmurf-server-base

WORKDIR /usr/local/src/

COPY . /usr/local/src/smurf-streamer
WORKDIR /usr/local/src/smurf-streamer/build
RUN rm -rf * && cmake ..
RUN make

RUN pip3 install dumb-init

ENV PYTHONPATH /usr/local/src/smurf-streamer/lib:${PYTHONPATH}
ENV PYTHONPATH /usr/local/src/smurf-streamer/python:${PYTHONPATH}

WORKDIR /usr/local/src/smurf-streamer
ENTRYPOINT /bin/bash
