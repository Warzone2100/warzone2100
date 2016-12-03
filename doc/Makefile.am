if A2X_AVAILABLE
if WKHTMLTOPDF_AVAILABLE

qsgpdf = quickstartguide.pdf

else

qsgpdf =

endif
endif

nobase_dist_doc_DATA = \
	images/artillery-far-away.jpg \
	images/artillery-sensor.jpg \
	images/attackrange.jpg \
	images/awaymission.jpg \
	images/building.jpg \
	images/building-select.jpg \
	images/cb-sensor.png \
	images/cb-sensor-vtol.png \
	images/cheapweapon.png \
	images/commander-factory-assignment.jpg \
	images/commander-panel.jpg \
	images/commander.png \
	images/commandpanel.png \
	images/design-bars.jpg \
	images/design.jpg \
	images/design-more.png \
	images/design-screen.jpg \
	images/design-unit.jpg \
	images/expensiveweapon.png \
	images/firing.jpg \
	images/hq.png \
	images/indirect-fire-support.jpg \
	images/intelligencedisplay.jpg \
	images/interface.jpg \
	images/logo.png \
	images/manufacture.jpg \
	images/manufacture-select.jpg \
	images/minimap.jpg \
	images/movement.jpg \
	images/oilresource.jpg \
	images/powerbar.jpg \
	images/powerupgrade.png \
	images/rallypoints.jpg \
	images/recycling.jpg \
	images/research.jpg \
	images/research-select.jpg \
	images/retreatthreshold.jpg \
	images/return.jpg \
	images/satellite-uplink.png \
	images/sensor.png \
	images/sensor-tower.png \
	images/transport.jpg \
	images/unitordersmenu.jpg \
	images/wss.png \
	images/www.png \
	ScriptingManual.htm \
	docbook-xsl.css

EXTRA_DIST = \
	quickstartguide.asciidoc \
	warzone2100.6.asciidoc

DISTCLEANFILES = \
	quickstartguide.html \
	quickstartguide.pdf \
	warzone2100.6

if A2X_AVAILABLE

nobase_dist_doc_DATA += \
	quickstartguide.html \
	$(qsgpdf)

dist_man6_MANS = \
	warzone2100.6

warzone2100.6: warzone2100.6.asciidoc
	$(A2X) -f manpage -D $(srcdir) $(srcdir)/warzone2100.6.asciidoc

quickstartguide.html: quickstartguide.asciidoc
	$(A2X) -f xhtml -D $(srcdir) $(srcdir)/quickstartguide.asciidoc

if WKHTMLTOPDF_AVAILABLE

quickstartguide.pdf: quickstartguide.html
	$(WKHTMLTOPDF) $(srcdir)/quickstartguide.html $(srcdir)/quickstartguide.pdf

endif

qsg: quickstartguide.html $(qsgpdf)

manpage: warzone2100.6

dist-hook: qsg manpage

endif

if INSTALLER

all: qsg

endif
