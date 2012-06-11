function Fib(X)
  if (X > 2) then
    return (Fib(X - 1) + Fib(X - 2))
  end
  return 1
end

I = 1
while I <= 25 do
  print(Fib(I))
  I = I + 1
end

