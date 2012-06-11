#!/usr/bin/env lua

bottles = 9999
while (bottles > 0) do
	print(bottles, "bottles of beer on the wall,")
	print(bottles, "bottles of beer,")
	print("Take none down, pass none around,")
	print(bottles - 1, "bottles of beer on the wall.");
	print("");
	bottles = bottles - 1
end
