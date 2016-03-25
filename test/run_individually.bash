#!/usr/bin/env bash
if [ -z "$TRAIN_SCH_ROOTDIR" ]; then
	echo "environment not set up"
	exit 1;
fi;

EXE="$TRAIN_SCH_EXEDIR/train-sch"

START_DATASET=$1
STOP_DATASET=$2

if [ -z "$START_DATASET" ]; then
	echo "please select a data set"
	exit 2;
fi

for i in $(seq $START_DATASET $STOP_DATASET); do
	DIR="dataset-$i";
	(mkdir -p "$DIR" && cd "$DIR" && "$EXE" --data-num "$i" > stdout.txt) &
done

wait
