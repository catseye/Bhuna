#!/usr/bin/env perl

while ($d = <STDIN>) {
	$ary[$1]++ if $d =~ /^\#(\d+)\:/;
}

printf "%8s %8s\n", "Instr#", "Count";
for ($i = 0; $i <= $#ary; $i++) {
	printf "%8d %8d\n", $i, $ary[$i] if $ary[$i] > 0;
}

