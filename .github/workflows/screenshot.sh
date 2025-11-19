#!/bin/bash

anApp=$1
anImg=$2
aDelay=$3

"$anApp" &
anAppPid=$!

rm -f "$anImg"
scrot -d $aDelay -F "$anImg"
kill $anAppPid
