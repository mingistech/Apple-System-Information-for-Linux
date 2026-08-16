cmake_minimum_required(VERSION 3.16)

project(SpxViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(spx-viewer
    src/main.cpp
    src/parser/PlistValue.cpp
    src/parser/PlistValue.h
    src/parser/SpxParser.cpp
    src/parser/SpxParser.h
    src/model/LabelHumanizer.cpp
    src/model/LabelHumanizer.h
    src/model/SystemReport.cpp
    src/model/SystemReport.h
    src/ui/DetailView.cpp
    src/ui/DetailView.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/ReportTreeModel.cpp
    src/ui/ReportTreeModel.h
)

target_include_directories(spx-viewer PRIVATE src)

target_link_libraries(spx-viewer PRIVATE Qt6::Widgets)

set_target_properties(spx-viewer PROPERTIES
    OUTPUT_NAME spx-viewer
)

install(TARGETS spx-viewer RUNTIME DESTINATION bin)
install(FILES data/spx-viewer.desktop DESTINATION share/applications)
install(FILES data/spx-viewer.xml DESTINATION share/mime/packages)
