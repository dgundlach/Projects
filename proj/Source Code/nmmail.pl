#!/usr/bin/perl

use GetOpt::Long;

$MYNAME = $0;
$USAGE = <<EOF;
usage: $MYNAME -to email [-bcc email] [-cc email] [-from email]
[-host address] [-subject text] [-i] [-v] [-purge] [-help]
[-a[u|o][b] file] [-u name] [-mail file | text]
EOF

sub help {

   print <<EOF;
$MYNAME -- Emulates the nmmail program's command line functionality.
This script provides a wrapper for most of the command line options
available from TELAMON's nmmail program on MPE/iX. It will accept the
appropriate options and invoke the UNIX mailx program to send an email
message and/or file attachment to the specified email address.
Warning: Not all functionality is available do the difference in the
mail programs themself. Please review individual options for
specific differences.

$USAGE

options:

-a[u|o][b] file

Attach a file to the message. The file name is in MPE's
naming structure of FILE[.GROUP[.ACCOUNT]]. The second and
third characters have the following meaning.

u - UUENCODE the attached file. (This is the default).

o - Other, sent as is.

b - Don't strip blanks. (This option is ignored).

-b[cc] email

The email address to send this message to under the bcc
setting. To specify more than one email address this option
may be repeated. Note: This option will be ignored.

-c[c] email

The email address to send this message to under the cc
setting. To specify more than one email address this option
may be repeated. Note: This option will be ignored.

-f[rom] email

The email address the message is from. Note: This option will
be ignored.

-help

Show this help text.

-h[ost] address

Host ip address of the SMTP server used to transmit the
message. Note: This option will be ignored.

-i

Interactive (prompting) mode. Note: This option will be
ignored.

-m[ail] file

File to be mailed as the message text. The file name is in
MPE's naming structure of FILE[.GROUP[.ACCOUNT]]. If the
-mail option is not specified the message text will be read
from the stdin.

-p[urge]

Purge the -mail file when done.

-q[uiet]

Quite mode. Suppress delivery confirmation message. Note: This
option will be ignored.

-r[eplyto] email

An email address to send this message to under the reply-to
setting. Note: This option will be ignored.

-s[ubject] subject

Provides a subject for the email message. A multi-word subject
must be enclosed in quotes. (i.e. "Sharedraft returns").

text

Text consists of any word(s) following the options. This script
will assume any word that it does not recognise as a option
is the start of the text that will be used as the email's
message. If the -mail option was specified then the text will
be treated as an error condition.

-t[o] email

Specifies the email address of the person to whom this email
is to be sent (i.e. psmith@summit.fiserv.com). To specify
more than one email address this option may be repeated.

-u name

An alternate name given to the attached uuencoded file. If
this option is not specified the MPE file name specified in
the -attach option will be used.

-v

Verbose mode. Can be used for debugging purposes to determine
what this script is doing and the values of variables being
used.

Defaults from system variables:

MAILSMTPHOST - smtp host ip address (see -host option).
MAILFROM - from email address (see -from option).
MAILSUBJECT - subject test (see -subject option).
EOF

   exit 0;
}

sub error;
   my ($message) = @_;
   print "Error: $message\n";
   exit 1;
}

$result = GetOptions ("t|to=s"           => \$mailto,
                      "b|bcc=s"          => \$bccto,
                      "c|cc=s"           => \$ccto,
                      "f|from=s"         => \$mailfrom,
                      "h|host=s"         => \$host,
                      "s|subject=s"      => \$subject,
                      "i"                => \$interactive,
                      "v"                => \$verbose,
                      "p|purge"          => \$purge,
                      "h|help"           => \$help,
                      "a|au|aub=s"       => \$attachuuencode,
                      "ao|aob=s"         => \$attachother,
                      "u|uuencodename=s" => \$uuencodename,
                      "m|mail            => \$mailfile
);
