#!/bin/sh

if [ "$1" = "install" ]; then

    webname="$2"
    if [ "$webname" = "" ]; then
	echo "Please provide a name specifier when installing"
	exit
    fi

    echo "installing $2"

    if [ ! -e "./thermHtmlGen" ]; then
	echo "Please make executable before running install"
	exit
    fi

    sudo mkdir -p /usr/local/bin/XBeeThermClient
    sudo cp -p ./thermHtmlGen /usr/local/bin/XBeeThermClient
    sudo cp -p ./thermHtmlGenWork.sh /usr/local/bin/XBeeThermClient
    sudo perl -pi -e "s/current/$webname/" /usr/local/bin/XBeeThermClient/thermHtmlGenWork.sh

    sudo mkdir -p /etc/XBeeThermClient
    sudo cp -p ./template_dg.html /etc/XBeeThermClient
    sudo cp -p ./template_gchart.html /etc/XBeeThermClient

    crontab -l | grep -v "thermHtmlGenWork.sh" | crontab -
    cronline="*/15 * * * * /usr/local/bin/XBeeThermClient/thermHtmlGenWork.sh"
    (crontab -l; echo "$cronline") | crontab -

elif [ "$1" = "uninstall" ]; then
    
    echo "uninstalling"
    sudo rm -f /usr/local/bin/XBeeThermClient/thermHtmlGen
    sudo rm -f /usr/local/bin/XBeeThermClient/thermHtmlGenWork.sh
    if [ `find /usr/local/bin/XBeeThermClient -prune -empty -type d` ]; then
	sudo rmdir /usr/local/bin/XBeeThermClient
    fi
    sudo rm -r /etc/XBeeThermClient/template_dg.html
    sudo rm -r /etc/XBeeThermClient/template_gchart.html
    if [ `find /etc/XBeeThermClient -prune -empty -type d` ]; then
	sudo rmdir /etc/XBeeThermClient
    fi
    crontab -l | grep -v "thermHtmlGenWork.sh" | crontab -

else

    echo "usage: $0 [install|uninstall]"

fi
