#!/bin/env dash

while true; do
    isrec=$(sysstats isrec)
    # browsers=$(browserctrl -bst 25)   # disabled in your config
    song=$(songctrl -sst Supersonic 20)
    bright=$(sysstats brightness)
    vol=$(sysstats volume)
    bat=$(sysstats battery)
    datetime=$(sysstats date_time)

    # xsetroot -name " $isrec || $browsers ||  $song || $bright || $vol || $bat || $datetime "
    xsetroot -name " $isrec || $song || $bright || $vol || $bat || $datetime "

    sleep 0.2
done
