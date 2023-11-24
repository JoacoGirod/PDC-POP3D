#!/bin/bash

# Number of users to connect
num_users=200

# Server details
server_host="localhost"
server_port=1110

# Function to connect a user in a separate thread
connect_user() {
    nc -C $server_host $server_port
}

# Record the start time
start_time=$(date +%s.%N)

# Loop to connect users
for ((i = 1; i <= num_users; i++)); do
    # Call the connect_user function in the background
    connect_user &
done

# Wait for all background processes to finish
wait

# Record the end time
end_time=$(date +%s.%N)

# Calculate the duration
duration=$(echo "$end_time - $start_time" | bc)

# Display the throughput
echo "Throughput: $num_users connections in $duration seconds."
