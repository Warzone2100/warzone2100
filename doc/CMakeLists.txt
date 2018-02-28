project(doc)

add_custom_target(doc_wz2100 ALL)

#######################
# Scripting docs

add_custom_command(
	TARGET doc_wz2100
	COMMAND ${CMAKE_COMMAND} -DOUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/build_tools/GenerateJSDocs.cmake"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

configure_file(Scripting.md ${CMAKE_CURRENT_BINARY_DIR}/Scripting.md COPYONLY)

install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/js-globals.md"
		"${CMAKE_CURRENT_BINARY_DIR}/js-events.md"
		"${CMAKE_CURRENT_BINARY_DIR}/js-functions.md"
		"${CMAKE_CURRENT_BINARY_DIR}/js-objects.md"
		"${CMAKE_CURRENT_BINARY_DIR}/js-campaign.md"
		"${CMAKE_CURRENT_BINARY_DIR}/Scripting.md"
		DESTINATION "doc/" COMPONENT Core
)

#######################
# Main documentation

find_package(A2X)
if(NOT A2X_FOUND)
	message( WARNING "a2x is not found. Documentation cannot be generated if asciidoc is not installed. (Skipping documentation generation.)" )
	return()
endif()

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
	COMMAND ${A2X_COMMAND}
	ARGS
		"-f"
		"xhtml"
		"-D"
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/quickstartguide.asciidoc"
	)

add_custom_command(
	TARGET doc_wz2100
	COMMAND ${A2X_COMMAND}
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

install(FILES ${doc_DATA} DESTINATION "doc/images/" COMPONENT Core)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/docbook-xsl.css" DESTINATION "doc/" COMPONENT Core)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/quickstartguide.html" DESTINATION "doc/" COMPONENT Core)
