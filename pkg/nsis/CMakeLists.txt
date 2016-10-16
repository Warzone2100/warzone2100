project(installer)

add_custom_target(nsis_wz2100)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})

set(INSTALLER_VERSION "0.0.0.0" CACHE STRING "NSIS version")
set(INSTALLER_EXTDIR "" CACHE STRING "NSIS ext dir")
set(INSTALLER_COMPRESSION 1 CACHE STRING "NSIS compression level")

if(${WZ_PORTABLE})
if (${VIDEOS})
	add_custom_command(
		TARGET nsis_wz2100
		COMMAND "i686-w64-mingw32.static-makensis"
		ARGS
			"-DTOP_SRCDIR=\"${CMAKE_SOURCE_DIR}\""
			"-DPORTABLE"
			"-DINSTALLVIDS"
			"-DPACKAGE=\"warzone2100\""
			"-DPACKAGE_NAME=\"warzone2100\""
			"-DTOP_BUILDDIR=\"${CMAKE_BINARY_DIR}\""
			"-DPACKAGE_VERSION=\"3.2\""
			"-DVERSIONNUM=\"${INSTALLER_VERSION}\""
			"-DOUTFILE=\"warzone2100_portable.exe\""
			"-DEXTDIR=\"${INSTALLER_EXTDIR}\""
			#"-X\"SetCompressor /FINAL /SOLID ${INSTALLER_COMPRESSION}\""
			"${CMAKE_CURRENT_SOURCE_DIR}/warzone2100.nsi"
			# -DPACKAGE_BUGREPORT="$(PACKAGE_BUGREPORT)" 
	)
else()
	add_custom_command(
		TARGET nsis_wz2100
		COMMAND "i686-w64-mingw32.static-makensis"
		ARGS
			"-DTOP_SRCDIR=\"${CMAKE_SOURCE_DIR}\""
			"-DPORTABLE"
			"-DPACKAGE=\"warzone2100\""
			"-DPACKAGE_NAME=\"warzone2100\""
			"-DTOP_BUILDDIR=\"${CMAKE_BINARY_DIR}\""
			"-DPACKAGE_VERSION=\"3.2\""
			"-DVERSIONNUM=\"${INSTALLER_VERSION}\""
			"-DOUTFILE=\"warzone2100_portable.exe\""
			"-DEXTDIR=\"${INSTALLER_EXTDIR}\""
			#"-X\"SetCompressor /FINAL /SOLID ${INSTALLER_COMPRESSION}\""
			"${CMAKE_CURRENT_SOURCE_DIR}/warzone2100.nsi"
			# -DPACKAGE_BUGREPORT="$(PACKAGE_BUGREPORT)"
	)
endif()
else()
	add_custom_command(
		TARGET nsis_wz2100
		COMMAND "i686-w64-mingw32.static-makensis"
		ARGS
			"-DTOP_SRCDIR=\"${CMAKE_SOURCE_DIR}\""
			"-DPACKAGE=\"warzone2100\""
			"-DPACKAGE_NAME=\"warzone2100\""
			"-DTOP_BUILDDIR=\"${CMAKE_BINARY_DIR}\""
			"-DPACKAGE_VERSION=\"3.2\""
			"-DVERSIONNUM=\"${INSTALLER_VERSION}\""
			"-DOUTFILE=\"warzone2100.exe\""
			"-DEXTDIR=\"${INSTALLER_EXTDIR}\""
			#"-X\"SetCompressor /FINAL /SOLID ${INSTALLER_COMPRESSION}\""
			"${CMAKE_CURRENT_SOURCE_DIR}/warzone2100.nsi"
			# -DPACKAGE_BUGREPORT="$(PACKAGE_BUGREPORT)"
	)
endif()

add_dependencies(nsis_wz2100 data_base data_mp warzone2100 doc_wz2100)

#$(UPDATERFILE): warzone2100.update.nsi
#	$(MAKENSIS) -DVERSION="$(VERSION)" -DVERSIONNUM="$(INSTALLER_VERSION)" -DEXTDIR="$(INSTALLER_EXTDIR)" -X"SetCompressor /FINAL /SOLID $(INSTALLER_COMPRESSION)" $<

endif()
