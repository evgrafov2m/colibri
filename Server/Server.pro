TARGET = Server
TEMPLATE = app

QT += core widgets network
CONFIG += c++17

HEADERS += \
    connection.h \
    connectionman.h \
    gui.h

SOURCES += \
    connection.cpp \
    connectionman.cpp \
    gui.cpp \
    main.cpp



