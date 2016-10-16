project(doc)

add_custom_target(doc_wz2100)

set(doc_DATA
	"images/artillery-far-away.jpg"
	"images/artillery-sensor.jpg"
	"images/attackrange.jpg"
	"images/awaymission.jpg"
	"images/building.jpg"
	"images/building-select.jpg"
	"images/cb-sensor.png"
	"images/cb-sensor-vtol.png"
	"images/cheapweapon.png"
	"images/commander-factory-assignment.jpg"
	"images/commander-panel.jpg"
	"images/commander.png"
	"images/commandpanel.png"
	"images/design-bars.jpg"
	"images/design.jpg"
	"images/design-more.png"
	"images/design-screen.jpg"
	"images/design-unit.jpg"
	"images/expensiveweapon.png"
	"images/firing.jpg"
	"images/hq.png"
	"images/indirect-fire-support.jpg"
	"images/intelligencedisplay.jpg"
	"images/interface.jpg"
	"images/logo.png"
	"images/manufacture.jpg"
	"images/manufacture-select.jpg"
	"images/minimap.jpg"
	"images/movement.jpg"
	"images/oilresource.jpg"
	"images/powerbar.jpg"
	"images/powerupgrade.png"
	"images/rallypoints.jpg"
	"images/recycling.jpg"
	"images/research.jpg"
	"images/research-select.jpg"
	"images/retreatthreshold.jpg"
	"images/return.jpg"
	"images/satellite-uplink.png"
	"images/sensor.png"
	"images/sensor-tower.png"
	"images/transport.jpg"
	"images/unitordersmenu.jpg"
	"images/wss.png"
	"images/www.png"
	"ScriptingManual.htm"
	"docbook-xsl.css")

add_custom_command(
	TARGET doc_wz2100
	COMMAND "a2x"
	ARGS
		"-f"
		"xhtml"
		"-D"
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/quickstartguide.asciidoc"
	)

add_custom_command(
	TARGET doc_wz2100
	COMMAND "a2x"
	ARGS
		"-f"
		"xhtml"
		"-D"
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/warzone2100.6.asciidoc"
	)

if (${WKHTMLTOPDF_AVAILABLE})

add_custom_command(
	TARGET doc_wz2100
	COMMAND "wkhtmltopdf"
	ARGS
		"${CMAKE_CURRENT_SOURCE_DIR}/quickstartguide.htm"
		"${CMAKE_CURRENT_BINARY_DIR}/quickstartguide.pdf"
	)

endif()
