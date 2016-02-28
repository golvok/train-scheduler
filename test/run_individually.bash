#!/usr/bin/env bash

EXE=$1/train-sch

START_DATASET=1
STOP_DATASET=$2

for i in $(seq $START_DATASET $STOP_DATASET); do
	DIR="dataset-$i";
	(mkdir -p "$DIR" && cd "$DIR" && "../$EXE" --data-num "$i" > stdout.txt) &
done

wait
