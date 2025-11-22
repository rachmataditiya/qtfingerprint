#!/bin/bash
cd "$(dirname "$0")"
export LD_LIBRARY_PATH=./bin:./digitalpersonalib/lib:$LD_LIBRARY_PATH
./bin/FingerprintApp

