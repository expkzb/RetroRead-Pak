#!/usr/bin/env bash
set -euo pipefail

mkdir -p /etc/docker
cat >/etc/docker/daemon.json <<'EOF'
{
  "dns": ["8.8.8.8", "1.1.1.1"]
}
EOF

systemctl reset-failed docker
systemctl restart docker
sleep 2
docker --version
docker info >/dev/null
echo "DOCKER_DNS_OK"
