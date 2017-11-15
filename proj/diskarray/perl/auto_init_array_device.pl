#!/usr/bin/perl

$owner = "dan";
$mke2fsoptions = "-j -m0";
$tune2fsoptions = "-c0";
$fsoptions = "-fstype=ext3,noatime";
$redirect = "2>&1 >/dev/null";

$applevolumes = "/etc/netatalk/AppleVolumes.default";
$smbconf = "/etc/samba/smb.conf.drv";
$autodrv = "/etc/auto.drv";

#
# This array contains the physical positions of the drives in the array.
#
@position = ('4-1', '3-1', '2-1', '1-1', '4-2', '3-2', '2-2', '1-2',
             '4-3', '3-3', '2-3', '1-3', '4-4', '3-4', '2-4', '1-4',
             '4-5', '3-5', '2-5', '1-5'
            );
#
# The array is located on the devices ata3 - ata22
#
$minposition = 3;
$maxposition = 22;

#
# Make sure that a device is passed to the program, and that it is a scsi
# drive, and that there are no partitions.
#
if ($#ARGV != 0) {
   exit;
}
$newdev = $ARGV[0];
if ($newdev !~ /^sd([a-z]|[a-h][a-z]|i[a-v])$/) {
   exit;
}
$pattern = "^" . $newdev . "(?:[1-9]|1[0-5])\$";
opendir DEV, "/dev";
while ($dev = readdir DEV) {
   if ($dev =~ /$pattern/) {
#      exit;
   }
}
closedir DEV;
#
# Build a couple of hashes that store the device name by (Column, Row) and 
# vice versa.
#
%drivearray = ();
%drivearraybydev = ();
$drivearray{"oob"} = "oob";
opendir BLOCKDIR, '/sys/block';
while ($dev = readdir BLOCKDIR) {
   if ($dev =~ /^sd([a-z]|[a-h][a-z]|i[a-v])$/) {
      opendir DEVICEDIR, "/sys/block/$dev/device";
      while ($entry = readdir DEVICEDIR) {
         if (($scsiid) = ($entry =~ /:([0-9]+):/)) {
            last;
         }
      }
      closedir DEVICEDIR;
      open UNIQUEID, "/sys/class/scsi_host/host$scsiid/unique_id";
      $uniqueid = <UNIQUEID>;
      close UNIQUEID;
      chomp $uniqueid;
      if (($uniqueid >= $minposition) && ($uniqueid <= $maxposition)) {
         $colrow = $position[$uniqueid - $minposition];
         $drivearray{$colrow} = $dev . "1";
         $drivearraybydev{$dev} = $colrow;
      } else {
         $drivearraybydev{$dev} = "oob";
      }
   }
}
closedir BLOCKDIR;
#
# Check if the new device is part of the array.
#
if (($colrow = $drivearraybydev{$newdev}) eq "oob") {
   exit;
}

#partitionandformatdrive($newdev);

#
# Get the UUID values for each drive in the array.
#
open BLKID, "/etc/blkid/blkid.tab";
@ids = grep /DEVNO="0x(?:08|4[0-6]|8[0-7])/, <BLKID>;
close BLKID;
%uuid = ();
foreach $id (sort @ids) {
   ($u) = ($id =~ / (UUID="[0-9a-f\-]+")/);
   ($dev) = ($id =~ /\/dev\/(sd[a-z]+)/);

print "$dev $u\n";
   $uuid{$drivearraybydev{$dev}} = $u;
}
#
# Back up the old configuration files.
#
rename $applevolumes, "$applevolumes.bak";
rename $autodrv, "$autodrv.bak";
rename $smbconf, "$smbconf.bak";
#
# Create the new ones.
#
open SMBCONF, ">$smbconf";
open AUTODRV, ">$autodrv";
open APPLEVOLUMES, ">$applevolumes";
open OLDAPPLEVOLUMES, "<$applevolumes.bak";
#
# Write the current contents of the Appletalk configuration file (minus the
# drive array to the new file.
#
while ($line = <OLDAPPLEVOLUMES>) {
   if (!($line =~ /^\/drv\/[1-4]-[1-5]/)) {
      print APPLEVOLUMES $line;
   }
}
close OLDAPPLEVOLUMES;
#
# Write the new contents to each configuration file.
#
foreach $colrow (sort keys %drivearray) {
   if (($column, $row) = ($colrow =~ /^([1-4])-([1-5])$/)) {
      print SMBCONF<<EOD;

[drv$colrow]
	comment = Drive (Column $column, Row $row)
	path = /drv/$colrow
	browseable = yes
	writeable = yes
	Valid users = $owner
EOD
      print AUTODRV "$colrow\t$fsoptions\t:$uuid{$colrow}\n";
      print APPLEVOLUMES "/drv/$colrow\t\t\"drv$colrow\"\n";
   }
}
close SMBCONF;
close AUTODRV;
close APPLEVOLUMES;
#
# Restart the services.
#
`/sbin/service smb reload $redirect`;
`/sbin/service autofs reload $redirect`;
`/sbin/service atalk reload $redirect`;
`/sbin/service linkmoviesd restart $redirect`;

exit;

sub partitionandformatdrive {
   my ($drive) = @_;
   my $dev = "/dev/$drive";
   my $dir = "/root/mnt.$$";

   open FDISK, "|/sbin/fdisk $dev $redirect";
   print FDISK "n\np\n1\n\n\nw\n";
   close FDISK;
   $dev .= "1";
   `/sbin/mke2fs $mke2fsoptions $dev $redirect`;
   `/sbin/tune2fs $tune2fsoptions $dev $redirect`;
   sleep 10;
   unlink $dir;
   mkdir $dir;
   `mount $dev $dir $redirect`;
   mkdir "$dir/Movies";
   mkdir "$dir/Television";
   mkdir "$dir/Files";
   chown((getpwnam($user))[2,3], glob("*"));
   `umount $dir $redirect`;
   unlink $dir;
}

# SCSI drive major device numbers: 0x08, 0x40-0x46, 0x80-0x87 /(0x(?:08|4[0-6]|8[0-7]))/

# Regex to determine a SCSI drive: /(sd(?:[a-z]|[a-h][a-z]|i[a-v]))/

# Regex to determine a SCSI partition: /(sd(?:[a-z]|[a-h][a-z]|i[a-v])(?:[1-9]|1[0-5]))/

