#Copyright (c) 2019 Kracejic

#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# Original file here: https://github.com/kracejic/cleanCppProject/blob/ba579d8fdbcc880cff45cb0b1b5ac165bf8727a8/packaging/CMakeLists.txt
# Modified to fit RScraper package specifics, and will likely continue to evolve/diverge.


#------------------------------------------------------------------------------
# CPACK
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_VENDOR "NotCompsky")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CONTACT "NotCompsky <adammarkgray@gmail.com>")
set(HOMEPAGE "https://github.com/NotCompsky/rscraper")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Family of resources written in C++ for scraping Reddit into a MySQL database.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${RSCRAPER_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${RSCRAPER_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${RSCRAPER_PATCH_VERSION})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/pkg")

#set(CPACK_PACKAGE_FILE_NAME DEB-DEFAULT) # Incompatible with default CMake version on Ubuntu 18.04
if("${CPACK_DEBIAN_PACKAGE_ARCHITECTURE} " STREQUAL " ")
	message(FATAL_ERROR "Please set -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=$(dpkg --print-architecture)")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")


set(CPACK_CREATE_DESKTOP_LINKS rscraper-hub)
set(CPACK_STRIP_FILES TRUE)

if(APPLE OR NOT UNIX)
	message(FATAL_ERROR "I haven't written the CMake to package on non-Linux systems yet")
else()
	#-----------------------------------------------------------------------------
	# Linux specific
	set(CPACK_GENERATOR "DEB")
	message(STATUS "Package generation - UNIX")
	message(STATUS "   + DEB	YES ")

	set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${HOMEPAGE}")

	# Icon and app shortcut for Linux systems
	# Note: .desktop file must have same name as executable
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/rscraper-hub.desktop
		DESTINATION share/applications/
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/rscraper-hub.png
		DESTINATION share/icons/hicolor/64x64/apps/
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/rscraper-hub.png
		DESTINATION share/icons/gnome/64x64/apps/
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		)
	# License file
	install(FILES ${PROJECT_SOURCE_DIR}/LICENSE
		DESTINATION "${CMAKE_INSTALL_DOCDIR}"
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		RENAME copyright)
	# set package icon
	set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}/tagger/browser-addon/icons/64.png")
endif()




#------------------------------------------------------------------------------
# and include CPack, so we get target for packages
include(CPack)
