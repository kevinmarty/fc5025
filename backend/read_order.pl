#!/usr/bin/perl

$secsize=shift;
$minsec=min(@ARGV);
print "struct sector_list sector_list[]={";
for($i=0;$i<=$#ARGV;$i++) {
	$secnum=$ARGV[$i];
	$offset=($secnum-$minsec)*$secsize;
	print "{$secnum,$offset}";
	print "," if($i!=$#ARGV);
}
print "};\n";

sub min {
	my $min=999;
	my $foo;

	foreach $foo (@_) {
		$min=$foo if($foo<$min);
	}
	print "min of @_ is $min\n";
	return $min;
}
