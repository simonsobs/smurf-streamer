#FROM sodetlib:latest
FROM simonsobs/sodetlib:DEV-so-docker-builds

WORKDIR /usr/local/src/

ENV SO3G_DIR=/app_lib/so3g
COPY . /usr/local/src/smurf-streamer
WORKDIR /usr/local/src/smurf-streamer/build
RUN cmake ..
RUN make

RUN pip3 install dumb-init

ENV PYTHONPATH /usr/local/src/smurf-streamer/lib:${PYTHONPATH}
ENV PYTHONPATH /usr/local/src/smurf-streamer/python:${PYTHONPATH}

WORKDIR /usr/local/src/smurf-streamer
ENTRYPOINT /bin/bash
