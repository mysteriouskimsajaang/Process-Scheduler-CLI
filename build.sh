#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -Wextra -pthread src/main.cpp src/scheduler.cpp -o csopesy
printf 'Built ./csopesy\n'
