# gnuUnifont2things
a utility to turn gnu Unifont glyph descriptors into Hellschreiber sound files, ascii depictions, and eventually, gEDA PCB footprint silkscreen elements.

Usage:

	git clone https://github.com/erichVK5/gnuUnifont2things
	cd gnuUnifont2things
	make
	./main "\!@#$%^&*()[]{}':\",.<>\?\/-=_+\`~"
	sox -r 8000 -t raw -b 8 -e signed-integer output.raw output.wav

then either:

	play output.wav

or: 

	fldigi &
	file->audio->playback   and select output.wav for viewing on the waterfall


You can also try:

	./main "abcd Test Strings Here U+30U+262FU+002603U+2622U+2623"

The utility will manage extraction of individual Unicode descriptors of length 4, 6 or 8 fairly reliably.

Already done:

	- unicode and plain text conversion using the gnu Unifont bdf file, which includes Chinese, Korean and Japanese glyphs.
	- gaussian shaping of the start and stop of tones to minimise splatter
	- no memory leaks on testing with valgrind

TODO:

	- more command line options for tone spacing, duration, verbosity, tone shaping
	- more efficient use of memory
	- option for sequential multitone tone (S-MT) Hellschreiber, to reduce linearity demands on the transceiver, vs the currently implemented concurrent multitone (C/MT) HellSchreiber, and perhaps consider a chirped Hellschreiber option that sends a subset of the tones at once.
	- optimisation of the audio generation routine and glyph map code
	- interactive CLI with audio out, maybe for pulseAudio
	- audio generation code for other orientations of the glyphs, i.e. left rotated, etc...

Licence information:

gnu Unifont, see:
	http://unifoundry.com/

see also:
	http://unifoundry.com/pub/unifont-8.0.01/unifont-8.0.01.bmp for the complete glyph table

Useful links:

	http://www.utf8-chartable.de
	http://www.qsl.net/zl1bpu/HELL/Index.htm

bitmap2waterfall.cc v1.0, 
main.cc v1.0, 
comprising gnuUnifont2things

Copyright (C) 2016 Erich S. Heinzle, a1039181@gmail.com

    see LICENSE-gpl-v2.txt for software license
    see README.txt
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
    
    bitmap2waterfall.cc (C) 2016 Erich S. Heinzle a1039181@gmail.com
