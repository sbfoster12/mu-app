#!/bin/bash

# Get absolute path to the root of the repo (assuming this script is in scripts/)
export APP_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Source environment variables for submodules but first check if they exist
if [ -f "$APP_PATH/mu-data-products/scripts/unsetenv.sh" ]; then
    source "$APP_PATH/mu-data-products/scripts/unsetenv.sh"
else
    echo "[setenv.sh, ERROR] mu-data-products scripts not found. Please ensure the mu-data-products submodule is initialized and updated."
    exit 1
fi  

if [ -f "$APP_PATH/mu-unpackers/scripts/unsetenv.sh" ]; then
    source "$APP_PATH/mu-unpackers/scripts/unsetenv.sh"
else
    echo "[setenv.sh, ERROR] mu-unpackers scripts not found. Please ensure the mu-unpackers submodule is initialized and updated."
    exit 1
fi  

#reco
if [ -f "$APP_PATH/mu-reco/scripts/unsetenv.sh" ]; then
    source "$APP_PATH/mu-reco/scripts/unsetenv.sh"
else
    echo "[setenv.sh, ERROR] mu-reco scripts not found. Please ensure the mu-reco submodule is initialized and updated."
    exit 1
fi