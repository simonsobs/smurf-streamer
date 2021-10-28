FROM tidair/pysmurf-server:v5.0.2

# Installs spt3g to /usr/local/src/
WORKDIR /usr/local/src
ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update --fix-missing \
    && apt-get install -y \
        libflac-dev \
        libnetcdf-dev \
        libfftw3-dev \
        libgsl0-dev \
        tcl \
        environment-modules \
        gdb \
        rsync \
        libopenblas-dev \
        build-essential \
        automake \
        gfortran \
        libbz2-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/CMB-S4/spt3g_software.git

RUN cd spt3g_software \
      && mkdir -p build \
      && cd build \
      && cmake .. -DPYTHON_EXECUTABLE=`which python3` \
      && make core version

ENV SPT3G_SOFTWARE_PATH /usr/local/src/spt3g_software
ENV SPT3G_SOFTWARE_BUILD_PATH /usr/local/src/spt3g_software/build
ENV PYTHONPATH /usr/local/src/spt3g_software/build:${PYTHONPATH}

# SO3G install
WORKDIR /usr/local/src
RUN git clone https://github.com/simonsobs/so3g.git
ENV LANG C.UTF-8
WORKDIR /usr/local/src/so3g

RUN pip3 install -r requirements.txt
RUN mkdir -p build \
    && cd build \
    && cmake .. -DCMAKE_PREFIX_PATH=$SPT3G_SOFTWARE_BUILD_PATH \
    && make -j 16 \
    && make install

env SO3G_DIR /usr/local/src/so3g

# Install pysmurf-client requirements so we can use the client-version of the
# pysmurf publisher
RUN pip3 install -r /usr/local/src/pysmurf/requirements.txt
