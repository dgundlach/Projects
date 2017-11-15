#!/usr/bin/perl

@tabs = ("\t\t\t\t", "\t\t\t", "\t\t", "\t");

while (<STDIN>) {
   $line = $_;
   chop($line);
   ($title, $artist) = split " - ", $line;
   $_ = $title;
   $title =~ s/\"//g;
   $_ = $artist;
   if ($_ =~ /,/) {
      ($last, $first) = split ", ";
      @words = split " ", $first;
      $i = 0;
      foreach $word (@words) {
         if ($i == 0) {
            $artist = $word . " " . $last;
            $i = 1;
         } else {
            $artist .= " ". $word;
         }
      }
   }
   $l = int(length($title) / 8);
#   print $title . $tabs[$l] . $artist . "\n";
   print $title . "\t" . $artist . "\n";
}
