#!/bin/bash
# test_concurrent.sh

echo "启动 10 个客户端并发下载..."

for i in {1..10}; do
    ./client large.bin &
done

wait
echo "所有客户端完成"
