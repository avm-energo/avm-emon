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
# (-DAVM_EMON_LOCAL_DIR=...), else fetched from git.
find_package(libavm-emon QUIET)
if(libavm-emon_FOUND)
  message(STATUS "Found package libavm-emon")
else()
  set(AVM_EMON_LOCAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libavm-emon" CACHE PATH "Local libavm-emon checkout")
  if(EXISTS "${AVM_EMON_LOCAL_DIR}/CMakeLists.txt")
    message(STATUS "Using local libavm-emon: ${AVM_EMON_LOCAL_DIR}")
    add_subdirectory("${AVM_EMON_LOCAL_DIR}" libavm-emon EXCLUDE_FROM_ALL)
  else()
    include(emon)
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
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "electronics")
set(CPACK_RPM_PACKAGE_LICENSE "Proprietary")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_AUTOREQ ON)
include(CPack)
