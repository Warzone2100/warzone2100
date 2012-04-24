#!/bin/sh

for i in *.po; do
	echo -n $i:\ ; msgfmt --statistics $i 2>&1 | awk 'ORS="% done - " {print 100*$1/($1+$4+$7)}'; msgfmt --statistics $i 2>&1
done
