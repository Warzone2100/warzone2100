player%.slo:: playerX.slo.in
	cp -p $< $@

player%.vlo:: playerX.vlo.in
	cp -p $< $@
	sed -r -i "s/script\s+\"playerX\.slo\"/script \"$(@:%.vlo=%.slo)\"/" $@
	sed -r -i "s/player\s+INT\s+X/player INT $(@:player%.vlo=%)/" $@
	@if [ ! `grep -c -P "script\s+\"playerX\.slo\"" $@` -eq 0  -o  ! `grep -c -P "script\s+\"$(@:%.vlo=%.slo)\"" $@` -eq 1 ] ; then echo "Template script wrong?!" ; exit 1 ; fi
	@if [ ! `grep -c -P "player\s+INT\s+X" $@` -eq 0  -o  ! `grep -c -P "player\s+INT\s+$(@:player%.vlo=%)" $@` -eq 1 ] ; then echo "Template script wrong?!" ; exit 1 ; fi

all: player0.slo player1.slo player2.slo player3.slo player4.slo player5.slo player6.slo player7.slo player0.vlo player1.vlo player2.vlo player3.vlo player4.vlo player5.vlo player6.vlo player7.vlo
