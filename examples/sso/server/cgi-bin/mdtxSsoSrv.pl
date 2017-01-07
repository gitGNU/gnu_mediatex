#!/usr/bin/perl
#  * Project: mediatex's sso
#  * Module: client
#  *
#  * Cgi script for server
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
    
use CGI qw(:standard);
use CGI::Carp qw(warningsToBrowser fatalsToBrowser);
use utf8;
use CGI::Cookie;
use DBI;
use URI::Escape;

use Authen::Passphrase::PHPass;
use MIME::Base64;
use Convert::PEM;
use Crypt::OpenSSL::AES;
use Crypt::OpenSSL::RSA;

use strict;
use warnings;

# domain
my $domain = "narval.fr.eu.org";

# database
my $dbHost="localhost";
my $dbPort="5432";
my $dbName = "phpbb3";
my $dbUser = "phpbb3";
my $dbPasswd = "xxxxxxxx";

# keys
my $aesKey = '1234567890123456';
my $privateKey = '/etc/apache2/mdtxSso.pem';
my $password = '1234';

############################################

# local variables
my $bad_login = 0;
my $good_login = 0;
my $permissions = "";

# get user values from the formulary
my $q = new CGI;
my $login = $q->param('login');
my $passwd = $q->param('passwd');
my $url = $q->param('url');

check_passwd($q);
output_top($q);
output_form($q);
output_end($q);

exit(0);

############################################

sub fails {
    my ($q, $msg) = @_;
    
    print $q->header();
    print $msg;
    print $q->end_html;
    exit(1);
}
    
# Check the results of the form
sub check_passwd {
    my ($q) = @_;
    my $query1 = "";
    my $query2 = "";
    my $dbh;
    my $cursor;
    my @row;
    my $ppr;

    $permissions="";

    if (!defined($login) || !defined($passwd)) {
	return;
    }

    $query1 = "select user_password from phpbb_users where username='".
	$login."'";
    $query2 = "select group_desc".
	" from phpbb_users, phpbb_user_group, phpbb_groups".
	" where username = '".$login."'".
	" and phpbb_users.user_id = phpbb_user_group.user_id".
	" and phpbb_groups.group_id = phpbb_user_group.group_id".
	" and group_name like 'mdtx-%'";
    
    # connect DB
    $dbh = DBI->connect(
	"dbi:Pg:dbname=$dbName;host=$dbHost;port=$dbPort", 
	$dbUser, $dbPasswd, {AutoCommit=>1,RaiseError=>1,PrintError=>0}) 
	or die "Database error: ".$DBI::errstr;

    # read hashed password from DB
    $cursor = $dbh->prepare($query1);
    $cursor->execute;
    @row = $cursor->fetchrow;
    if (!defined($row[0])) {
	$bad_login = 1;
	$cursor->finish;
	$dbh->disconnect;
	return;
    }

    # check passwd
    substr($row[0], 1, 1) = "P";
    $ppr = Authen::Passphrase::PHPass->from_crypt($row[0]);
    if(! $ppr->match($passwd)) {
	$bad_login = 1;
	$cursor->finish;
	$dbh->disconnect;
	return;
    }	
    $cursor->finish;
    $good_login = 1;
    
    # get permissions from DB
    $cursor = $dbh->prepare($query2);
    $cursor->execute;
    while (@row = $cursor->fetchrow) {
	$permissions=$permissions.$row[0].';';
    }
    $cursor->finish;
    $dbh->disconnect;
}

sub encrypt {
    my ($string) = @_;
    my $crypted = '';
    my $cipher;
    my $chunk;
    my $i = 0;
    
    while (length($aesKey) % 16) {
	$aesKey.=' ';
    }

    $cipher = new Crypt::OpenSSL::AES($aesKey);

    while ($i < length($string)) {
	$chunk = substr($string, $i, 16);

	while (length($chunk) % 16) {
	    $chunk.=' ';
	}
	
	$crypted .= $cipher->encrypt($chunk);
	$i+=16;
    }

    encode_base64($crypted);
}

sub signPrivate {
    my ($string) = @_;
    my $keyString;
    my $private;
    my $base64;
    
    $keyString = decryptPEM($privateKey, $password);
    return "" unless ($keyString);
    
    $private = Crypt::OpenSSL::RSA->new_private_key($keyString) ||
	die "$!";
    
    encode_base64($private->sign($string));
}    

# Outputs the start html tag, stylesheet and heading
sub output_top {
    my ($q) = @_;
    my %cookies;
    my $cookie;
    my $ip;
    my $msg;
    my $sign;
    my $value;
    
    $ip = $ENV{REMOTE_ADDR};
    $msg = encrypt($ip."=".$permissions);
    $sign = signPrivate($msg);
    $value = $msg.":".$sign;
    
    # Re-use current coookie
    %cookies = CGI::Cookie->fetch;
    if (defined($cookies{"mediatex"})) {
	$cookie = $cookies{"mediatex"};
	$cookie->value($value);
	$cookie->domain('.'.$domain);
	$cookie->expires('+5m');
	$cookie->path('/');
    }
    else {
	# Create a new cookie
	$cookie = $q->cookie(
	    -name=>'mediatex',
	    -value=>$value,
	    -domain  =>  '.'.$domain,
	    -expires=>'+5m',+
	    -path=>'/');
    }

    if ($good_login eq 1 && $url ne "") {
	print $q->redirect(-uri=>uri_unescape($url), -cookie=>$cookie);
	exit(0);
    }

    print $q->header(-type=>'text/html', -cookie=>$cookie);
    
    print $q->start_html(
	-title => 'Autentication:',
	-bgcolor => 'white',
	-style => {
	    -code => '
                    /* Stylesheet code */
                    body {
                        font-family: verdana, sans-serif;
                    }
                    h2 {
                        color: darkblue;
                        border-bottom: 1pt solid;
                        width: 100%;
                    }
                    h4 {
                        color: red;
                    }
                    h5 {
                        color: blue;
                    }
                    div {
                        text-align: right;
                        color: steelblue;
                        border-top: darkblue 1pt solid;
                        margin-top: 4pt;
                    }
                    th {
                        text-align: right;
                        padding: 2pt;
                        vertical-align: top;
                    }
                    td {
                        padding: 2pt;
                        vertical-align: top;
                    }
                    /* End Stylesheet code */
                ',
	},
        );
}

# Outputs a web form
sub output_form {
    my ($q) = @_;

    print $q->h2("Authentication");
    if ($bad_login eq 1) {
	print $q->h4(' Bad credentials, please try again');
    }
    if ($good_login eq 1) {
	print $q->h5(' Good credentials (no redirection priovided)');
    }
    
    print $q->start_form(
	-name => 'main',
	-method => 'POST',
        );

    print $q->start_table;
    print $q->Tr(
	$q->td('Login:'),
	$q->td(
            $q->textfield(-name => "login", -size => 50)
	)
        );
    print $q->Tr(
	$q->td('Password:'),
	$q->td(
            $q->password_field(-name => "passwd", -size => 50)
	)
	);
    print $q->Tr(
	$q->td(''),
	$q->td(
	    $q->hidden(-name => "url", -default => $url)
	)
	);
    print $q->Tr(
	$q->td($q->submit(-value => 'Submit')),
	$q->td('&nbsp;')
        );
    print $q->end_table;
    print $q->end_form;
}

# Outputs a footer line and end html tags
sub output_end {
    my ($q) = @_;
    print $q->div("Single sign one");
    print $q->end_html;
}

sub decryptPEM {
    my ($file,$password) = @_;
    my $pem;
    my $pkey;
    
    $pem = Convert::PEM->new(
	Name => 'RSA PRIVATE KEY',
	ASN  => qq(
                  RSAPrivateKey SEQUENCE {
                      version INTEGER,
                      n INTEGER,
                      e INTEGER,
                      d INTEGER,
                      p INTEGER,
                      q INTEGER,
                      dp INTEGER,
                      dq INTEGER,
                      iqmp INTEGER
                  }
           ));
    
    $pkey =
	$pem->read(Filename => $file, Password => $password);
    
    if (!defined($pkey)) {
	die "No private key or bad key format: ".$file;
    }
    
    return(undef) unless ($pkey); # Decrypt failed.
    $pem->encode(Content => $pkey);
}

1;
