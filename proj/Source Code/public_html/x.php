<?php

    $defaultcomment = "Rode solo.";
    $currdate = date("Y-m-d");
 
    $today = getdate();
    $year = $today['year'];
    $today = getdate(mktime(0,0,0,1,1,$year));
    $weekoffset = $today['wday'];
    
    $database=mysql_connect('localhost', 'apache', 'zxasqw12');
    mysql_select_db('apache');

    $t = $year;
    $d1 = (int)($t / 1000);
    $t = $t % 1000;
    $d2 = (int)($t / 100);
    $t = $t % 100;
    $d3 = (int)($t / 10);
    $d4 = $t % 10;
    $title="R I D E L O G&nbsp;&nbsp; $d1 $d2 $d3 $d4";
    echo "<html>\n";
    echo "<title>$title</title>\n";
    echo "<body><br><br>\n";
    echo "<center>\n";

    $me = $_SERVER['PHP_SELF'];
    if ($_SERVER['REQUEST_METHOD'] == 'POST') {
        $odometer = $_POST['odometer'];
        if ($odometer) {
            if ($_POST['submit'] == "Delete") {
                $sql = "delete from ridelog where odometer = " . $odometer;
            } else {
                $sql = "insert into ridelog (odometer";
                $sql2 = ") values (" . $_POST['odometer'];
                $comments = $_POST['comments'];
                $ridedate = $_POST['ridedate'];
                if ($comments) {
                    $sql .= ", comments";
                    $sql2 .= ", '" . mysql_real_escape_string($comments) . "'";
                }
                if ($ridedate) {
                    $sql .= ", ridedate";
                    $sql2 .= ", '" . $ridedate . "'";
                }
                $sql .= $sql2 . ")";
            }
#            echo "$sql\n";
            $result = mysql_query($sql, $database);
            mysql_free_result($result);
        } else {
            echo "No odometer value entered.\n";
        }
    }
    $sql = "select * from ridelog where ridedate < '$year-01-01' order by " .
           "odometer desc limit 1";
    $result = mysql_query($sql, $database);
    $row = mysql_fetch_array($result, MYSQL_ASSOC);
    $starting = $row['odometer'];
    mysql_free_result($result);

    $sql = "select * from ridelog where ridedate >= '$year-01-01' and ".
            "ridedate <= '$year-12-31' order by odometer";
    $result = mysql_query($sql, $database);
    $nrows = mysql_num_rows($result);
    $weekno = 0;
    $currmonth = 1;
    $monthly = 0;
    $yearly = 0;
    $weekbg="ffffff";
    $monbg="ffffff";
    $milebg="ffffff";
    $yrbg="ffffff";
    $yrtest=500;
    $sp="&nbsp;";
    if ($nrows) {
        echo "<table border=2 cellspacing=0 cellpadding=4 bgcolor=ffffff>\n";
        echo "<caption><center><h1>$title</h1></center></caption>\n"; 
        echo "<tr>\n";
        echo "<td><center>Seq</center></td>\n";
        echo "<td><center>Date</center></td>\n";
        echo "<td><center>Week</center></td>\n";
        echo "<td><center>Odometer</center></td>\n";
        echo "<td><center>Mileage</center></td>\n";
        echo "<td><center>Weekly</center></td>\n";
        echo "<td><center>Monthly</center></td>\n";
        echo "<td><center>Yearly</center></td>\n";
        echo "<td><center>Comments</center></td>\n";
        echo "</tr>\n";
        for ($i = 0; $i < $nrows; $i++) {
            $row = mysql_fetch_array($result, MYSQL_ASSOC);
            $ridedate = getdate(strtotime($row['ridedate']));
            $mon = $ridedate['mon'];
            $mday = $ridedate['mday'];
            $yday = $ridedate['yday'];
            if ((int)$mon != $currmonth) {
                $currmonth = $mon;
                $monthly = 0;
                $monbg="00cccc";                
            }
            $newweekno = (int)(($yday + $weekoffset) / 7);
            if ((int)$newweekno != $weekno) {
                $weekno = $newweekno;
                $weekly = 0;
                $weekbg="00cc00";
            }
            $odometer = $row['odometer'];
            $mileage = $odometer - $starting;
            if ($mileage > 100) $milebg = "cc00cc";
            $starting = $odometer;
            $weekly += $mileage;
            $monthly += $mileage;
            $yearly += $mileage;
            if ($yearly >= $yrtest) {
                $yrbg = "cc0000";
                $yrtest += 500;
            }
            echo "<tr>\n";
            printf("<td align=center>%s%s%s</td>\n", $sp, $i + 1, $sp);
            printf("<td align=center>%02s/%02s</td>\n", $mon, $mday);
            printf("<td align=right>%s%s%s</td>\n", $sp, $weekno + 1, $sp);
            printf("<td align=right>%s%.1f%s</td>\n", $sp, $odometer, $sp);
            printf("<td align=right bgcolor=%s>%s%.1f%s</td>\n", $milebg, $sp, $mileage, $sp);
            printf("<td align=right bgcolor=%s>%s%.1f%s</td>\n", $weekbg, $sp, $weekly, $sp);
            printf("<td align=right bgcolor=%s>%s%.1f%s</td>\n", $monbg, $sp, $monthly, $sp);
            printf("<td align=right bgcolor=%s>%s%.1f%s</td>\n", $yrbg,$sp, $yearly, $sp);
            printf("<td align=left>%s</td>\n", $row['comments']);
            echo "</tr>\n";
            $monbg="ffffff";
            $weekbg="ffffff";
            $milebg="ffffff";
            $yrbg="ffffff";
        }
    }
    mysql_free_result($result);
    mysql_close($database);

    echo "<form action=$me method=post>\n";
    echo "<p>\n";
    echo "<br><br><table border=0 cellspacing=0 cellpadding=4 bgcolor=ffffff>\n";
    echo "<caption><center>Insert/Delete Data Here</center></caption>\n"; 
    echo "<tr>\n";
    echo "<td align=right><label for=ridedate>Ride Date (yyyy-mm-dd): </label></td>\n";
    echo "<td align=left><input type=text size=11 maxlength=10 value=$currdate name=ridedate></td>\n";
    echo "</tr>\n";
    echo "<tr>\n";
    echo "<td align=right><label for=comments>Comments: </label></td>\n";
    echo "<td align=left><input type=text size=61 maxlength=60 value=\"$defaultcomment\" name=comments></td>\n";
    echo "</tr>\n";
    echo "<tr>\n";
    echo "<td align=right><label for=odometer>Odometer: </label></td>\n";
    echo "<td align=left><input type=text size=11 maxlength=10 name=odometer></td>\n";
    echo "</tr>\n";


    echo "<tr>\n";
    echo "<td></td>\n";

    echo "<td>\n";
    echo "<table width=100%>\n";
    echo "<tr>\n";
    echo "<td align=left><input type=submit name=submit value=Insert><input type=reset></td>\n";
    echo "<td align=right><input type=submit name=submit value=Delete></td>\n";
    echo "</tr>\n";
    echo "</table>\n";

    echo "</td>\n";
    echo "</tr>\n";
    echo "</table>\n";


    echo "</center\n";
    echo "<br><br></body>\n";
    echo "</html>\n";
?>

