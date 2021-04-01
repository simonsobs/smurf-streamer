FROM simonsobs/spt3g:0.3-23-gd903080 AS spt3g_base
WORKDIR /app_lib/spt3g_software/build

FROM tidair/pysmurf-server:v5.0.0

WORKDIR /usr/local/src/

COPY --from=spt3g_base /app_lib/spt3g_software /usr/local/src/spt3g_software

ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

ENV SPT3G_SOFTWARE_PATH /usr/local/src/spt3g_software
ENV SPT3G_SOFTWARE_BUILD_PATH /usr/local/src/spt3g_software/build
ENV PYTHONPATH /usr/local/src/spt3g_software/build:${PYTHONPATH}

COPY . /usr/local/src/smurf-streamer
WORKDIR /usr/local/src/smurf-streamer/build

RUN cmake ..
RUN make

ENV PYTHONPATH /usr/local/src/smurf-streamer/lib:${PYTHONPATH}
ENV PYTHONPATH /usr/local/src/smurf-streamer/python:${PYTHONPATH}

WORKDIR /usr/local/src/smurf-streamer
