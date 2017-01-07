#  * Project: mediatex's sso
#  * Module: client
#  *
#  * Apache module
# 
#  MediaTex is an Electronic Records Management System
#  Copyright (C) 2017 Nicolas Roche
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Apache2::mdtxSsoClt;

use URI::Escape;
use MIME::Base64;
use Crypt::OpenSSL::AES;
use Crypt::OpenSSL::RSA;

use Apache2::ServerRec; # log_error
use Apache2::Cookie; # dir_config...
use Apache2::URI;    # construct_url
use Apache2::Const -compile => 
    qw(DECLINED REDIRECT OK SERVER_ERROR :log);

sub handler {
    use strict;
    use warnings;
    
    sub redirect {
    	my ($r, $ssoServer, $url, $msg) = @_;

	if (defined($r->args)) {
	    $url.="?".$r->args;
	}
	
	$r->server->log_error("sso: ".$msg." from ".$url);
    	$r->content_type('text/html');
    	$r->err_headers_out->add('Location' => 
				 $ssoServer."?url=".uri_escape($url));
    }

    sub decrypt {
	my ($aesKey, $base64) = @_;
	my $crypted = decode_base64($base64);
	my $string = '';
	my $cipher;
	my $chunk;
	my $i = 0;
	    
	while (length($aesKey) % 16) {
	    $aesKey.=' ';
	}
	
	$cipher = new Crypt::OpenSSL::AES($aesKey);
	
	while ($i < length($crypted)) {
	    $chunk = substr($crypted, $i, 16);
	    $string .= $cipher->decrypt($chunk);
	    $i+=16;
	}

	return $string;
    }
    
    sub verifyPublic {
	my ($r, $publicKey, $msg, $sign64) = @_;
	my $keyString;
	my $public;
	my $fh;
	my $toto;
	
	if (!open($fh, "<", $publicKey)) {
	    $r->server->log_error("Cannot read public key on ".
				  $publicKey.": ".$!);
	    return undef;
	}
	
	read($fh, $keyString,-s $fh); # Suck in the whole file
	close($fh);
	
	$public = Crypt::OpenSSL::RSA->new_public_key($keyString);
	$public->verify($msg, decode_base64($sign64));
    }

    my $r = shift;
    my $confSsoServerUrl;
    my $confPubKeyFile;
    my $confAesKey;
    my $confKeyPass;
    my $confGroup;
    my %cookies;
    my $urlEncoded;
    my $cookie; # "mediatex=coll1:index,cache;coll2:index
    my $crypted;
    my $signed;
    my $perm;   # "coll1:index,cache"
    my $coll;   # "coll1"
    my $string; # "index,cache"
    my $group;  # "index"
    my @parts;
    my @perms;
    my @groups;
    my $url = $r->construct_url;
    my $urlColl;
    my $userIp;
    my $remote_ip;

    $r->server->log_error("->sso");
    
    # get configuration values
    $confSsoServerUrl=$r->dir_config('SSO_SERVER_URL');
    $confPubKeyFile=$r->dir_config('PUB_KEY_FILE');
    $confAesKey=$r->dir_config('AES_KEY');
    $confGroup=$r->dir_config('GROUP');

    if (!defined($confSsoServerUrl)) {
    	$r->server->log_error("please set SSO_SERVER_URL variable");
    	return Apache2::Const::SERVER_ERROR;
    }
    
    if (!defined($confPubKeyFile)) {
    	$r->server->log_error("please set PUB_KEY_FILE variable");
    	return Apache2::Const::SERVER_ERROR;
    }
    
    if (!defined($confAesKey)) {
    	$r->server->log_error("please set AES_KEY variable");
    	return Apache2::Const::SERVER_ERROR;
    }

    if (!defined($confGroup)) {
    	$r->server->log_error("please set GROUP variable");
    	return Apache2::Const::SERVER_ERROR;
    }
    
    if (!($url =~ /\/~mdtx-([^\/]+)/)) {
	$r->server->log_error("cannot get collection from url: ".$url);
    	return Apache2::Const::SERVER_ERROR;
    }
    $urlColl=$1;
    
    # fetch existing cookies
    %cookies = Apache2::Cookie->fetch($r);
    if (!defined($cookies{"mediatex"})) {
    	redirect($r, $confSsoServerUrl, $url, "no cookie for mediatex");
    	return Apache2::Const::REDIRECT;
    }

    # remove left part
    $urlEncoded = $cookies{"mediatex"};        
    $cookie = uri_unescape($urlEncoded);
    $string = substr($cookie, 9, length($cookie)-9);
    if (substr($cookie, 0, 9) ne 'mediatex=') {
  	redirect($r, $confSsoServerUrl, $url, "no content for mediatex");
    	return Apache2::Const::REDIRECT;
    }

    # check signature
    @parts = split(/:/, $string);    
    $crypted = $parts[0];
    $signed = $parts[1];
    if (!verifyPublic($r, $confPubKeyFile, $crypted, $signed)) {
	redirect($r, $confSsoServerUrl, $url, "invalid cookie signature");
    	return Apache2::Const::REDIRECT;
    }

    # decrypt permissions
    $string = decrypt($confAesKey, $crypted);
    
    # split permission string
    @parts = split(/=/, $string);
    $userIp = $parts[0];
    $string = $parts[1];
    
    # # match IP
    $remote_ip =$r->useragent_ip();
    if ($userIp ne $remote_ip) {
    	redirect($r, $confSsoServerUrl, $url, "bad ip for ".$remote_ip.
		 ". Expected ".$userIp);
    	return Apache2::Const::REDIRECT;
    }
    
    # match permissions
    @perms = split(/;/, $string);
    foreach $perm (@perms) {
    	@parts = split(/:/, $perm);
    	$coll = $parts[0];
    	$string = $parts[1];
    	if ($urlColl eq $coll) {
    	    @groups = split(/,/, $string);
    	    foreach $group (@groups) {
    		if ($confGroup eq $group) {
    		    return Apache2::Const::DECLINED;
    		}
    	    }
    	}
    }
    
    redirect($r, $confSsoServerUrl, $url,
	     "bad credentials for ".$urlColl.":".$confGroup);
    return Apache2::Const::REDIRECT;
    
    $r->server->log_error("-> DECLINED");
    return Apache2::Const::DECLINED;
}

1;
