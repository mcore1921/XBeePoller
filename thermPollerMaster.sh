#!/bin/sh

outlog=$1
if [ "$outlog" = "" ]; then
    outlog="/dev/null"
fi

while [ 1 ]; do
    /usr/local/bin/XBeeThermClient/thermPoller >> $outlog
    sleep 1
done

