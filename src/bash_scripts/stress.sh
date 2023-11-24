#!/bin/bash

stress_test() {
    local SERVER="localhost"
    local PORT="1110"
    local USERNAME="testuser"
    local PASSWORD="testuserpassword"
    local TOTAL_REQUESTS=$1

    echo "Performing stress test with $TOTAL_REQUESTS curls"

    total_time=0
    for ((i = 1; i <= TOTAL_REQUESTS; i++)); do
        (
            start_time=$(date +%s%N)
            curl -u "$USERNAME:$PASSWORD" "pop3://$SERVER:$PORT/1" >/dev/null 2>&1
            end_time=$(date +%s%N)
            elapsed_time=$(((end_time - start_time) / 1000000)) # Convert nanoseconds to milliseconds
            echo "Curl request $i took $elapsed_time milliseconds"
        ) &
    done

    # Wait for all background processes to finish
    wait

}

# Call the function with increasing numbers of curls
stress_test 10
sleep 1
stress_test 50
sleep 1
stress_test 100
sleep 1
stress_test 200
