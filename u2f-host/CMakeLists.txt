include(CheckIncludeFiles)
include(CheckFunctionExists) 

# ==========
# Check for stdint.h presence
# ==========
check_include_files(stdint.h HAVE_STDINT_H)

# ==========
# Configure version
# ==========
set(U2FH_VERSION_MAJOR 1)
set(U2FH_VERSION_MINOR 1)
set(U2FH_VERSION_PATCH 1)
set(U2FH_VERSION_NUMBER "0x010101")
set(VERSION "1.1.1")
set(LT_CURRENT 1)
set(LT_AGE, 1)
set(LT_REVISION, 1)
configure_file(u2f-host-version.h.in u2f-host-version.h)

# ==========
# Other configurations
# ==========
configure_file(config.h.cmake.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ==========
# Source files
# ==========
set(SOURCE authenticate.c  cdecode.c  cencode.c  devs.c  error.c  global.c  register.c  u2fmisc.c  version.c)
source_group(sources FILES ${SOURCE})
include_directories(.)
set(HEADERS u2f-host.h  u2f-host-types.h  internal.h)
source_group(headers FILES ${HEADERS})
set(HEADERS_B64 b64/cdecode.h  b64/cencode.h)
source_group(headers\\b64 FILES ${HEADERS_B64})
set(HEADERS_INC inc/u2f.h  inc/u2f_hid.h)
source_group(headers\\inc FILES ${HEADERS_INC})

# ==========
# Add support gnulib files
# ==========
set(GL_DIR ${CMAKE_SOURCE_DIR}/gl)
set(SOURCE_GL ${GL_DIR}/sha256.c ${GL_DIR}/check-version.c ${GL_DIR}/strverscmp.c)
set_source_files_properties(${GL_DIR}/check-version.c PROPERTIES COMPILE_DEFINITIONS VERSION="${VERSION}") 
source_group(sources\\gl FILES ${SOURCE_GL})
include_directories(${GL_DIR})
set(HEADERS_GL ${GL_DIR}/sha256.h ${GL_DIR}/check-version.h)
source_group(headers\\gl FILES ${HEADERS_GL})
configure_file(${GL_DIR}/stdalign.in.h stdalign.h)

# ==========
# Import hidapi
# ==========
add_library(hidapi SHARED IMPORTED)
set(HIDAPI_DIR hidapi CACHE PATH "Path to hidapi library")
set(HIDAPI_BIN_DIR ${HIDAPI_DIR}/windows)

set_target_properties(hidapi PROPERTIES
	IMPORTED_LOCATION_DEBUG ${HIDAPI_BIN_DIR}/Debug/hidapi.dll
	IMPORTED_IMPLIB_DEBUG ${HIDAPI_BIN_DIR}/Debug/hidapi.lib
	IMPORTED_LOCATION_RELEASE ${HIDAPI_BIN_DIR}/Release/hidapi.dll
	IMPORTED_IMPLIB_RELEASE ${HIDAPI_BIN_DIR}/Release/hidapi.lib
	INTERFACE_INCLUDE_DIRECTORIES ${HIDAPI_DIR}/hidapi)

# ==========
# Import json-c
# ==========
add_library(json-c SHARED IMPORTED)
set(JSON-C_DIR json-c CACHE PATH "Path to json-c library")
set(JSON-C_BIN_DIR ${JSON-C_DIR})

set_target_properties(json-c PROPERTIES
	IMPORTED_LOCATION_DEBUG ${JSON-C_BIN_DIR}/Debug/json-c.dll
	IMPORTED_IMPLIB_DEBUG ${JSON-C_BIN_DIR}/Debug/json-c.lib
	IMPORTED_LOCATION_RELEASE ${JSON-C_BIN_DIR}/Release/json-c.dll
	IMPORTED_IMPLIB_RELEASE ${JSON-C_BIN_DIR}/Release/json-c.lib
	INTERFACE_INCLUDE_DIRECTORIES ${JSON-C_DIR})

# ==========
# Build as library
# ==========
add_library(u2f-host SHARED ${SOURCE} ${SOURCE_GL} ${HEADERS} ${HEADERS_B64} ${HEADERS_INC} ${HEADERS_STDINT} ${HEADERS_GL})
if(WIN32)
	set_target_properties(u2f-host PROPERTIES OUTPUT_NAME "libu2f-host-${LT_CURRENT}" VERSION ${VERSION} )
else()
	set_target_properties(u2f-host PROPERTIES OUTPUT_NAME "libu2f-host" VERSION "${LT_CURRENT}.${LT_AGE}.${LT_REVISION}" SOVERSION ${LT_CURRENT} )
endif()
target_link_libraries(u2f-host hidapi json-c)
