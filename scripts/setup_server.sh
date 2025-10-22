#!/usr/bin/env bash

. /usr/local/src/pysmurf/docker/server/scripts/server_common.sh

arg_parser args "$@"

initialize extra_args

args+=" ${extra_args}"

echo "OUTPUT={\"NEW_ARGS\": \"${args}\", \"PYTHONPATH\": \"${PYTHONPATH}\"}"
