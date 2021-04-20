FROM tidair/pysmurf-server:v4.0.0

# Installs spt3g to /usr/local/src/
WORKDIR /usr/local/src/
ENV TZ=America/New_York

RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
# RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 6B05F25D762E3157
RUN apt-get update

RUN apt-get install -y \
    libflac-dev \
    libnetcdf-dev \
    libfftw3-dev \
    libgsl0-dev \
    tcl \
    environment-modules \
    gdb \
    rsync \
    && rm -rf /var/lib/apt/lists/*


RUN git clone https://github.com/CMB-S4/spt3g_software.git

#WORKDIR /usr/local/src/spt3g_software/build
#COPY tarballs/spt3g.tgz .
#RUN tar zvxf spt3g.tgz

RUN cd spt3g_software \
      && mkdir -p build \
      && cd build \
      && cmake .. -DPYTHON_EXECUTABLE=`which python3` \
      && make core version


ENV SPT3G_SOFTWARE_PATH /usr/local/src/spt3g_software
ENV SPT3G_SOFTWARE_BUILD_PATH /usr/local/src/spt3g_software/build
ENV PYTHONPATH /usr/local/src/spt3g_software/build:${PYTHONPATH}
