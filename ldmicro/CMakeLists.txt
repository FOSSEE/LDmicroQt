cmake_minimum_required(VERSION 2.6)

project(LDMicro)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/includes/ldmicroVC.h" VersionRead)

string(REGEX MATCH "LDMicro_VERSION_MAJOR ([0-9]*)" LDMicro_VERSION_MAJOR ${VersionRead})
string(REGEX MATCH "LDMicro_VERSION_MINOR ([0-9]*)" LDMicro_VERSION_MINOR ${VersionRead})
string(REGEX MATCH "LDMicro_VERSION_PATCH ([0-9]*)" LDMicro_VERSION_PATCH ${VersionRead})

macro(use_cxx14)

if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
    endif ()
else ()
    set (CMAKE_CXX_STANDARD 14)
endif ()

endmacro(use_cxx14)

IF(WIN32)
    MESSAGE( FATAL_ERROR "Cannot build for windows, exiting" )
    add_definitions(-D__WIN32__)
ENDIF(WIN32)

IF (MSVC)
    MESSAGE( FATAL_ERROR "Cannot build for MacOS, exiting" )
    add_definitions(-D__MSVC__)
ENDIF (MSVC)

IF(UNIX)
    MESSAGE( STATUS "Initializing.." )
    add_definitions ("-w")
    add_definitions ("-g")
    add_definitions(-D__UNIX__)
    add_definitions(-DLDLANG_EN)

    execute_process(COMMAND lsb_release -cs
    OUTPUT_VARIABLE RELEASE_CODENAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(TOLOWER ${RELEASE_CODENAME} RELEASE_LOWER)
    message("-- os_version_suffix:${RELEASE_CODENAME}")
    if(${RELEASE_LOWER} MATCHES "trusty")
        MESSAGE("Add flag to compile for Ubuntu 14")
        add_definitions(-D__UBUNTU_14_)
    else()
        MESSAGE("Add flag to compile for Ubuntu 16 and above")
        add_definitions(-D__UBUNTU_16_)
    endif()
    use_cxx14()
    
    # set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/")

    MESSAGE( STATUS "Performing system check.." )
    MESSAGE( STATUS "Identifing bitness of the platform.." )
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-D__UNIX64)
        MESSAGE( STATUS "Bitness of the platform: " 64)
    else()
        add_definitions(-D__UNIX32)
        MESSAGE( STATUS "Bitness of the platform: " 36)
    endif()
    MESSAGE( STATUS "Performing system check - done" )

    ## Set object dir
    set(OBJDIR ${CMAKE_CURRENT_SOURCE_DIR}/obj)

    ## Set perl scripts to be run before build to generate files needed
    MESSAGE( STATUS "Adding perl scripts to build.." )

    add_custom_command( 
        OUTPUT ${OBJDIR}/lang-tables.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND perl lang-tables.pl > ${OBJDIR}/lang-tables.h
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lang-*.txt")
    
    add_custom_command( 
        OUTPUT ${OBJDIR}/helptext.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND perl txt2c.pl  > ${OBJDIR}/helptext.cpp
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/manual*.txt")

    set(SCRIPT_GENERATED_FILES ${OBJDIR}/helptext.cpp
                            ${OBJDIR}/lang-tables.h)

    add_custom_target(LDMicro_SCRIPT_GENERATED_FILES DEPENDS ${SCRIPT_GENERATED_FILES})

    ## configure a header file to pass some of the CMake settings
    ## to the source code
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/linuxUI")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/freezeLD")
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/includes")
    # include_directories("${OBJDIR}")
    set(PROJECT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/includes")
    CONFIGURE_FILE (
        "${PROJECT_INCLUDE_DIR}/ldmicroVC.h.in"
        "${PROJECT_INCLUDE_DIR}/ldmicroVC.h"
    )
    # MESSAGE ( STATUS " PROJECT_INCLUDE_DIR: " ${PROJECT_INCLUDE_DIR} )

    set_property(SOURCE ${OBJDIR}/helptext.cpp PROPERTY SKIP_AUTOGEN ON)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    find_package(Qt5Widgets CONFIG REQUIRED)
    find_package(Qt5Core CONFIG REQUIRED)
    find_package(Qt5Gui CONFIG REQUIRED)
    get_target_property(Qt5Widgets_INCLUDES Qt5::Widgets INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(Qt5Core_INCLUDES Qt5::Core INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(Qt5Gui_INCLUDES Qt5::Gui INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${Qt5Widgets_INCLUDES})
    include_directories(${Qt5Core_INCLUDES})
    include_directories(${Qt5Gui_INCLUDES})
    add_definitions(${Qt5Widgets_DEFINITIONS})
    add_definitions(${Qt5Core_DEFINITIONS})
    include_directories(${Qt5Gui_DEFINITIONS})
    set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
    #get_target_property(CMAKE_CXX_FLAGS Qt5::Core LINK_FLAGS)
    MESSAGE ( STATUS " Qt INCLUDEs: " ${Qt5Core_INCLUDES} )

    MESSAGE ( STATUS " Qt CXX FLAGS: " ${CMAKE_CXX_FLAGS} )

        if (Qt5_POSITION_INDEPENDENT_CODE)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib/linuxUI")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib/freezeLD")
    
    ## Dummy compile and install to test linuxUI
    ## to compile LDmicro uncomment the below 2 line2
    set (COMPILE_CPP_SOURCES arduino.cpp
                            avr.cpp
                            pic16.cpp
                            interpreted.cpp
                            ansic.cpp
                            compilecommon.cpp
                            intcode.cpp
                            lang.cpp
                            miscutil.cpp
                            iolist.cpp
                            confdialog.cpp
                            lutdialog.cpp
                            resetdialog.cpp
                            simpledialog.cpp
                            coildialog.cpp
                            contactsdialog.cpp
                            commentdialog.cpp
                            simulate.cpp
                            loadsave.cpp
                            undoredo.cpp
                            circuit.cpp
                            draw_outputdev.cpp
                            draw.cpp
                            schematic.cpp
                            helpdialog.cpp
                            toolbar.cpp
                            ${OBJDIR}/helptext.cpp
                            maincontrols.cpp
                            ldmicro.cpp)
    
    ## Build *.int interpreter
    add_executable (ldinterpret ldinterpret.c)
    target_link_libraries (ldinterpret LinuxUI)
    
    ## Build main LDMicro executable
    add_executable (LDMicro ${COMPILE_CPP_SOURCES} ldmicro.qrc)
    add_dependencies(LDMicro LDMicro_SCRIPT_GENERATED_FILES)
    target_link_libraries (LDMicro LinuxUI)
    target_link_libraries (LDMicro FreezeLD)
    target_link_libraries(LDMicro ${Qt5Widgets_LIBRARIES})
    target_link_libraries(LDMicro ${Qt5Core_LIBRARIES})
    target_link_libraries(LDMicro ${Qt5Gui_LIBRARIES})

    # Package Creation

    if( NOT DEFAULT_INSTALL_PATH )
    set( DEFAULT_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}"
         CACHE
         PATH
         "Location of LDMicro data files." )
    endif()

    # Set paths
    set( UNIX_ICON_FILES icons/ )
    set( UNIX_APPLICATIONS_FILES applications/ )
    set( LDMICRO_BIN bin CACHE PATH "LDmicro binaries.")

    install( DIRECTORY ${UNIX_ICON_FILES}
        DESTINATION share/icons
        COMPONENT resources
        )

    install( DIRECTORY ${UNIX_APPLICATIONS_FILES}
        DESTINATION share/applications
        COMPONENT resources
        )
    install( TARGETS LDMicro
    DESTINATION bin
    )
    ##Debian package creation
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_VERSION_MAJOR ${LDMicro_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${LDMicro_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${LDMicro_VERSION_PATCH})
    
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_PACKAGE_DESCRIPTION "Ladder Logic editor, simulator and compiler for 8 bit microcontrollers")
    set(CPACK_PACKAGE_CONTACT "Akshay Chipkar akshaychipkar@yahoo.co.in")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst")

    include(CPack)
    
    ## Add tests
    MESSAGE( STATUS "Adding tests.." )
    add_custom_command(
        TARGET LDMicro
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/reg
        COMMAND perl run-tests.pl)
    
ENDIF(UNIX)
