#!/bin/sh

echo "5" > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio5/direction
echo "$1" > /sys/class/gpio/gpio5/value

echo "6" > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio6/direction
echo "$1" > /sys/class/gpio/gpio6/value
