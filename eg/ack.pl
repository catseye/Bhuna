#!/usr/bin/env perl
# $Id: ackermann.perl.html,v 1.5 2004/07/03 07:11:34 bfulgham Exp $
# http://www.bagley.org/~doug/shootout/

# With help from Ernesto Hernandez-Novich
use integer;

# Note:  If memoization were allowed in this program, we could
# do so by adding:
# use Memoization;
# memoize("Ack");
#

# It's prettier but slower to do this
#sub Ack {
#    my($M, $N) = @_;
#    return( $N + 1 )         if ($M == 0);
#    return( Ack($M - 1, 1) ) if ($N == 0);
#    Ack($M - 1, Ack($M, $N - 1));
#}

# in our quest for speed, we must get ugly:
# it helps reduce stack frame size a little bit
# from Leif Stensson
sub Ack {
    return $_[0] ? ($_[1] ? Ack($_[0]-1, Ack($_[0], $_[1]-1))
		    : Ack($_[0]-1, 1))
	: $_[1]+1;
}

my $NUM = $ARGV[0];
$NUM = 1 if ($NUM < 1);
my $ack = Ack(3, $NUM);
print "Ack(3,$NUM): $ack\n";
