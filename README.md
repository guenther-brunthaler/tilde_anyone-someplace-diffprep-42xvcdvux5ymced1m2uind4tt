diffprep
========

Abstract
--------

This project is about a preprocessor/postprocessor for "diff" and
"patch", allowing to create word-, character-, byte- or bit-based
diffs and patches.

It can also be "mis-used" as a hex-dump or bit-dump utility,
supporting the conversion of manually edited dump files back to
the original binary data (similar to "xxd -r").

The utility has no external library dependencies and only uses
the standard C runtime library.

It is written in portable ANSI-C 89 as a single source file, and
should therefore be easy to build and install.


Keywords
--------

word-diff, character-diff, bit-diff, hexdump, reverse hexdump,
bit-dump, reverse bit-dump, word-based merge, character-based
merge, bit-based merge, word-wrapped merge, binary diff, binary
patch, binary merge.


Examples
--------

Word-diff files 1.txt and 2.txt with bash as shell:

	$ diff -u <(diffprep 1.txt) <(diffprep 2.txt)

The same with a normal POSIX shell:

	$ diffprep 1.txt > 1-w.txt
	$ diffprep 2.txt > 2-w.txt
	$ diff -u 1-w.txt 2-w.txt

Create a hex-dump similar to "hexdump -C 1.txt":

	$ diffprep -axn16 1.txt

Create an editable hexdump of file data.bin with 20 bytes per line:

	$ diffprep -xn20 data.bin > data-dump.txt

Re-create binary file data.bin from data-dump.txt after manually
editing that file:

	$ diffprep -X data-dump.txt > data.bin

Create a patch out-w.patch from the word-based differences of 1.txt and 2.txt
which treats all whitespace equal (newlines and normal spaces will be
considered equal). Then apply that patch to some file 1-modified.txt, again
treating newlines and spaces as interchangable:

	$ diffprep 1.txt > 1-w.txt
	$ diffprep 2.txt > 2-w.txt
	$ diff -bu 1-w.txt 2-w.txt > out-w.patch
	$ diffprep 1-modified.txt > 1-modified-w.txt
	$ patch -l 1-modified-w.txt out-w.patch
	$ diffprep -W 1-modified-w.txt > 1-modified.txt

Compare two 24-bit RGB bitmap image files base.png and base_w_logo.png and
show the different RGB pixels (requires imagemagick to be installed):

	$ convert base.png base.ppm
	$ convert base_w_logo.png base_w_logo.ppm
	$ ppm2rgb() { local x; for x in `seq 3`; do read x; done; cat; }
	$ ppm2rgb < base.ppm > base.rgb
	$ ppm2rgb < base_w_logo.ppm > base_w_logo.rgb
	$ diffprep -xn3 base.rgb > base-x.txt
	$ diffprep -xn3 base_w_logo.rgb > base_w_logo-x.txt
	$ diff -u base-x.txt base_w_logo-x.txt

Display the different bits of two bitstream files 1.bin and 2.bin:

	$ diffprep -b 1.bin > 1-b.txt
	$ diffprep -b 2.bin > 2-b.txt
	$ diff -u 1-b.txt 2-b.txt

How to build
------------

Under UNIX, Linux, Cygwin or MinGW it is simple: Just run

	$ make

to compile and build the program. If you want to build with
specific compiler flags, you can invoke 'make' like this instead:

	$ make CFLAGS="-D NDEBUG -O2 -s"

If you don't have a POSIX-compliant "make" utility, but some
C/C++ IDE is available instead, just create a new C project in
your IDE and import diffprep.c as the only source file. Then
build the project.

If you have neither "make" nor an IDE, you might still have a C compiler
installed. Try this:

	$ cc -o diffprep -D NDEBUG -s -O2 diffprep.c

In all cases, after successful compilation, run

	$ ./diffprep -h

for displaying help, copyright and usage information.

You might also want to to install the built executable, so that
it can be invoked from anywhere:

	$ sudo cp diffprep /usr/local/bin/ \
	  && sudo chmod +x /usr/local/bin/diffprep


License Information
-------------------

Copyright (c) 2016 Guenther Brunthaler. All rights reserved.

This program is free software.
Distribution is permitted under the terms of the GPLv3.
