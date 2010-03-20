FORMS += 3ds2pie_gui.ui
SOURCES += 3ds2pie_gui.cpp \
           3ds2pie.cpp \
           main.cpp
HEADERS += 3ds2pie_gui.h \
           3ds2pie.h
TEMPLATE = app
CONFIG += warn_on \
          qt \
          precompile_header
TARGET = 3ds2pie_gui
LIBS += -l3ds

DEFINES += WZ_3DS2PIE_GUI

TRANSLATIONS += 3ds2pie_gui_de.ts
