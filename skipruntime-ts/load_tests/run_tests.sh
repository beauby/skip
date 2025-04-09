#!/bin/sh

set -e

COMMIT_HASH=$(git rev-parse --short HEAD)

echo "Building skiplang base image..."
docker build -t skiplabs/skip:${COMMIT_HASH} ../../
echo -e "Done building skiplang base image.\n"

echo "Building load test base image..."
docker build \
       --build-arg COMMIT_HASH=${COMMIT_HASH} \
       -f ./Dockerfile \
       -t skiplabs/skip-load-tests-base:${COMMIT_HASH} \
       ../../
echo -e "Done building load test base image.\n"

for t in $(find ./tests -mindepth 1 -maxdepth 1 -type d)
do
    test_name=$(basename "$t")
    echo "Building docker image for '${test_name}'..."
    docker build \
           --build-arg COMMIT_HASH=${COMMIT_HASH} \
           -t "skiplabs/skip-load-tests_${test_name}:${COMMIT_HASH}" \
           "$t/server"
    docker push "skiplabs/skip-load-tests_${test_name}:${COMMIT_HASH}"
done
