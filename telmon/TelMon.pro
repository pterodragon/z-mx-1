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
#DEFINES += QT_MESSAGELOGCONTEXT

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


HEADERS +=                           \
    src/controllers/*.h              \
    src/models/raw/*.h               \
    src/models/wrappers/*.h          \
    src/views/raw/*.h                \
    src/factories/*.h                \
    src/network_component/*.h        \
    src/distributors/*.h             \
    src/subscribers/*.h              \
    src/utilities/typeWrappers/*.h   \
    src/widgets/*.h


SOURCES +=                             \
    src/main.cpp                       \
    src/controllers/*.cpp              \
    src/models/raw/*.cpp               \
    src/models/wrappers/*.cpp          \
    src/views/raw/*.cpp                \
    src/factories/*.cpp                \
    src/network_component/*.cpp        \
    src/distributors/*.cpp             \
    src/subscribers/*.cpp              \
    src/utilities/typeWrappers/*.cpp   \
    src/widgets/*.cpp

#SOURCES += $${PWD}/../mxbase/test/*.cpp

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


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/release/ -lZdb
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../sandbox/code/lib/debug/ -lZdb
else:unix: LIBS += -L$${UNIX_Z_LIBRARY_PATH} -lZdb


# adding support for lhwloc
LIBS += -lhwloc


# # # # # # # # # # # # # # # # # # # # # # # # # # #
# support for building with <MxTelemetry.hpp> - END #
# # # # # # # # # # # # # # # # # # # # # # # # # # #



