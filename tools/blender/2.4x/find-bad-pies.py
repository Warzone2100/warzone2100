#!/usr/bin/env python

import pie, sys

for f in sys.argv[1:]:
	gen = pie.bsp_mutator(pie.data_mutator(pie.parse(f)))
	for token in gen:
		if token[pie.TOKEN_TYPE] is "error":
			print f, token[pie.LINENO], token[pie.ERROR]
