-- $Id: ackermann.lua.html,v 1.5 2004/07/03 07:11:33 bfulgham Exp $
-- http://www.bagley.org/~doug/shootout/

function Ack(M, N)
    if (M == 0) then
	return( N + 1 )
    end
    if (N == 0) then
	return( Ack(M - 1, 1) )
    end
    return( Ack(M - 1, Ack(M, (N - 1))) )
end

NUM = tonumber((arg and arg[1])) or 1
io.write("Ack(3,", NUM ,"): ", Ack(3,NUM), "\n")
