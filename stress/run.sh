#!/usr/bin/env bash
# Convenience wrapper for the Go stress client.
#
#   ./run.sh concurrent [clients] [pings-each]
#   ./run.sh large       [megabytes]
#   ./run.sh partial     [delay-ms]
#   ./run.sh pipeline    [num-commands]
#   ./run.sh all
#
# Override the target with ADDR=host:port ./run.sh ...
#
# `env -u GOROOT` works around a stale exported GOROOT (gvm sets it to a Go
# version that doesn't match the `go` binary on PATH).
cd "$(dirname "$0")" || exit 1
exec env -u GOROOT go run . "$@"
