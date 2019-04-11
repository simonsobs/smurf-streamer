#!/usr/bin/env bash

user="cryo"

docker run -it --rm  \
  --net host \
  -e DISPLAY=${DISPLAY} \
  -v /home/${user}:/home/${user} \
  -v /data:/data \
  -v ${PWD}/fw:/tmp/fw/ \
  smurf-custom-processor:latest \
  /usr/local/src/smurf-processor-example/scripts/control-server/start_server.sh -D -a 192.168.2.20 -e smurf_server -c eth-rssi-interleaved -d /tmp/fw/config/defaults.yml -f Int16 -b 524288
