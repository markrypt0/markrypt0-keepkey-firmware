

Overview
--------

It is possible to build the firmware on an ARM-based system with the pi-dev branch. It was developed specifically for a raspbery pi 5 running Bookworm but in general should work for any ARM-based build system.
The standard build uses a docker container to isolate the raspi os, but theoretically this is not required. The dockerized environment has some patches to surpress moot warnings from the linker and does some other tricks to make the build clean.
The pi-dev branch has been refactored to simplify the build, namely the crypto library. No longer is the entire trezor monorepo pulled into the firmware build just to use some of the trezor crypto library. The crypto library submodule is now called hw-crypto, which was copied directly from the keepkey branch of the trezor crypto directory.

Requirements for pi environment
-------------------------------
docker
docker-compose (if you want to run the emulator for testing)

Install
-------
```sh
$ git clone https://github.com/markrypt0/keepkey-firmware.git
$ git checkout pi-dev
$ git submodule update --init --recursive
```

Build the docker container from keepkey-firmware root directory
```sh
$ ./pidevDockerStart
```

Build firmware
```sh
$ ./scripts/build/docker/device/armdebug.sh
```
