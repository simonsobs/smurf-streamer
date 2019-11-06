FROM tidair/pysmurf-server-base:v4.0.0-rc3

# Installs spt3g to /usr/local/src/
WORKDIR /usr/local/src/
ENV TZ=America/New_York

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y \
    libflac-dev \
    libnetcdf-dev \
    libfftw3-dev \
    libgsl0-dev \
    tcl \
    environment-modules \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# Kind of a sketchy way to do this, but builds really quickly.
COPY --from=simonsobs/spt3g:61a4c7b /app_lib/spt3g_software /usr/local/src/spt3g_software

ENV SPT3G_SOFTWARE_PATH /usr/local/src/spt3g_software
ENV SPT3G_SOFTWARE_BUILD_PATH /usr/local/src/spt3g_software/build
ENV PYTHONPATH /usr/local/src/spt3g_software/build:${PYTHONPATH}

#
# RUN git clone https://github.com/CMB-S4/spt3g_software.git \
#     && cd spt3g_software \
#     && mkdir -p build \
#     && cd build \
#     && cmake .. -DPYTHON_EXECUTABLE=`which python3` \
#     && make


COPY . /usr/local/src/smurf-streamer
WORKDIR /usr/local/src/smurf-streamer/build
RUN cmake .. && make

ENV PYTHONPATH /usr/local/src/smurf-streamer/lib:${PYTHONPATH}
ENV PYTHONPATH /usr/local/src/smurf-streamer/python:${PYTHONPATH}

WORKDIR /usr/local/src/smurf-streamer
