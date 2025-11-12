#!/bin/bash

# Test script for gym management system

echo "Building hw1..."
make clean
make

if [ ! -f "./hw1" ]; then
    echo "Error: hw1 binary not found!"
    exit 1
fi

echo "Starting server..."
./hw1 @/tmp/gym.sock 1000 5000 3 &
SERVER_PID=$!
sleep 1

echo "Server started with PID: $SERVER_PID"

# Test with Python client
echo "Testing with Python client..."
timeout 5 python3 -c "
import socket
import time

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('/tmp/gym.sock')

# Request a tool
sock.send(b'REQUEST 2000\\n')
time.sleep(0.5)

# Get report
sock.send(b'REPORT\\n')
response = sock.recv(4096)
print('REPORT Response:')
print(response.decode())

# Rest
sock.send(b'REST\\n')
time.sleep(0.5)

# Quit
sock.send(b'QUIT\\n')
sock.close()
"

echo "Killing server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "Test complete!"