#!/bin/bash
cat payload.bin |& sudo tee 2>&1 1>/dev/null /dev/injector
