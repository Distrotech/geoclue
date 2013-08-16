#!/bin/bash

# taken from http://git.gnome.org/browse/totem/tree/browser-plugin/tests/launch-web-server.sh with some necessary modifications

# Port to listen on
PORT=12345


usage()
{
    echo "Usage: ./`basename $0` <--remote> [stop | start]"
echo " --remote: allow for connections from remote machines"
exit 1
}


# Find Apache first

if [ -x /usr/sbin/httpd ] ; then APACHE_HTTPD=/usr/sbin/httpd; fi

HTTPD=${APACHE_HTTPD:-/usr/sbin/apache2}

if [ -z $HTTPD ] ; then
    echo "Could not find httpd at the usual locations"
    exit 1
fi

# Check whether we in the right directory

if [ ! -f `basename $0` ] ; then
    echo "You need to launch `basename $0` from within its directory"
    echo "eg: ./`basename $0` <--remote> [stop | start]"
echo " --remote: allow for connections from remote machines"
exit 1
fi

ROOTDIR=`dirname $0`/root

# See if we shoud stop the web server

if [ -z $1 ] ; then
    usage $0
fi

if [ x$1 = x"--remote" ] ; then
    ADDRESS=
    shift
else
    ADDRESS=127.0.0.1
fi

if [ x$1 = xstop ] ; then
    echo "Trying to stop $HTTPD(`cat root/pid`)"
    pushd root/ > /dev/null
    $HTTPD -f `pwd`/conf -k stop
    popd > /dev/null
    exit
elif [ x$1 != xstart ] ; then
    usage $0
fi

# Setup the ServerRoot

if [ ! -d $ROOTDIR ] ; then
    mkdir -p root/ || ( echo "Could not create the ServerRoot" ; exit 1 )
fi

DOCDIR=`pwd`
CGI_BIN_DIR=`pwd`/cgi-bin/
server_exec=geoip-lookup

#set up the cgi-bin directory
if [ ! -d $CGI_BIN_DIR ] ; then
    mkdir -p cgi-bin/ || ( echo "Could not create the cgi-bin directory" ; exit 1 )
fi

if [ ! -f cgi-bin/$server_exec ] ; then
    ln -s `pwd`/../$server_exec cgi-bin/$server_exec || ( echo "Could not create a symbolic link to the server executable" ; exit 1 )
fi

pushd root/ > /dev/null
# Resolve the relative ROOTDIR path
ROOTDIR=`pwd`
if [ -f pid ] && [ -f conf ] ; then
    $HTTPD -f $ROOTDIR/conf -k stop
    sleep 2
fi
rm -f conf pid lock log access_log

# Setup the config file

cat > conf <<EOF
LoadModule alias_module modules/mod_alias.so
LoadModule log_config_module modules/mod_log_config.so
LoadModule unixd_module modules/mod_unixd.so
LoadModule access_compat_module modules/mod_access_compat.so
# Authentication modules
LoadModule auth_basic_module modules/mod_auth_basic.so
LoadModule auth_digest_module modules/mod_auth_digest.so
LoadModule authn_anon_module modules/mod_authn_anon.so
LoadModule authn_core_module modules/mod_authn_core.so
LoadModule authn_dbd_module modules/mod_authn_dbd.so
LoadModule authn_dbm_module modules/mod_authn_dbm.so
LoadModule authn_file_module modules/mod_authn_file.so
LoadModule authn_socache_module modules/mod_authn_socache.so
LoadModule authz_core_module modules/mod_authz_core.so
LoadModule authz_dbd_module modules/mod_authz_dbd.so
LoadModule authz_dbm_module modules/mod_authz_dbm.so
LoadModule authz_groupfile_module modules/mod_authz_groupfile.so
LoadModule authz_host_module modules/mod_authz_host.so
LoadModule authz_owner_module modules/mod_authz_owner.so
LoadModule authz_user_module modules/mod_authz_user.so

LoadModule mpm_prefork_module modules/mod_mpm_prefork.so
LoadModule autoindex_module modules/mod_autoindex.so
LoadModule cgid_module modules/mod_cgid.so
LoadModule deflate_module modules/mod_deflate.so
LoadModule dir_module modules/mod_dir.so
LoadModule env_module modules/mod_env.so
LoadModule mime_module modules/mod_mime.so
LoadModule negotiation_module modules/mod_negotiation.so
LoadModule reqtimeout_module modules/mod_reqtimeout.so
LoadModule setenvif_module modules/mod_setenvif.so
LoadModule status_module modules/mod_status.so

SetEnv HTTP_CLIENT_IP 24.24.24.24

ServerName localhost
ServerRoot "$ROOTDIR"
DefaultRuntimeDir "$ROOTDIR"
PidFile pid
#LockFile lock
# LogLevel crit
LogLevel info
ErrorLog log
LogFormat "%h %l %u %t \"%r\" %>s %b \"%{Referer}i\" \"%{User-Agent}i\"" combined
CustomLog access_log combined
TypesConfig /etc/mime.types
# Socket for cgid communication
ScriptSock cgisock

<VirtualHost *:$PORT>
  DocumentRoot "$DOCDIR"

  <Directory />
    Options FollowSymLinks
    AllowOverride None
  </Directory>

  <Directory "$DOCDIR">
   Options Indexes FollowSymLinks MultiViews
   AllowOverride None
   Order allow,deny
   allow from all
  </Directory>

  ScriptAlias /cgi-bin/ "$CGI_BIN_DIR"
  <Directory "$CGI_BIN_DIR">
   AllowOverride None
   Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch
   Order allow,deny
   Allow from all
  </Directory>

</VirtualHost>

StartServers 1
EOF

popd > /dev/null

# Launch!


#$HTTPD -f $ROOTDIR/conf -C "Listen 127.0.0.1:$PORT"
if [ -z $ADDRESS ] ; then
$HTTPD -f $ROOTDIR/conf -C "Listen $PORT"
else
$HTTPD -f $ROOTDIR/conf -C "Listen ${ADDRESS}:$PORT"
fi
echo "Please start debugging at http://localhost:$PORT/cgi-bin/$server_exec"
