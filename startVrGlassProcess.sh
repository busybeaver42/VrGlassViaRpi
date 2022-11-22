#!/bin/bash
date +"%H:%M:%S"
ifconfig | grep inet | grep 192.168.
sshpass -p "cyberdyn" ssh pi@192.168.1.100 bash -c "echo mypass | sudo -S ./startCamServer.sh"&
date +"%H:%M:%S"
sshpass -p "cyberdyn" ssh pi@192.168.1.101 bash -c "echo mypass | sudo -S ./startCamServer.sh"&
# sleep for 5 seconds
sleep 5

# end time
date +"%H:%M:%S"
sshpass -p "cyberdyn" ssh pi@192.168.1.102 bash -c "echo mypass | sudo -S ./startGrabber.sh"&
date +"%H:%M:%S"
sshpass -p "cyberdyn" ssh pi@192.168.1.103 bash -c "echo mypass | sudo -S ./startGrabber.sh"&
# end time
date +"%H:%M:%S"
