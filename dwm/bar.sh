#!/bin/env dash

while true; do
    isrec=$(sysstats isrec)
    # browserctrl -bst 25   # disabled in your config
    song=$(songctrl -sst Supersonic 20)
    bright=$(brightnessctrl --get)
    vol=$(volumectrl --printvol)
    bat=$(battery)
    datetime=$(sysstats date_time)

    xsetroot -name " $isrec || $song || $bright || $vol || $bat || $datetime "

    sleep 0.2
done
