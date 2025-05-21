#!/bin/bash

NICE_VAL=-20
APP_DIR="/home/sarv-pi/RGBCapture"
APP="./RGBCaptureAndSave"

cd "$APP_DIR" || exit 1
sudo nice -n "$NICE_VAL" "$APP"
