AM_CPPFLAGS = $(SDL_CFLAGS) $(WZ_CPPFLAGS) $(QT5_CFLAGS)
AM_CFLAGS = $(WZ_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(QT5_CFLAGS)

noinst_LIBRARIES = libwidget.a
noinst_HEADERS = \
	tip.h \
	widget.h \
	widgint.h \
	bar.h \
	button.h \
	editbox.h \
	form.h \
	label.h \
	listwidget.h \
	slider.h \
	widgbase.h

nodist_libwidget_a_SOURCES = \
	bar_moc.cpp \
	button_moc.cpp \
	editbox_moc.cpp \
	form_moc.cpp \
	label_moc.cpp \
	listwidget_moc.cpp \
	slider_moc.cpp \
	tip_moc.cpp \
	widgbase_moc.cpp \
	widget_moc.cpp \
	widgint_moc.cpp

libwidget_a_SOURCES = \
	bar.cpp \
	button.cpp \
	editbox.cpp \
	form.cpp \
	label.cpp \
	listwidget.cpp \
	slider.cpp \
	tip.cpp \
	widget.cpp
