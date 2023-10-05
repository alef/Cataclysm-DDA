# TODO after CMake v3.22: guess Linux distro via host info command
if(EXISTS /etc/os-release)
    file(STRINGS /etc/os-release OS_ID_LIKE REGEX ^ID_LIKE=.+$)
    string(REGEX REPLACE ^ID_LIKE= "" OS_ID_LIKE ${OS_ID_LIKE})
endif()
set(CPACK_GENERATOR ZIP) 
set(CPACK_PACKAGE_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR})

include(CPack)
