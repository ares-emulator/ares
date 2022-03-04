#! /bin/bash
make -j$(nproc) -C desktop-ui build=optimized local=false lto=true compiler=g++
