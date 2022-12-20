#!/bin/bash

set -e

echo "*******************************************************************************"
echo "* SKFS TESTS *"
echo "*******************************************************************************"
echo ""

build/skdb --all

echo "*******************************************************************************"
echo "* WASM TESTS *"
echo "*******************************************************************************"
echo ""

# node run.js
echo "TODO: re-enable me"
# TODO: disabled as, under docker, these result in a LinkError.

echo "*******************************************************************************"
echo "* NATIVE TESTS *"
echo "*******************************************************************************"
echo ""

(cd sql/ && ./test_sql.sh)