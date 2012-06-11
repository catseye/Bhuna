#!/usr/bin/perl

$bottles = 9999;
while ($bottles > 0) {
	print "$bottles bottles of beer on the wall,\n";
	print "$bottles bottles of beer,\n";
	print "Take none down, pass none around,\n";
	$bottles = $bottles - 1;
	print "$bottles bottles of beer on the wall.\n";
	print "\n";
}
