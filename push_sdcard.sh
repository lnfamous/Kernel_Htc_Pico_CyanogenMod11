#!/bin/bash

mkaflash

mv output.zip releases/output_`date +%d_%b-%H%M%S``scripts/setlocalversion`.zip

adb push releases/output_`date +%d_%b-%H%M%S``scripts/setlocalversion`.zip /sdcard/cody/output_`date +%d_%b-%H%M%S``scripts/setlocalversion`.zip


