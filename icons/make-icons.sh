#!/bin/sh

#RESOLUTIONS="16 22 24 32 36 48 64 72 96 128 192 256 512"
RESOLUTIONS="48 64 128 192 256"
INPUT="terminal-2.svg"

for n in $RESOLUTIONS
do
  echo icon_$n.png
  rsvg-convert -w $n -h $n -f png -o icon_$n.png $INPUT
  optipng -quiet icon_$n.png
done
