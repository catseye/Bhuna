#!/usr/bin/python
# $Id: ackermann.python.html,v 1.5 2004/07/03 07:11:34 bfulgham Exp $
# http://www.bagley.org/~doug/shootout/
# from Brad Knotwell

import sys

def Ack(M, N):
    if (not M):
        return( N + 1 )
    if (not N):
        return( Ack(M-1, 1) )
    return( Ack(M-1, Ack(M, N-1)) )

def main():
    NUM = int(sys.argv[1])
    sys.setrecursionlimit(3000)
    print "Ack(3,%d): %d" % (NUM, Ack(3, NUM))

main()
