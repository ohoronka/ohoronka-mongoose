#!/bin/bash
# mos build --local --platform esp8266 --verbose
# mos flash
 mos config-set mqtt.server=192.168.0.103:1883 mqtt.client_id=1 mqtt.user=1 mqtt.pass=kYuuYly6sETizxl3+Z+UqI3N1bqQyRR6mMY3+kwT device.id=1 device.gpio_listen=12336 wifi.ap.ssid=OHORONKA1 wifi.ap.pass=12345678
 mos wifi TP-LINK_3B3DC8 0505933918
#
# mos build --local --platform esp8266 --verbose && mos flash && mos config-set mqtt.server=192.168.0.103:1883 mqtt.client_id=1 mqtt.user=1 mqtt.pass=kYuuYly6sETizxl3+Z+UqI3N1bqQyRR6mMY3+kwT device.id=1 device.gpio_listen=12336 wifi.ap.ssid=OHORONKA1 wifi.ap.pass=12345678 # && mos wifi TP-LINK_3B3DC8 0505933918 && mos console
