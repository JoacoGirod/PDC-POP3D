#!/bin/bash

# Number of users to connect
num_users=500

# Server details
server_host="localhost"
server_port=1110

# Loop to connect users
for ((i = 1; i <= num_users; i++)); do
  # Attempt connection using netcat
  nc -C $server_host $server_port &
  echo "User $i connected."
  sleep 0.02
done

# Wait for all background processes to finish
wait

echo "All users connected."
