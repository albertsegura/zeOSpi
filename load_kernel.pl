#!/usr/bin/perl
if ($#ARGV != 0) {
	print "Usage: perl script.pl  device\n";
	exit 1;
}

my $disp = $ARGV[0];
unless ( -b $disp ){
	die "device not found";
}

@out = split(/ /,`mount | grep $disp`);
$folder = $out[2];

if (@out) {
	`rm $folder/kernel.img 2> /dev/null`;
	`cp kernel.img $folder`;
	`umount $folder`;
	print "kernel loaded!\n";
}
else {
	die "device not found";
}
