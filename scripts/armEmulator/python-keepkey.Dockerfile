FROM kkfirmware:v16_pidev

WORKDIR /kkemu
COPY ./ /kkemu

ENTRYPOINT ["/bin/sh", "./scripts/armEmulator/python-keepkey-tests.sh"]
