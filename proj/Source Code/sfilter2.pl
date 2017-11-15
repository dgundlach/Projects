#!/usr/bin/perl

@tabs = ("\t\t\t\t", "\t\t\t", "\t\t", "\t");

while (<STDIN>) {
   $line = $_;
   chop($line);
   ($title, $artist) = split "\t", $line;
   print $artist . "\t" . $title . "\n";
}
