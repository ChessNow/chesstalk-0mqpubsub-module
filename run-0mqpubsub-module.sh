#!/bin/bash

CHESSTALK=$HOME/bin/chesstalk/bin/chesstalk

export PUB_ADDRESS=192.168.0.100
export PUB_PORT=4920
CHESSTALK_MODULE=module/0mqpubsub/.libs/libchesstalk_0mqpubsub.so "$CHESSTALK"

