#!/bin/sh

if [ -e "/etc/iptables.firewall.rules" ]; then
  echo "/etc/iptables.firewall.rules already exists; skipping this step"
else
  sudo echo "*filter

#  Allow all loopback (lo0) traffic and drop all traffic to 127/8 that doesn't use lo0
-A INPUT -i lo -j ACCEPT
-A INPUT -d 127.0.0.0/8 -j REJECT

#  Accept all established inbound connections
-A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

#  Allow all outbound traffic - you can modify this to only allow certain traffic
-A OUTPUT -j ACCEPT

#  Allow HTTP and HTTPS connections from anywhere (the normal ports for websites and SSL).
-A INPUT -p tcp --dport 80 -j ACCEPT
-A INPUT -p tcp --dport 443 -j ACCEPT

#  Allow SSH connections
#  The -dport number should be the same port number you set in sshd_config
-A INPUT -p tcp -m state --state NEW --dport 22 -j ACCEPT

#  Allow ping
-A INPUT -p icmp --icmp-type echo-request -j ACCEPT

#  Log iptables denied calls
-A INPUT -m limit --limit 5/min -j LOG --log-prefix \"iptables denied: \" --log-level 7

#  Drop all other inbound - default deny unless explicitly allowed policy
-A INPUT -j DROP
-A FORWARD -j DROP

COMMIT
" > /etc/iptables.firewall.rules
  sudo iptables-restore < /etc/iptables.firewall.rules
  sudo echo "#!/bin/sh
/sbin/iptables-restore < /etc/iptables.firewall.rules
" > /etc/network/if-pre-up.d/firewall
  sudo chmod +x /etc/network/if-pre-up.d/firewall
fi

sudo mkdir -p /data/localsite/www
sudo mkdir -p /data/localsite/logs
sudo chown -R www-data:www-data /data/localsite/

sudo cp /etc/nginx/sites-available/default /etc/nginx/sites-available/localsite
sudo ln -s /etc/nginx/sites-available/localsite /etc/nginx/sites-enabled/localsite
sudo rm /etc/nginx/sites-enabled/default 

sudo perl -pi -e "s/(^\s+)root \/usr.*$/\troot \/data\/localsite\/www\;/" /etc/nginx/sites-available/localsite
sudo perl -pi -e "s/(^\s+)index index.html index.htm\;/\tindex index.php index.html index.htm\;/" /etc/nginx/sites-available/localsite
sudo perl -pi -e "s/(^\s+)server_name localhost\;/\tserver_name localsite\;\n\terror_log \/data\/localsite\/logs\/error.log error\;\n\taccess_log \/data\/localsite\/logs\/access.log\;/" /etc/nginx/sites-available/localsite
sudo perl -pi -e 'print "\tlocation ~ [^/].php(/|\$\) {
\t\tfastcgi_split_path_info ^(.+?.php)(/.*)\$\;
\t\tfastcgi_pass unix:/var/run/php5-fpm.sock;
\t\tfastcgi_index index.php;
\t\tinclude fastcgi_params;
\t}

" if (/^\s+# Only for nginx-naxsi used with.*$/)' /etc/nginx/sites-available/localsite

if [ -e "/data/localsite/www/index.php" ]; then
  echo "/data/localsite/www/index.php already exists; skipping this step"
else
  sudo echo "<html>
  <head>
    <meta http-equiv=\"refresh\" content=\"1\">
    <meta http-equiv=\"expires\" content=\"-1\">
  </head>
  <body>
  <pre>
<?
ob_start();
include '/var/tmp/xbcfg.out';
\$s = ob_get_clean();
print \$s;
?>
  </pre>
  </body>
</html>
" > /data/localsite/www/index.php
fi

sudo service php5-fpm restart
sudo service nginx stop
sudo service nginx start

ipaddr=`ifconfig | awk -F':' '/inet addr/&&!/127.0.0.1/{split($2,_," ");print _[1]}'`
echo "Configured!  Go to this address in a browser to check: http://$ipaddr"
