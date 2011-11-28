FORMS += qwzm.ui \
	pieexport.ui
SOURCES += qwzm.cpp \
	wzmutils.c \
	wzmglwidget.cpp \
	conversion.cpp
HEADERS += qwzm.h \
	wzmutils.h \
	wzmglwidget.h \
	conversion.h \
	qhexspinbox.h
TEMPLATE = app
CONFIG += debug \
	warn_on \
	qt \
	precompile_header
TARGET = qwzm

UI_DIR = built
MOC_DIR = built
OBJECTS_DIR = built

# If your system uses different paths for QGLViewer, create a file named
# "config" and override the two variables below (with "=").
QGLVIEWER_INCL = /usr/include/QGLViewer
QGLVIEWER_LIBS = -lQGLViewer
include("config")
INCLUDEPATH += $$QGLVIEWER_INCL
LIBS += -l3ds \
	-lm \
	$$QGLVIEWER_LIBS
QT += opengl \
	xml
