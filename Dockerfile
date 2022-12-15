FROM simonsobs/so_smurf_base:v0.0.2-1-g4a75f5b

WORKDIR /usr/local/src/

ENV SO3G_DIR /usr/local/src/so3g
ENV SPT3G_DIR /usr/local/src/spt3g_software

COPY . /usr/local/src/smurf-streamer
WORKDIR /usr/local/src/smurf-streamer/build
RUN rm -rf * && cmake ..
RUN make

RUN pip3 install dumb-init

ENV PYTHONPATH /usr/local/src/smurf-streamer/lib:${PYTHONPATH}
ENV PYTHONPATH /usr/local/src/smurf-streamer/python:${PYTHONPATH}

WORKDIR /usr/local/src/smurf-streamer
ENTRYPOINT /bin/bash
