FORMS += qwzm.ui animationview.ui
SOURCES += qwzm.cpp ../display/wzmutils.c wzmglwidget.cpp conversion.cpp
HEADERS += qwzm.h ../display/wzmutils.h wzmglwidget.h
TEMPLATE = app
CONFIG += warn_on \
          qt \
          precompile_header
TARGET = qwzm
INCLUDEPATH += ../display
LIBS += -l3ds -lm
QT += opengl
