#!/bin/sh

for dir in perl python; do
    (cd $dir && ./generate.sh)
done
