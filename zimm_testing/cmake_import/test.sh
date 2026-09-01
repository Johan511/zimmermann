#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE="zimm-import-tests"
RESULTS_DIR="$SCRIPT_DIR/from_docker"
CONTAINER=zimm-import-run

CONNECT=false
if [[ "${1:-}" == "--connect" ]]; then
  CONNECT=true
fi

docker build -f "$SCRIPT_DIR/Dockerfile" -t "$IMAGE" "$PROJECT_ROOT"

docker rm -f "$CONTAINER" &> /dev/null || true

if [ "$CONNECT" = "true" ]; then
    docker run -d --name "$CONTAINER" "$IMAGE" \
    sh -c '
            python3 /opt/harness/run_tests.py
            echo "$?" > /tmp/test_rc
            sleep infinity
          '

    # wait for tests to finish and get return code
    while [ -z "$(docker exec "$CONTAINER" cat /tmp/test_rc)" ]; do
        sleep 1
    done
    rc=$(docker exec "$CONTAINER" cat /tmp/test_rc)
else
    docker run --name "$CONTAINER" "$IMAGE" python3 /opt/harness/run_tests.py || rc=$?
fi

rm -rf "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR"
docker cp "$CONTAINER":/work/build/. "$RESULTS_DIR/"

if [ "$CONNECT" = "true" ]; then
    docker exec -it "$CONTAINER" /bin/bash
fi

exit "${rc:-0}"
