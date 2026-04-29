#!/bin/bash
cd /home/meowbuster/Danger-Dash
make clean 2>&1 > /tmp/build.txt
make 2>&1 >> /tmp/build.txt
if [ -f game.out ]; then
    echo "BUILD_SUCCESS" >> /tmp/build.txt
else
    echo "BUILD_FAILED" >> /tmp/build.txt
fi
cat /tmp/build.txt
