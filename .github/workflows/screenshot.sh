#!/bin/bash

anApp=$1
anImg=$2
aDelay=$3

"$anApp" &
anAppPid=$!

rm -f "$anImg"
scrot -d $aDelay -F "$anImg"

anImgSize=$(wc -c <"$anImg")
if [ $actualsize -lt 2000 ]; then
  # retry one more time
  mv -f "$anImg" "back-first-${anImg}"
  scrot -d 1 -F "$anImg"
fi

kill $anAppPid
