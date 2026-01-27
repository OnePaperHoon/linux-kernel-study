#!/bin/bash

echo "=== TEST 1: 전체 트리 ==="
./pstree | head -20

echo ""
echo "=== TEST 2: 틍정 PID ==="
./pstree --pid $$

echo ""
echo "=== TEST 3: 현재 쉘 하위 ==="
./pstree -pid $$

echo ""
echo "=== TEST 4: 메모리 누수 체크 ==="
valgrind --leak-check=full ./prstree > /dev/null

echo ""
echo "=== TEST 5: 성능 ==="
time ./pstree > /dev/null