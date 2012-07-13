Bhuna
=====

Bhuna is a small, garbage-collected language with a simple syntax, closures,
inferred types, lightweight processes, and support for UTF-8 source code.

It was implemented partly to see how closely I could match the performance
of Lua's interpreter.

It is not well-specified; it was designed more-or-less by fiat of building
the interpreter.  I originally wrote it on FreeBSD, while figuring out how
to implement closures (the hard way.)  So for the longest time, building it
required BSD `make` and the associated support files from FreeBSD.  Now,
however, it has a self-contained `Makefile` and can be built with GNU `make`.

It is also now covered under a BSD-style license; see the file `LICENSE`.

The Bhuna project is basically dead.  See [Kosheri][] for a virtual machine
that sprang from its ashes.

[Kosheri]: https://github.com/catseye/Kosheri
