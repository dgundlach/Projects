#!/usr/bin/perl
$i = 0;
$delim = "\t";
while (<STDIN>) {
   if ($i++) {
      ($in, $turntime, $turndist, $action, $on, $road, $exit, $finishtime, $finishdist, $totaltime, $distance) = split /\t/;
      $action =~ s/Go straight.*/C/;
      $action =~ s/Turn left.*/L/;
      $action =~ s/Turn right.*/R/;
      $action =~ s/Keep left.*/BL/;
      $action =~ s/Keep right.*/BR/;
      $action =~ s/Finish.*/End/;
      $turndist =~ s/ mi//;
      if ($turndist == "") {
         $turndist = "0.00";
      }
      $distance =~ s/ mi//;
      $ld = length($distance);
#      print "$turndist$delim$distance$delim$action$delim$road\n";
      print "$distance$delim$action$delim$road\n";
   }
}
