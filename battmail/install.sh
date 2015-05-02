#!/bin/sh

if [ "$1" = "install" ]; then

    echo "installing $2"

    if [ ! -e "./battmail" ]; then
	echo "Please make executable before running install"
	exit
    fi

    sudo mkdir -p /usr/local/bin/XBeeThermClient
    sudo cp -p ./battmail /usr/local/bin/XBeeThermClient

    sudo mkdir -p /etc/XBeeThermClient
    if [ -e "/etc/XBeeThermClient/BattMailConfig" ]; then
	echo "BattMailConfig already exists; not replacing"
    else
	sudo cp -p ./BattMailConfig /etc/XBeeThermClient
    fi

    crontab -l | grep -v "battmail" | crontab -
    cronline="0 */2  * * * /usr/local/bin/XBeeThermClient/battmail"
    (crontab -l; echo "$cronline") | crontab -

elif [ "$1" = "uninstall" ]; then
    
    echo "uninstalling"
    sudo rm -f /usr/local/bin/XBeeThermClient/battmail
    if [ `find /usr/local/bin/XBeeThermClient -prune -empty -type d` ]; then
	rmdir --ignore-fail-on-non-empty /usr/local/bin/XBeeThermClient
    fi
    if [ -e "/etc/XBeeThermClient/BattMailConfig" ]; then
	cmp ./BattMailConfig /etc/XBeeThermClient/BattMailConfig > /dev/null 2>&1
	if [  $? != 0 ]; then
	    echo "/etc/XBeeThermClient/BattMailConfig has been altered; not removing"
	    echo "Please remove manually and repeat uninstall for a complete uninstall"
	else
	    sudo rm -f /etc/XBeeThermClient/BattMailConfig
	fi
    fi
    if [ `find /etc/XBeeThermClient -prune -empty -type d` ]; then
	rmdir --ignore-fail-on-non-empty /etc/XBeeThermClient
    fi
    crontab -l | grep -v "battmail" | crontab -

else

    echo "usage: $0 [install|uninstall]"

fi
