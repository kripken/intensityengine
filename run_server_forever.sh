#!/bin/bash
echo ""
echo "Running server instance in a repeated loop"
echo "Home directory:" $1
echo "Log file:" $1out.log
echo ""

while true; do
    # Save old log, and tell instance to read from there, so it can know if it crashed last time
    mv $1/out.log $1/out.log.old
    echo ""
    echo "Starting server instance"
    echo "To stop server, press Ctrl-C"
    echo ""
    ./intensity_server.sh $1 $1out.log.old &> $1/out.log
    echo ""
    echo ""
    echo "Server instance stopped"
    echo "Pausing 5 seconds"
    echo "press Ctrl-C if you do not want server instance to be automatically restarted"
	sleep 5
done

