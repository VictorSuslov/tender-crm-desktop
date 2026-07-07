cmake_minimum_required(VERSION 3.16)
project(TenderCrmDesktop VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(SOURCES
    src/main.cpp
    src/api/ApiClient.cpp
    src/models/TendersModel.cpp
    src/models/EmailsModel.cpp
    src/views/MainWindow.cpp
    src/views/TendersView.cpp
    src/views/EmailsView.cpp
    src/views/SettingsView.cpp
    src/dialogs/TenderEditDialog.cpp
    src/dialogs/LinkEmailDialog.cpp
    src/dialogs/EmailDetailDialog.cpp
	src/views/RagView.cpp
	src/widgets/EmailTextBrowser.cpp
)

set(HEADERS
    src/api/ApiClient.h
    src/api/DtoModels.h
    src/models/TendersModel.h
    src/models/EmailsModel.h
    src/views/MainWindow.h
    src/views/TendersView.h
    src/views/EmailsView.h
    src/views/SettingsView.h
    src/dialogs/TenderEditDialog.h
    src/dialogs/LinkEmailDialog.h
    src/dialogs/EmailDetailDialog.h
	src/views/RagView.h
	src/widgets/EmailTextBrowser.h
)

add_executable(TenderCrmDesktop ${SOURCES} ${HEADERS})

target_link_libraries(TenderCrmDesktop PRIVATE
    Qt6::Widgets
    Qt6::Network
)

target_include_directories(TenderCrmDesktop PRIVATE src)