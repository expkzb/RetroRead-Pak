#!/usr/bin/env bash
set -euo pipefail

echo "OS_RELEASE_BEGIN"
cat /etc/os-release
echo "OS_RELEASE_END"

echo "DOCKER_LIST_BEGIN"
if [[ -f /etc/apt/sources.list.d/docker.list ]]; then
  cat /etc/apt/sources.list.d/docker.list
else
  echo "missing"
fi
echo "DOCKER_LIST_END"

echo "KEYRING_BEGIN"
if [[ -f /etc/apt/keyrings/docker.asc ]]; then
  ls -l /etc/apt/keyrings/docker.asc
else
  echo "missing"
fi
echo "KEYRING_END"

echo "DOCKER_VERSION_BEGIN"
if command -v docker >/dev/null 2>&1; then
  docker --version || true
  docker compose version || true
else
  echo "docker-missing"
fi
echo "DOCKER_VERSION_END"
