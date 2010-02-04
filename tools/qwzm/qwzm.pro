FORMS += qwzm.ui animationview.ui connectorview.ui
SOURCES += qwzm.cpp ../display/wzmutils.c wzmglwidget.cpp conversion.cpp
HEADERS += qwzm.h ../display/wzmutils.h wzmglwidget.h
TEMPLATE = app
CONFIG += debug warn_on qt precompile_header
TARGET = qwzm

# If your system uses different paths for QGLViewer, create a file named
# "config" and override the two variables below (with "=").
QGLVIEWER_INCL = /usr/include/QGLViewer
QGLVIEWER_LIBS = -lQGLViewer

include("config")
INCLUDEPATH += ../display $$QGLVIEWER_INCL
LIBS += -l3ds -lm $$QGLVIEWER_LIBS
QT += opengl xml
