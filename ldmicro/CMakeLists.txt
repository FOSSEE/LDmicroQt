cmake_minimum_required(VERSION 2.6)

project(LDMicro)

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

    MESSAGE ( STATUS "Searching for GTK+3.0..." )
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (GTK3 REQUIRED gtk+-3.0)

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

    ## Version control
    set (LDMicro_VERSION_MAJOR 2)
    set (LDMicro_VERSION_MINOR 2)

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

    ## Add GTK3 include files if GTK3 is found
    IF ( GTK3_FOUND )
        include_directories (${GTK3_INCLUDE_DIRS})
        link_directories (${GTK3_LIBRARY_DIRS})
        add_definitions (${GTK3_CFLAGS_OTHER})
        link_libraries (${GTK3_LIBRARIES})
        # MESSAGE( STATUS " GTK3_INCLUDE_DIR: " ${GTK3_INCLUDE_DIRS} )
        # MESSAGE( STATUS " GTK3_LIBRARIES: " ${GTK3_LIBRARIES} )
    ENDIF ( GTK3_FOUND )

    set_property(SOURCE ${OBJDIR}/helptext.cpp PROPERTY SKIP_AUTOGEN ON)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
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
                            miscutil.cpp    #add message boxes to Error(), CompileSuccessfulMessage()
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
                            ${OBJDIR}/helptext.cpp
                            maincontrols.cpp
                            ldmicro.cpp)
    
    ## Build *.int interpreter
    add_executable (ldinterpret ldinterpret.c)
    target_link_libraries (ldinterpret LinuxUI)
    
    ## Build main LDMicro executable
    add_executable (LDMicro ${COMPILE_CPP_SOURCES})
    add_dependencies(LDMicro LDMicro_SCRIPT_GENERATED_FILES)
    install (TARGETS LDMicro DESTINATION bin)
    target_link_libraries (LDMicro LinuxUI)
    target_link_libraries (LDMicro FreezeLD)
    target_link_libraries(LDMicro ${Qt5Widgets_LIBRARIES})
    target_link_libraries(LDMicro ${Qt5Core_LIBRARIES})
    target_link_libraries(LDMicro ${Qt5Gui_LIBRARIES})

    INSTALL(TARGETS LDMicro DESTINATION /usr/local/bin/ldmicro)
    INSTALL(FILES ldmicro.ico DESTINATION /usr/local/bin/ldmicro)
    INSTALL(FILES LDMicro.desktop DESTINATION /usr/share/applications)

    ##Debian package creation
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_VERSION_MAJOR "2")
    set(CPACK_PACKAGE_VERSION_MINOR "2")
    set(CPACK_PACKAGE_VERSION_PATCH "4")
    
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
