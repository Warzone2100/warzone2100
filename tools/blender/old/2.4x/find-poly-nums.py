#!/usr/bin/env python

import pie, sys

for f in sys.argv[1:]:
	gen = pie.data_mutator(pie.parse(f))
	maximum = 0
	for token in gen:
		if token[pie.TOKEN_TYPE] is "polygon":
			sides = token[pie.FIRST + 1]
			if sides > maximum:
				maximum = sides
	if maximum > 4:
		print f, sides, "sided poly"
