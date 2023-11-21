project(LinuxUI)

add_library(LinuxUI linuxUI.cpp linuxLD.cpp)
target_link_libraries(LinuxUI PUBLIC
    Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)
