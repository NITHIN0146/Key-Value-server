#!/bin/bash

SCRIPT=$1
URL="http://localhost:8080"

if [ -z "$SCRIPT" ]; then
  echo "Usage: ./run.sh get.js"
  exit 1
fi

echo "vus,tps,avg,p50,p90,p95,p99,cpu_usage_max,disk_usage_max" > results.csv

for VUS in 50 100 150 200 250 500 1000 1500 2000
do
    echo "Running test with $VUS VUs..."

    vmstat 1 > cpu.log & 
    CPU_PID=$!

    iostat -dx 1 > disk.log &
    DISK_PID=$!

    OUT=$(k6 run --summary-export=summary.json --env VUS=$VUS $SCRIPT 2>&1)

    kill $CPU_PID
    kill $DISK_PID

    CPU_IDLE_MIN=$(awk 'NR>2 {if(min=="" || $15 < min) min=$15} END {print min+0}' cpu.log)
    CPU_UTIL_MAX=$(echo "100 - $CPU_IDLE_MIN" | bc)
    DISK_UTIL_MAX=$(awk '/^[a-z]/ {if(max=="" || $(NF) > max) max=$(NF)} END {print max+0}' disk.log)

    TPS=$(jq '.metrics.http_reqs.rate' summary.json)
    AVG=$(jq '.metrics.http_req_duration.avg' summary.json)
    P50=$(jq '.metrics.http_req_duration["p(50)"]' summary.json)
    P90=$(jq '.metrics.http_req_duration["p(90)"]' summary.json)
    P95=$(jq '.metrics.http_req_duration["p(95)"]' summary.json)
    P99=$(jq '.metrics.http_req_duration["p(99)"]' summary.json)

    echo "$VUS,$TPS,$AVG,$P50,$P90,$P95,$P99,$CPU_UTIL_MAX,$DISK_UTIL_MAX" >> results.csv
done