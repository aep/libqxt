win32:include(../../depends.pri)
CONFIG -= app_bundle
TEMPLATE = app
TARGET = xss
DEPENDPATH += .
INCLUDEPATH += .
SOURCES += main.cpp
!win32:LIBS+=-lXss
