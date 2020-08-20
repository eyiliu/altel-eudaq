#!/usr/bin/env bash

SOURCE_DIR=/home/teleuser/tmp/altel_eudaq
BIN_DIR=/home/teleuser/tmp/INSTALL/bin

# cp $SOURCE_DIR/source/eudaq/misc/* /tmp
# cp $SOURCE_DIR/source/lib/misc/*   /tmp


killall -q xterm

killall -q euCliProducer; killall -q euRun
killall -q euCliProducer; killall -q euCliCollector; killall -q StdEventMonitor; killall -q euLog
sleep 1

xterm -T "RUN" -e "$BIN_DIR/euRun" &
sleep 1

sleep 1
xterm -T "Monitor" -e "$BIN_DIR/StdEventMonitor -t StdEventMonitor -r tcp://192.168.21.1:44000" &
xterm -T "Collector" -e "$BIN_DIR/euCliCollector -n TriggerIDSyncDataCollector -t one -r tcp://192.168.21.1:44000" &
sleep 1

xterm -T "TLU" -e "$BIN_DIR/euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://192.168.21.1:44000" &
xterm -T "altel" -e "$BIN_DIR/euCliProducer -n AltelProducer -t altel -r tcp://192.168.21.1:44000" & 
