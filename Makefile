IMAGE = g3stream-processor
TAG = latest
NAME = g3stream-processor

DOCKER_RUN_FLAGS = -it --rm
DOCKER_RUN_FLAGS += --name $(NAME) --net host  -e DISPLAY=${DISPLAY}
DOCKER_RUN_FLAGS += -v /home/${user}:/home/${user} -v /data:/data -v ${PWD}/fw:/tmp/fw/
DOCKER_RUN_FLAGS += -v ${PWD}/scripts/:/usr/local/src/G3StreamWriter/scripts/
DOCKER_RUN_FLAGS += -v ${PWD}/smurf.cfg:/usr/local/src/G3StreamWriter/smurf.cfg
DOCKER_RUN_FLAGS += -v ${PWD}/mask.txt:/usr/local/src/G3StreamWriter/mask.txt
DOCKER_RUN_FLAGS += -v ${PWD}/config.txt:/usr/local/src/G3StreamWriter/config.txt

START_SCRIPT = /usr/local/src/G3StreamWriter/scripts/control-server/start_server.sh
SMURF_FLAGS = -D -a 192.168.2.20 -e smurf_server -c eth-rssi-interleaved -d /tmp/fw/config/defaults.yml -f Int16 -b 524288
SMURF_FLAGS += --stream-config /usr/local/src/G3StreamWriter/config.txt

build:
	docker build -t $(IMAGE):$(TAG) .

run:
	docker run $(DOCKER_RUN_FLAGS) $(IMAGE):$(TAG) $(START_SCRIPT) $(SMURF_FLAGS)

run_shell:
	docker run $(DOCKER_RUN_FLAGS) $(IMAGE):$(TAG)
