cmake_minimum_required(VERSION 3.21)

project(avm-emon
    VERSION 0.1.0
    DESCRIPTION "GUI for reading the EM-3.1KM (EMon) power-quality meter"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core SerialPort Widgets)
qt_standard_project_setup()

# libavm-emon: installed package, else a local checkout next to this repo
# (-DAVM_EMON_LOCAL_DIR=...), else fetched from git. A package is used as it was
# built (shared for the DEB/RPM); the in-tree copy is built as a shared library
# by default (-DAVM_EMON_SHARED_LIB=OFF links it statically) and shipped with the app.
option(AVM_EMON_SHARED_LIB "Build the in-tree libavm-emon as a shared library and install it with the app" ON)
find_package(libavm-emon QUIET)
if(libavm-emon_FOUND)
  message(STATUS "Found package libavm-emon")
  # The installed package exports libavm-emon::libavm-emon; the rest of this file links the plain name.
  if(NOT TARGET libavm-emon AND TARGET libavm-emon::libavm-emon)
    add_library(libavm-emon ALIAS libavm-emon::libavm-emon)
  endif()
else()
  if(DEFINED BUILD_SHARED_LIBS)
    set(_avm_saved_shared "${BUILD_SHARED_LIBS}")
  endif()
  if(AVM_EMON_SHARED_LIB)
    set(BUILD_SHARED_LIBS ON)
  endif()
  set(AVM_EMON_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libavm-emon" CACHE PATH "Local libavm-emon checkout")
  if(EXISTS "${AVM_EMON_LOCAL_DIR}/CMakeLists.txt")
    message(STATUS "Using local libavm-emon: ${AVM_EMON_LOCAL_DIR}")
    add_subdirectory("${AVM_EMON_LOCAL_DIR}" libavm-emon EXCLUDE_FROM_ALL)
  else()
    include(emon)
  endif()
  # Do not let the library's setting leak into libavm-vectorwidget below.
  if(DEFINED _avm_saved_shared)
    set(BUILD_SHARED_LIBS "${_avm_saved_shared}")
  else()
    unset(BUILD_SHARED_LIBS)
  endif()
endif()

# libavm-vectorwidget (vector diagram): same lookup order.
find_package(libavm-vectorwidget QUIET)
if(libavm-vectorwidget_FOUND)
  message(STATUS "Found package libavm-vectorwidget")
else()
  set(AVM_VECTORWIDGET_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libavm-vectorwidget" CACHE PATH "Local libavm-vectorwidget checkout")
  if(EXISTS "${AVM_VECTORWIDGET_LOCAL_DIR}/CMakeLists.txt")
    message(STATUS "Using local libavm-vectorwidget: ${AVM_VECTORWIDGET_LOCAL_DIR}")
    add_subdirectory("${AVM_VECTORWIDGET_LOCAL_DIR}" libavm-vectorwidget EXCLUDE_FROM_ALL)
  else()
    include(vectorwidget)
  endif()
endif()

qt_add_executable(avm-emon
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/EmonWorker.h
    src/EmonWorker.cpp
)

target_link_libraries(avm-emon PRIVATE Qt6::Core Qt6::SerialPort Qt6::Widgets libavm-emon libavm-vectorwidget)

# --- Install & packaging (CPack: DEB / RPM on Linux; the Windows installer is built by packaging/windows) ---
include(GNUInstallDirs)
install(TARGETS avm-emon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# The in-tree libavm-emon, when built shared, travels with the app (.so / .dll; no
# headers or dev symlink - those belong to the libavm-emon-dev(el) package).
if(TARGET libavm-emon)
  get_target_property(_emon_type libavm-emon TYPE)
  get_target_property(_emon_imported libavm-emon IMPORTED)
  if(_emon_type STREQUAL "SHARED_LIBRARY" AND NOT _emon_imported)
    install(TARGETS libavm-emon
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()
set_target_properties(avm-emon PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
if(UNIX AND NOT APPLE)
  install(FILES packaging/linux/avm-emon.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

# CI passes -DAVM_EMON_VERSION=<tag or tag-less build version>; defaults to the project version.
set(AVM_EMON_VERSION "${PROJECT_VERSION}" CACHE STRING "Version stamped on the packages")
set(CPACK_PACKAGE_NAME "avm-emon")
set(CPACK_PACKAGE_VERSION "${AVM_EMON_VERSION}")
set(CPACK_PACKAGE_VENDOR "AVM-Energo LLC")
set(CPACK_PACKAGE_CONTACT "AVM-Energo LLC <info@avmenergo.ru>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GUI client for the EM-3.1KM (EMon) power-quality meter")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://git.avmenergo.ru/avm-energo/avm-emon")
set(CPACK_PACKAGE_FILE_NAME "avm-emon-${AVM_EMON_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# dpkg-shlibdeps must find the libavm-emon.so.N that this very package ships
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
set(CPACK_RPM_PACKAGE_LICENSE "Proprietary")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
include(CPack)
