#------------------------------------------------- #
#                                                  #
# Project created by QtCreator 2019-01-29T14:55:08 #
#                                                  #
#------------------------------------------------- #

# for qt charts problem
# in ubuntu "sudo apt install libqt5charts5 libqt5charts5-de"
QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TelMon
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


HEADERS +=                                          \
    # controllers
    controllers/MainWindowController.h              \
    controllers/TreeMenuWidgetController.h          \
    controllers/DockWindowController.h              \
    controllers/TableWidgetDockWindowController.h   \
    controllers/BasicController.h                   \
    # models
    ## raw
    models/raw/MainWindowModel.h                   \
    models/raw/TreeModel.h                         \
    models/raw/TreeItem.h                          \
    ## wrappers
    models/wrappers/TreeMenuWidgetModelWrapper.h   \
    models/wrappers/TempModelWrapper.h             \
    models/wrappers/TempTable.h                    \
    models/wrappers/TempDockWidget.h               \
    # views
    ## raw
    views/raw/MainWindowView.h                    \
    views/raw/TreeView.h                          \
    ## wrappers
    # factories
    factories/ControllerFactory.h                 \
    factories/NetworkManagerFactory.h             \
    factories/DataDistributorFactory.h            \
    factories/TableSubscriberFactory.h            \
    factories/TableWidgetFactory.h                \
    # network_component
    network_component/NetworkManager.h                    \
    network_component/NetworkManagerQThreadImpl.h         \
    network_component/ConnectionQThread.h                 \
    network_component/MxTelMonClient.h                    \
    # distributor
    distributors/DataDistributor.h                   \
    distributors/DataDistributorQThreadImpl.h        \
    # subscribers
    subscribers/DataSubscriber.h                    \
    subscribers/TempTableSubscriber.h

SOURCES += \
    main.cpp \
    #controllers
    controllers/MainWindowController.cpp \
    controllers/BasicController.cpp \
    controllers/TreeMenuWidgetController.cpp \
    controllers/DockWindowController.cpp \
    controllers/TableWidgetDockWindowController.cpp \
    # models
    ## raw
    models/raw/MainWindowModel.cpp \
    models/raw/TreeModel.cpp \
    models/raw/TreeItem.cpp \
    ## wrappers
    models/wrappers/TreeMenuWidgetModelWrapper.cpp \
    models/wrappers/TempModelWrapper.cpp \
    models/wrappers/TempTable.cpp \
    models/wrappers/TempDockWidget.cpp \
    # views
    ## raw
    views/raw/MainWindowView.cpp \
    views/raw/TreeView.cpp \
    ## wrappers
    # factories
    factories/ControllerFactory.cpp \
    factories/DataDistributorFactory.cpp \
    factories/NetworkManagerFactory.cpp \
    factories/TableSubscriberFactory.cpp \
    factories/TableWidgetFactory.cpp  \
    # network_component
    network_component/NetworkManager.cpp \
    network_component/NetworkManagerQThreadImpl.cpp \
    network_component/ConnectionQThread.cpp \
    network_component/MxTelMonClient.cpp \
    # distributor
    distributors/DataDistributor.cpp \
    distributors/DataDistributorQThreadImpl.cpp \
    # subscribers
    subscribers/DataSubscriber.cpp \
    subscribers/TempTableSubscriber.cpp

# # # # # # # # # # # # # # # # # # # # # # # # # # # #
# support for building with <MxTelemetry.hpp> - BEGIN #
# # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Readme
# memory checks - used Qt Create Memcheck tools

# # # # # PATHS - BEGIN # # # # #
UNIX_Z_PATH = $${PWD}/build_directory

UNIX_Z_LIBRARY_PATH = $${UNIX_Z_PATH}/lib
UNIX_Z_INCLUDE_PATH = $${UNIX_Z_PATH}/include
# # # # # PATHS - END # # # # #

CONFIG += c++17 # set C++17

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $${UNIX_Z_INCLUDE_PATH}
DEPENDPATH  += $${UNIX_Z_INCLUDE_PATH}

QMAKE_CFLAGS = -g -m64 -fno-math-errno -fno-trapping-math -fno-rounding-math -fno-signaling-nans -DZDEBUG
QMAKE_CXXFLAGS = -std=gnu++2a -g -m64 -fno-math-errno -fno-trapping-math -fno-rounding-math -fno-signaling-nans -DZDEBUG
QMAKE_LFLAGS = -Wall -Wno-parentheses -Wno-invalid-offsetof -Wno-misleading-indentation -fstrict-aliasing -std=gnu++2a -g -m64 -fno-math-errno -fno-trapping-math -fno-rounding-math -fno-signaling-nans -DZDEBUG -rdynamic

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZv
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZv
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZv


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZi
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZi


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZe
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZe
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZe


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZt
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZt
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZt


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZm
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZm
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZm


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZu
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZu
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZu


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lMxBase
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lMxBase
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lMxBase


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lMxMD
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lMxMD
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lMxMD


# adding support for lhwloc
LIBS += -lhwloc


# # # # # # # # # # # # # # # # # # # # # # # # # # #
# support for building with <MxTelemetry.hpp> - END #
# # # # # # # # # # # # # # # # # # # # # # # # # # #



