# mu-app

This repository is the application level repo for using the midas unpacker.

##  Cloning the repository and its submodules

To clone the repository and its submodules, use the following command:
```bash
git clone --recurse-submodules https://github.com/sbfoster12/mu-unpackers.git
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
source ./sripts/setenv.sh
```
This script will source the setup scripts for the submodules as well.

Once the environment is configured, the application can be built with the following command:
```bash
./scripts/build_all.sh
```
This will build all the submodules and the main application, which depends on the submodules.