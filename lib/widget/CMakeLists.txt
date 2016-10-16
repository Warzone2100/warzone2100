cmake_minimum_required (VERSION 3.5)
project (widget CXX)

qt5_wrap_cpp(MOCFILES bar.h button.h editbox.h form.h label.h listwidget.h slider.h widgbase.h)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(widget ${HEADERS} ${SRC} ${MOCFILES})
target_link_libraries(widget Qt5::Core)
