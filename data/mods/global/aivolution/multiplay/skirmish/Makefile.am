player%.slo:: playerX.slo.in
	cp -p $< $@

player%.vlo:: playerX.slo.in
	cp -p $< $@
	sed -r -i "s/script\s+"playerX\.slo"/script \"$(@:%.vlo=%.slo)\"/" $@
	sed -r -i "s/player\s+INT\s+X/player INT $(@:player%.vlo=%)/" $@

all: player0.slo player1.slo player2.slo player3.slo player4.slo player5.slo player6.slo player7.slo player0.vlo player1.vlo player2.vlo player3.vlo player4.vlo player5.vlo player6.vlo player7.vlo
