#!/bin/sh

if [ "$1" = "install" ]; then

    runas="$2"
    if [ "$runas" = "" ]; then
	echo "Please provide a username to run thermPoller as when installing"
	exit
    fi

    echo "installing $2"

    if [ ! -e "./thermPoller" ]; then
	echo "Please make executable before running install"
	exit
    fi

    sudo mkdir -p /usr/local/bin/XBeeThermClient
    sudo cp -p ./thermPoller /usr/local/bin/XBeeThermClient
    sudo cp -p ./thermPollerMaster.sh /usr/local/bin/XBeeThermClient

    sudo mkdir -p /etc/XBeeThermClient
    if [ -e "/etc/XBeeThermClient/XBeeThermPollerConfig" ]; then
	echo "XBeeThermPollerConfig already exists; not replacing"
    else
	sudo cp -p ./XBeeThermPollerConfig /etc/XBeeThermClient
    fi

    sudo cp -p ./thermPollerInit /etc/init.d
    sudo perl -pi -e "s/username/$runas/" /etc/init.d/thermPollerInit
    sudo update-rc.d thermPollerInit defaults

elif [ "$1" = "uninstall" ]; then
    
    echo "uninstalling"
    sudo service thermPollerInit stop
    sudo rm -f /usr/local/bin/XBeeThermClient/thermPoller
    sudo rm -f /usr/local/bin/XBeeThermClient/thermPollerMaster.sh
    if [ `find /usr/local/bin/XBeeThermClient -prune -empty -type d` ]; then
	rmdir --ignore-fail-on-non-empty /usr/local/bin/XBeeThermClient
    fi
    if [ -e "/etc/XBeeThermClient/XBeeThermPollerConfig" ]; then
	cmp ./XBeeThermPollerConfig /etc/XBeeThermClient/XBeeThermPollerConfig > /dev/null 2>&1
	if [  $? != 0 ]; then
	    echo "/etc/XBeeThermClient/XBeeThermPollerConfig has been altered; not removing"
	    echo "Please remove manually and repeat uninstall for a complete uninstall"
	else
	    sudo rm -f /etc/XBeeThermClient/XBeeThermPollerConfig
	fi
    fi
    if [ `find /etc/XBeeThermClient -prune -empty -type d` ]; then
	rmdir --ignore-fail-on-non-empty /etc/XBeeThermClient
    fi

    sudo rm -f /etc/init.d/thermPollerInit
    sudo update-rc.d thermPollerInit remove

else

    echo "usage: $0 [install|uninstall]"

fi
