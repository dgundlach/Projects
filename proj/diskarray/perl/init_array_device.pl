#!/usr/bin/perl

$mke2fsoptions = "-j -m0";
$tune2fsoptions = "-c0";
$fsoptions = "-fstype=ext3,noatime";
$owner = "dan";

$applevolumes = "/etc/netatalk/AppleVolumes.default";
$smbconf = "/etc/samba/smb.conf.drv";
$autodrv = "/etc/auto.drv";

#
# This array contains the physical positions of the drives in the array.
# Looks like I didn't wire it in order.
#
@position = ('0-0', '0-1', '0-2', '4-1', '3-1', '2-1', '1-1', '4-2', '3-2',
             '2-2', '1-2', '4-3', '3-3', '2-3', '1-3', '4-4', '3-4', '2-4',
             '1-4', '4-5', '3-5', '2-5', '1-5', '0-3', '0-4'
            );

$minposition = 3;
$maxposition = 22;

$redirect = "2>&1 >/dev/null";

$reconfigure = 0;

sub chown_by_name {
   local ($user, $pattern) = @_;
   chown((getpwnam($user))[2,3], glob($pattern));
}

sub getdevices {
   my ($pattern) = @_;
   my $list = "";
   my $dev = "";

   opendir DEV, '/dev';
   while ($dev = readdir DEV) {
      if (($letter) = ($dev =~ /$pattern/)) {
         $list .= $letter;
      }
   }
   closedir DEV;
   return $list;
}

sub partitionandformatdrive {
   my ($drive) = @_;
   my $dev = "/dev/$drive";
   my $dir = "/tmp/mnt.$$";

   print "\nOK to format this drive? ";
   $a = <STDIN>;
   print "\n";
   if ($a !~ /^[Yy]/) {
      print "Not formatting\n";
   } else {
      print "Creating the partition...\n";
      open FDISK, "|/sbin/fdisk $dev $redirect";
      print FDISK "n\np\n1\n\n\nw\n";
      close FDISK;
      $dev .= "1";
      print "\nFormatting the partition...";
      `/sbin/mke2fs $mke2fsoptions $dev $redirect`;
      `/sbin/tune2fs $tune2fsoptions $dev $redirect`;
      sleep 10;
      unlink $dir;
      mkdir $dir;
      print "\nMounting the partition...";
      `mount $dev $dir $redirect`;
      print "\nCreating the default directories and setting permissions...";
      mkdir "$dir/Movies";
      mkdir "$dir/Television";
      mkdir "$dir/Files";
      chown_by_name($owner, "*");
      print "\nUnmounting the partition...";
      `umount $dir $redirect`;
      print "\n";
      unlink $dir;
      $reconfigure = 1;
   }
}

#
# Get the list of drives.
#
$currentdrives = getdevices('^sd([A-Za-z])$');
#
# Check if there are any unpartitioned drives.
#
$partitioneddrives = getdevices('^sd([a-zA-Z])[1-9]$');
$pattern = "[$partitioneddrives]";
$currentdrives =~ s/$pattern//g;
if (length($currentdrives) == 0) {
   die "No unpartitioned drives.\n";
}
#
# Build a couple of hashes that store the device name by (Column, Row) and 
# vice versa.
#
%drivearray = ();
%drivearraybydev = ();
opendir BLOCKDIR, '/sys/block';
while ($dev = readdir BLOCKDIR) {
   if ($dev =~ /^sd[a-zA-Z]$/) {
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
      $colrow = $position[$uniqueid];
      $drivearray{$colrow} = $dev . "1";
      $drivearraybydev{$dev} = $colrow;
   }
}
closedir BLOCKDIR;
#
# Find the unique id of each unpartitioned drive, and look up the physical position
# (Column, Row).  Partition and format the drive.
#
$len = length $currentdrives;
for ($pos=0; $pos<$len; $pos++) {
   $device = "sd" . substr $currentdrives, $pos, 1;
   $colrow = $drivearraybydev{$device};
   $column = substr($colrow,0,1);
   $row = substr($colrow,2,1);
   print "\nFound new drive: $device at (Column: $column, Row: $row).\n";
   partitionandformatdrive($device);
}
#
# Are we reconfiguring?  Rebuild the configuration files and reload the daemons.
#
if ($reconfigure == 1) {
#
# Get the UUID values for each drive in the array.
#
   @ids = grep /^\/dev\/sd[A-Za-z]/, `/sbin/blkid`;
   %uuid = ();
   foreach $id (sort @ids) {
      ($dev, $u) = ($id =~ /^\/dev\/(sd[a-zA-Z])1: (UUID=\"[a-z0-9\-]+\")/);
      $uuid{$drivearraybydev{$dev}} = $u;
   }
#
# Back up the old configuration files.
#
   print "Reconfiguring...";
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
      ($column, $row) = ($colrow =~ /^([1-4])-([1-5])$/);
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
   close SMBCONF;
   close AUTODRV;
   close APPLEVOLUMES;
#
# Restart the services.
#
   print "\nRestarting services...";
   `/sbin/service smb reload $redirect`;
   `/sbin/service autofs reload $redirect`;
   `/sbin/service atalk reload $redirect`;
   `/sbin/service linkmoviesd restart $redirect`;
   print "\n\n";
}

exit;
