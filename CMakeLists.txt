cmake_minimum_required(VERSION 3.21)
project(CarTerminal VERSION 0.1 LANGUAGES CXX)
if(MSVC AND CMAKE_CL_SHOWINCLUDES_PREFIX MATCHES "娉|鍖")
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "注意: 包含文件:  ")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Quick3D Svg Multimedia Test)
qt_standard_project_setup(REQUIRES 6.8)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(VEHICLE_SOURCES src/VehicleState.cpp src/VehicleMotion.cpp src/VehicleSimulation.cpp src/VehiclePreferences.cpp src/VehicleState.h)
qt_add_executable(CarTerminal src/main.cpp src/UiSmoke.cpp src/UiSmoke.h ${VEHICLE_SOURCES} src/IconProvider.h)
target_compile_options(CarTerminal PRIVATE /utf-8)
target_include_directories(CarTerminal PRIVATE src)
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(CarTerminal
    URI CarTerminal
    VERSION 1.0
    QML_FILES
        qml/SettingsPanel.qml qml/DemoPanel.qml qml/DrivingPanel.qml qml/DrivingPage.qml qml/DestinationPanel.qml qml/KeyboardPad.qml
        qml/Main.qml qml/DemoController.qml qml/Theme.qml qml/Label.qml qml/Icon.qml qml/Surface.qml
        qml/UiButton.qml qml/SoftSlider.qml qml/Album.qml qml/StatusBar.qml
        qml/BottomBar.qml qml/MusicCard.qml qml/HomePage.qml qml/MapView.qml
        qml/NavigationPage.qml qml/VehiclePage.qml qml/MusicPage.qml qml/Overlays.qml
        qml/Legacy3D.qml qml/ActionButton.qml assets/car/CarConcept.qml
        qml/CornerSelector.qml qml/WindowControls.qml qml/DoorControls.qml qml/QuickControls.qml qml/ChargeControls.qml qml/ComfortControls.qml
        qml/VehicleScene.qml qml/FeedbackBanner.qml qml/NotificationsPanel.qml assets/sedan/Sedan.qml
    SOURCES src/VehicleState.cpp src/VehicleState.h
)
file(GLOB_RECURSE UI_RESOURCES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui" "${CMAKE_CURRENT_SOURCE_DIR}/assets/ui/*")
list(TRANSFORM UI_RESOURCES PREPEND "assets/ui/")
qt_add_resources(CarTerminal ui_assets PREFIX "/ui" BASE "assets/ui" FILES ${UI_RESOURCES})
file(GLOB_RECURSE CAR_RESOURCES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/car/meshes/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/car/maps/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/sedan/meshes/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/sedan/maps/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/studio.hdr")
qt_add_resources(CarTerminal car_assets PREFIX "/qt/qml/CarTerminal" FILES ${CAR_RESOURCES})
target_link_libraries(CarTerminal PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Quick3D Qt6::Svg Qt6::Multimedia Qt6::Test)
set_target_properties(CarTerminal PROPERTIES WIN32_EXECUTABLE TRUE)
include(CTest)
if(BUILD_TESTING)
    qt_add_executable(VehicleStateTests tests/VehicleStateTests.cpp ${VEHICLE_SOURCES})
    target_compile_options(VehicleStateTests PRIVATE /utf-8)
    target_include_directories(VehicleStateTests PRIVATE src)
    target_link_libraries(VehicleStateTests PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
    add_test(NAME VehicleStateTests COMMAND VehicleStateTests)
    set_tests_properties(VehicleStateTests PROPERTIES ENVIRONMENT "PATH=D:/Qt/6.12.0/msvc2022_64/bin\;$ENV{PATH}")
endif()
