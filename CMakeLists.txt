CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(CCMIX)
SET(CCMIX_MAJOR "0")
SET(CCMIX_MINOR "1")
SET(CCMIX_PATCH "0")
SET(VERSION "0.1.0")

SET(CMAKE_MODULE_PATH ${CCMIX_SOURCE_DIR}/modules)

IF( NOT DEFINED CMAKE_INSTALL_PREFIX )
  SET(CMAKE_INSTALL_PREFIX /usr/local)
  SET(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
  SET(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_PREFIX}/share/ccmix)
  SET(EASTWOOD_ROOTINSTALL_DIR ${CMAKE_INSTALL_PREFIX})
ENDIF( NOT DEFINED CMAKE_INSTALL_PREFIX )

FIND_PACKAGE(Crypto++ REQUIRED)
IF ( NOT CRYPTOPP_FOUND)
  MESSAGE( FATAL_ERROR "libcrypto++ not found" )
ELSE ( NOT CRYPTOPP_FOUND)
  INCLUDE_DIRECTORIES(${CRYPTOPP_INCLUDE_DIRS})
ENDIF( NOT CRYPTOPP_FOUND)

#make binary relocatable if we used shared libs
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")

SET(PROJECT_SRC_DIR "src/")

IF(NOT MSVC)
  SET(WARN_FLAGS "-Wall -Wno-long-long -Wno-variadic-macros")
  SET(WARN_FLAGS_DEBUG "-Wextra -Weffc++ -pedantic -Wold-style-cast -Woverloaded-virtual -Wctor-dtor-privacy -Wreorder -Wabi -Wstrict-null-sentinel -Wc++0x-compat -Wformat=2 -Winit-self -Wswitch-default -Wsign-conversion -Wlogical-op -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Winline")
  # Don't enable these for now, but leave them around to investigate closer later
  SET(WARN_FLAGS_DEBUG_EXCESSIVE "-Wpadded -Wunreachable-code -Wconversion")
  SET(CMAKE_CXX_FLAGS "${WARN_FLAGS} -fPIC ${CMAKE_CXX_FLAGS}")
  SET(CMAKE_CXX_FLAGS_DEBUG "${WARN_FLAGS_DEBUG} -g -O0 ${CMAKE_CXX_FLAGS_DEBUG}")
ENDIF(NOT MSVC)

SET(SHARED_SRC
    ${PROJECT_SRC_DIR}/mix_db_gmd.cpp
    ${PROJECT_SRC_DIR}/mix_db_gamedb.cpp
    ${PROJECT_SRC_DIR}/mixid.cpp
   )

SET(CCMIX_SRC
    ${PROJECT_SRC_DIR}/ccmix.cpp
    ${PROJECT_SRC_DIR}/mix_db_lmd.cpp
    ${PROJECT_SRC_DIR}/mix_file.cpp
    ${PROJECT_SRC_DIR}/mix_header.cpp
    ${PROJECT_SRC_DIR}/sha1.cpp
   )

SET(GMDEDIT_SRC
    ${PROJECT_SRC_DIR}/gmdedit/gmdedit.cpp
    
   )

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CCMIX_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CCMIX_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CCMIX_BINARY_DIR}/bin)

ADD_EXECUTABLE(ccmix ${CCMIX_SRC} ${SHARED_SRC})
TARGET_LINK_LIBRARIES(ccmix ${CRYPTOPP_LIBRARIES})

ADD_EXECUTABLE(gmdedit ${GMDEDIT_SRC} ${SHARED_SRC})
TARGET_LINK_LIBRARIES(gmdedit)