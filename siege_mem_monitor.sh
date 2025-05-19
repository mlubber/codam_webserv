#!/bin/bash

# ---- CONFIGURATION ----
SERVER_COMMAND_NAME="webserv"         # Server binary name
SIEGE_URL="http://127.0.0.1:8081/"    # Target URL
MONITOR_INTERVAL=1                    # Seconds between memory samples
MEMORY_LOG="memory_usage.log"         # File to log memory usage

# ---- GET SERVER PID ----
PID=$(pgrep -f "$SERVER_COMMAND_NAME" | head -n 1)
if [ -z "$PID" ]; then
    echo "Error: Could not find a process matching '$SERVER_COMMAND_NAME'."
    exit 1
fi

echo "Monitoring PID $PID for '$SERVER_COMMAND_NAME'..."
echo "Logging memory usage to '$MEMORY_LOG'..."
echo "Starting siege on '$SIEGE_URL'..."

# ---- START MEMORY MONITORING IN BACKGROUND ----
(
    echo "Time(s) RSS(KB)"
    SECONDS=0
    while true; do
        if ps -p $PID > /dev/null; then
            RSS=$(ps -o rss= -p $PID)
            echo "$SECONDS $RSS"
        else
            echo "$SECONDS [PROCESS ENDED]"
            break
        fi
        sleep $MONITOR_INTERVAL
    done
) | tee "$MEMORY_LOG" &

MEM_MONITOR_PID=$!

# ---- RUN SIEGE ----
siege -b "$SIEGE_URL"

# ---- CLEANUP ----
echo "Siege finished. Stopping memory monitor..."
kill $MEM_MONITOR_PID 2>/dev/null
wait $MEM_MONITOR_PID 2>/dev/null

echo "Done. Memory usage logged to '$MEMORY_LOG'."

# Manual:
# get PID: ps aux | grep webserv
# monitor: top -p <PID>
# run: siege "http://127.0.0.1:8081"
