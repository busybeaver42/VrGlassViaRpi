#!/bin/bash
date +"%H:%M:%S"
ifconfig | grep inet | grep 192.168.
sshpass -p "cyberdyn" ssh pi@192.168.1.100 bash -c "echo mypass | sudo -S pkill mjpg_streamer"&
date +"%H:%M:%S"
sshpass -p "cyberdyn" ssh pi@192.168.1.101 bash -c "echo mypass | sudo -S pkill mjpg_streamer"&
# sleep for 5 seconds
sleep 2

# end time
date +"%H:%M:%S"
sshpass -p "cyberdyn" ssh pi@192.168.1.102 bash -c "echo mypass | sudo -S pkill ffmpeg"&
date +"%H:%M:%S"
sshpass -p "cyberdyn" ssh pi@192.168.1.103 bash -c "echo mypass | sudo -S pkill ffmpeg"&
# end time
date +"%H:%M:%S"
