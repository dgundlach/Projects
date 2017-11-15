#!/usr/bin/perl

$pt = 12;

@rowsperpt = (0,0,0,0,0,0,0,0,74,66,59,54,49,46,42,39,37);

if ($pt < 8 || $pt > 16) {
   die "7 < pt < 17\n";
}
$rows = $rowsperpt[$pt];
$row = 0;
$c1 = "";
$c2 = "";
$c3 = "";

while (<STDIN>) {
   if ($row == $rows) {
      print "$c1$c2$c3";
      $row = 0;
      $c1 = "";
      $c2 = "";
      $c3 = "";
   }
   ($distance, $action, $road) = split /[\t\n]/;
   $c1 .= "$distance\n";
   $c2 .= "$action\n";
   $c3 .= "$road\n";
   $row++;
}
if ($row) {
   while ($row < $rows) {
      $c1 .= "\n";
      $c2 .= "\n";
      $c3 .= "\n";
      $row++;
   }
   print "$c1$c2$c3";
}
