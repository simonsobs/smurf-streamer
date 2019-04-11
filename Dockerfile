FROM smurf-g3-base:latest

COPY . G3StreamWriter

WORKDIR G3StreamWriter/build
RUN cmake .. && make

WORKDIR ..

ENV PYTHONPATH /usr/local/src/G3StreamWriter/python:${PYTHONPATH}
ENV PATH /usr/local/src/smurf-processor-example/scripts/control-server:${PATH}
