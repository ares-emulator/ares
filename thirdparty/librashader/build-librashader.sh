#!/bin/bash

# On non-macOS posix systems, we can check for the presence of librashader using pkg-config
if [ "$(uname)" != "Darwin" ] && [ "$(uname)" != "Windows_NT" ]; then
    if pkg-config --exists librashader; then
        echo "librashader is already installed on the system. Skipping build."
        exit 0
    fi
fi

# Check for the presence of rustup to verify that we have a working rust installation
if ! command -v rustup &> /dev/null; then
    echo "rustup not found. Please install rustup from https://rustup.rs/. If prompted, please install the nightly toolchain."
    exit 1
fi

# Check for a nightly toolchain
if ! rustup toolchain list | grep -q nightly; then
    echo "No nightly toolchain installed. Please run 'rustup toolchain install nightly' to install it."
    exit 1
fi

# If we're on macOS, we need to build for both x86_64 and arm64 targets and merge into a universal binary
if [ "$(uname)" = "Darwin" ]; then
    # Check for the presence of the x86_64 and aarch64 targets
    if ! rustup target list --installed | grep -q x86_64-apple-darwin; then
        echo "x86_64-apple-darwin target not found. Please run 'rustup target add x86_64-apple-darwin --toolchain nightly' to install it."
        exit 1
    fi

    if ! rustup target list --installed | grep -q aarch64-apple-darwin; then
        echo "aarch64-apple-darwin target not found. Please run 'rustup target add aarch64-apple-darwin --toolchain nightly' to install it."
        exit 1
    fi

    cargo +nightly run -p librashader-build-script -- --profile optimized --target x86_64-apple-darwin
    cargo +nightly run -p librashader-build-script -- --profile optimized --target aarch64-apple-darwin
    lipo -create -output target/optimized/librashader.dylib target/x86_64-apple-darwin/optimized/librashader.dylib target/aarch64-apple-darwin/optimized/librashader.dylib
    rm target/x86_64-apple-darwin/optimized/librashader.dylib target/aarch64-apple-darwin/optimized/librashader.dylib
else
    # If a parameter is passed, we build for the specified target
    if [ $# -eq 1 ]; then
        cargo +nightly run -p librashader-build-script -- --profile optimized --target $1
    else
        cargo +nightly run -p librashader-build-script -- --profile optimized
    fi

    if [ "$(uname)" = "Linux" ]; then
        echo "\nlibrashader built successfully, to install, please run the following commands:"
        echo "!IMPORTANT!: Make sure to substitute the correct target paths for your system"
        echo "sudo cp target/optimized/librashader.so /usr/local/lib/"
        echo "sudo cp target/optimized/librashader.so /usr/local/lib/librashader.so.2"
        echo "sudo cp pkg/librashader.pc /usr/local/lib/pkgconfig/"
    fi
fi

