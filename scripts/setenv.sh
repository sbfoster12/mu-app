#!/bin/bash

# Get absolute path to the root of the repo (assuming this script is in scripts/)
export APP_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

#First unsetup environment variables
source "$APP_PATH/scripts/unsetenv.sh"

# Source environment variables for submodules but first check if they exist
if [ -f "$APP_PATH/mu-data-products/scripts/setenv.sh" ]; then
    source "$APP_PATH/mu-data-products/scripts/setenv.sh"
else
    echo "[setenv.sh, ERROR] mu-data-products scripts not found. Please ensure the mu-data-products submodule is initialized and updated."
    exit 1
fi  

if [ -f "$APP_PATH/mu-unpackers/scripts/setenv.sh" ]; then
    source "$APP_PATH/mu-unpackers/scripts/setenv.sh"
else
    echo "[setenv.sh, ERROR] mu-unpackers scripts not found. Please ensure the mu-unpackers submodule is initialized and updated."
    exit 1
fi  