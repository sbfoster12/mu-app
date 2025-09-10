BUILD_DATA_PRODUCTS=false
BUILD_UNPACKERS=false
BUILD_RECO=false

# Help message
show_help() {
    echo "Usage: ./build_all.sh [OPTIONS]"
    echo
    echo "Options:"
    echo "  -a, --all                   Build all components"
    echo "  -d, --dataproducts          Build the data products"
    echo "  -u, --unpackers             Build the unpackers"
    echo "  -r, --reco                  Build the reco"
    echo "  -j, --jobs <number>         Number of jobs"
    echo "  -h, --help                  Display this help message"
}

JOBS_ARG="-j"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -d|--dataproducts)
            BUILD_DATA_PRODUCTS=true
            shift
            ;;
        -u|--unpackers)
            BUILD_UNPACKERS=true
            shift
            ;;
        -r|--reco)
            BUILD_RECO=true
            shift
            ;;
        -a|--all)
            BUILD_DATA_PRODUCTS=true
            BUILD_UNPACKERS=true
            BUILD_RECO=true
            shift
            ;;
        -j|--jobs)
            if [[ -n "$2" && "$2" != -* ]]; then
                JOBS_ARG="-j $2"
                shift 2
            else
                JOBS_ARG="-j"
                shift
            fi
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "[build_all.sh, ERROR] Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

echo "Build all with job argument: ${JOBS_ARG}"

# If not argument were provided, default to building all components
if [ "$BUILD_DATA_PRODUCTS" = false ] && [ "$BUILD_UNPACKERS" = false ] && [ "$BUILD_RECO" = false ]; then
    BUILD_DATA_PRODUCTS=true
    BUILD_UNPACKERS=true
    BUILD_RECO=true
    echo "[build_all.sh] No specific packages specified, so building all packages."
fi

# Check some things. For instance, if you rebuild the data products, you should also rebuild everything!
if [ "$BUILD_DATA_PRODUCTS" = true ]; then
    if [ "$BUILD_UNPACKERS" = false ] || [ "$BUILD_RECO" = false ]; then
        echo "[build_all.sh, WARNING] Force rebuilding unpackers and reco since data products are being rebuilt."
        BUILD_UNPACKERS=true
        BUILD_RECO=true
    fi
fi

# Check if mu-unpackers is being rebuilt
if [ "$BUILD_UNPACKERS" = true ]; then
    if [ "$BUILD_RECO" = false ]; then
        echo "[build_all.sh, WARNING] Force rebuilding reco since unpackers are being rebuilt."
        BUILD_RECO=true
    fi
fi


# Build data-products
if [ "$BUILD_DATA_PRODUCTS" = true ]; then
    echo "[build_all.sh] Building data products..."
    ./mu-data-products/scripts/build.sh -o ${JOBS_ARG}
else
    echo "[build_all.sh] Skipping data products build."
fi

# Build unpackers
if [ "$BUILD_UNPACKERS" = true ]; then
    echo "[build_all.sh] Building unpackers..."
    ./mu-unpackers/scripts/build.sh -o ${JOBS_ARG}
else
    echo "[build_all.sh] Skipping unpackers build."
fi

# Build reco
if [ "$BUILD_RECO" = true ]; then
    echo "[build_all.sh] Building reco..."
    ./mu-reco/scripts/build.sh -o ${JOBS_ARG}
else
    echo "[build_all.sh] Skipping reco build."
fi

# Build the app always
echo "[build_all.sh] Building the app..."
./scripts/build.sh -o ${JOBS_ARG}