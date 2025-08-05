# Build data-products
echo "[build_all.sh] Building data products..."
./mu-data-products/scripts/build.sh -o

# Build unpackers
echo "[build_all.sh] Building unpackers..."
./mu-unpackers/scripts/build.sh -o

# Build reco
echo "[build_all.sh] Building reco..."
./mu-reco/scripts/build.sh -o

# Build the app
echo "[build_all.sh] Building the app..."
./scripts/build.sh -o