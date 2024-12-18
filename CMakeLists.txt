CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT("prometheus" VERSION 1.0.1)

# RPM/DEB and CENTOS8/ROCKY9/UBU2004/UBU2204/UBU2404
IF (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/OSRELEASE.txt")
	FILE(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/OSRELEASE.txt OSRELEASE)
ELSEIF (EXISTS "/OSRELEASE.txt")
	FILE(STRINGS /OSRELEASE.txt OSRELEASE)
ELSE()
	MESSAGE(FATAL_ERROR "Missing OSRELEASE")
ENDIF()

IF (OSRELEASE MATCHES "CENTOS8")
	SET(CPACK_GENERATOR "RPM")
ELSEIF (OSRELEASE MATCHES "ROCKY9")
	SET(CPACK_GENERATOR "RPM")
ELSEIF (OSRELEASE MATCHES "UBU2004")
	SET(CPACK_GENERATOR "DEB")
ELSEIF (OSRELEASE MATCHES "UBU2204")
	SET(CPACK_GENERATOR "DEB")
ELSEIF (OSRELEASE MATCHES "UBU2404")
	SET(CPACK_GENERATOR "DEB")
ELSE()
	MESSAGE(FATAL_ERROR "Invalid OSRELEASE")
ENDIF()

SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++17 -Wall -Wvla -Wshadow -Wconversion -Wno-sign-conversion -Wno-c++11-narrowing -O2 -fno-strict-aliasing -Wextra -Wno-unused-parameter")

ADD_LIBRARY(prometheus SHARED
	prometheus.cpp
	configuration.cpp
)

TARGET_LINK_LIBRARIES(prometheus
	microhttpd
)

SET_TARGET_PROPERTIES(prometheus PROPERTIES PREFIX "")

INCLUDE_DIRECTORIES(
	/opt/halon/include
)

IF(EXISTS "/etc/os-release")
	EXECUTE_PROCESS(COMMAND "sed" "-ne" "s/^ID=\"\\?\\([a-z]\\+\\)\"\\?$/\\1/p" "/etc/os-release" OUTPUT_VARIABLE OS_RELEASE_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
	EXECUTE_PROCESS(COMMAND "sed" "-ne" "s/^VERSION_ID=\"\\?\\([0-9\\.]\\+\\)\"\\?$/\\1/p" "/etc/os-release" OUTPUT_VARIABLE OS_RELEASE_VERSION_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
	EXECUTE_PROCESS(COMMAND "uname" "-m" OUTPUT_VARIABLE OS_RELEASE_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
	SET(SYSTEM_NAME "${OS_RELEASE_ID}-${OS_RELEASE_VERSION_ID}-${OS_RELEASE_ARCH}")
ENDIF()

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/prometheus.so DESTINATION plugins)
INSTALL(FILES prometheus.schema.json DESTINATION share/plugins)

IF(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0") 
	FILE(CREATE_LINK "/opt/halon/plugins/prometheus.so" "${CMAKE_CURRENT_BINARY_DIR}/prometheus.so.link" SYMBOLIC)
ELSE()
	ADD_CUSTOM_TARGET(link_target ALL COMMAND ${CMAKE_COMMAND} -E create_symlink "/opt/halon/plugins/prometheus.so" "${CMAKE_CURRENT_BINARY_DIR}/prometheus.so.link")
ENDIF()

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/prometheus.so.link" DESTINATION plugins/autoload/smtpd RENAME prometheus.so)

INSTALL(FILES "LICENSE" DESTINATION share/doc/plugins/prometheus)

SET(CMAKE_INSTALL_PREFIX "/opt/halon")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/opt/halon")

IF (CPACK_GENERATOR MATCHES "DEB")
	SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "halon (>= 6.3)")
	SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst;${CMAKE_CURRENT_SOURCE_DIR}/postrm")
	IF (OSRELEASE MATCHES "UBU2404")
		SET(CPACK_DEBIAN_PACKAGE_DEPENDS "halon (>= 6.3), libmicrohttpd12t64")
	ELSE()
		SET(CPACK_DEBIAN_PACKAGE_DEPENDS "halon (>= 6.3)")
	ENDIF()
ENDIF()
IF (CPACK_GENERATOR MATCHES "RPM")
	SET(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
	SET(CPACK_RPM_PACKAGE_REQUIRES "halon >= 6.3, libmicrohttpd")
	SET(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
	SET(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/postrm")
ENDIF()

SET(CPACK_PACKAGE_NAME "halon-extras-prometheus")
SET(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Halon extras prometheus plugin")
SET(CPACK_PACKAGE_DESCRIPTION "Halon extras prometheus plugin")
SET(CPACK_PACKAGE_CONTACT "Halon support@halon.io")
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${SYSTEM_NAME}")

SET(CPACK_DEBIAN_PACKAGE_SECTION "mail")

INCLUDE(CPack)
