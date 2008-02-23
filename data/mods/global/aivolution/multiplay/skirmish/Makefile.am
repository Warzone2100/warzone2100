player%.slo:: player0.slo
	cp -p $< $@

player%.vlo:: player0.vlo
	cp -p $< $@
	sed -r -i "s/script\s+\"player0\.slo\"/script \"$(@:%.vlo=%.slo)\"/" $@
	sed -r -i "s/player\s+INT\s+0/player INT $(@:player%.vlo=%)/" $@
	@if [ ! `grep -c -E "script[[:space:]]\s+\"player0\.slo\"" $@` -eq 0  -o  ! `grep -c -P "script[[:space:]]+\"$(@:%.vlo=%.slo)\"" $@` -eq 1 ] ; then echo "Template script wrong?!" ; exit 1 ; fi
	@if [ ! `grep -c -E "player+INT[[:space:]]+0" $@` -eq 0  -o  ! `grep -c -P "player[[:space:]]+INT[[:space:]]+$(@:player%.vlo=%)" $@` -eq 1 ] ; then echo "Template script wrong?!" ; exit 1 ; fi

all: player0.slo player1.slo player2.slo player3.slo player4.slo player5.slo player6.slo player7.slo player0.vlo player1.vlo player2.vlo player3.vlo player4.vlo player5.vlo player6.vlo player7.vlo

clean:
	rm -f player[1234567].[vs]lo
