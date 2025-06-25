SCRIPT_DIR=$(dirname "$(realpath "$0")")
# CMAKE_PREFIX_PATH="$SCRIPT_DIR/../install"

rm -rf build
rm -rf install
mkdir -p build
cd build
# cmake -DCMAKE_INSTALL_PREFIX=$CMAKE_PREFIX_PATH ..
cmake ..
# make -j
make install -j
