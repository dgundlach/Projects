#!/usr/bin/perl

$totaldistance = 0.00;
foreach $argnum (0 .. $#ARGV) {
   open INF, $ARGV[$argnum];
   while (<INF>) {
      ($distance, $action, $road) = split /[\t\n]/;
      $distance += $totaldistance;
      if ($road ne "") {
         printf "%.2f\t%s\t%s\n", $distance, $action, $road;
      } else {
         print "\n";
      }
   }
   $totaldistance = $distance;
   close INF;
}
