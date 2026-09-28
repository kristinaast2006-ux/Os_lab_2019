#!/bin/bash

count=$#
sum=0

for arg in "$@"; do
    sum=$((sum + arg))
done

if [ "$count" -eq 0 ]; then
    echo "Нет аргументов"
    exit 1
fi

average=$(echo "scale=2; $sum / $count" | bc)
echo "Количество: $count"
echo "Среднее: $average"
