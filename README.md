# SmurfStreamer

<a href="https://github.com/simonsobs/smurf-streamer/actions/workflows/main.yml">
<img src="https://img.shields.io/github/workflow/status/simonsobs/smurf-streamer/Build%20and%20Push%20Dockers?style=for-the-badge"/>
</a>

## Description
-------------------
The G3Streamer is a module that collects downsampled data from the
[smurf-processor](https://github.com/slaclab/smurf-processor),
packages it into G3Frames and sends it over the network using the G3NetworkSender.

The smurf packet format is documented here:
[Smurf Packet Format](https://github.com/slaclab/smurf-processor/blob/master/README.SmurfPacket.md)

`make build` and `make run` will build and run the docker. 
Currently you need to change the smurf ip address `Makefile`.
You can also set the seconds per G3Frame, output port, and max stream queue length in the Makefile.
Soon this will be put into a config file somewhere.

In the file [smurf.cfg](./smurf.cfg) you can set filter parameters such as the number of averaged frames per downsampled datapoint.

[mask.txt](./mask.txt) is required to map from raw smurf channel to output channel. Currently it just maps raw channels 0-528 to the averaged channels 0-528.
