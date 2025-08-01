# mu-app

This repository is the application level repo for using the midas unpacker.

##  Cloning the repository and its submodules

To clone the repository and its submodules, use the following command:
```bash
git clone --recurse-submodules https://github.com/sbfoster12/mu-app.git
```

You can udpate the submodules later with:
```bash
git pull --recurse-submodules
git submodule update --recursive --remote
```

## Development

Each of the submodules will be in a detached HEAD state. To work on a submodule, you will need to checkout a branch. To check out the main branch, do the following:
```bash
cd mu-data-products
git checkout main
git pull
```
Do so for all relevant submodules.

## Building

Before building, you should first set up the environment. This can be done by sourcing the `setenv.sh` script:
```bash
source ./scripts/setenv.sh
```
This script will source the setup scripts for the submodules as well.

Once the environment is configured, the application can be built with the following command:
```bash
./scripts/build_all.sh
```
This will build all the submodules and the main application, which depends on the submodules.

## Running the application

Building the application will create two executables, one to unpack midas files with WFD5 payloads and the other to unpack midas files with NALU payloads.

Assuming that `MY_FILE.mid` is your midas file, you can run the unpacker via (for WFD5 payloads):
```bash
./install/bin/mu_app_wfd5 MY_FILE.mid 0
```
or (for NALU payloads):
```bash
./install/bin/mu_app_nalu MY_FILE.mid 0
``` 
where the `0` is the verbosity level. 

Running these commands will create a ROOT file that you can use for all your analysis needs!