TEMPLATE = app
TARGET = LithoMaker
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += debug c++17
RESOURCES += lithomaker.qrc
RC_FILE = lithomaker.rc
QT += gui widgets 3dcore 3dextras
TRANSLATIONS = lithomaker_da_DK.ts
QMAKE_CXX = clang++
QMAKE_LINK = clang++
QMAKE_CXXFLAGS +=
LIBS +=

include(./VERSION)
DEFINES+=VERSION=\\\"$$VERSION\\\"

# Input
HEADERS += src/mainwindow.h \
           src/lineedit.h \
           src/slider.h \
           src/combobox.h \
           src/checkbox.h \
           src/configpages.h \
           src/configdialog.h \
           src/aboutbox.h \
           src/lithophane.h \
           src/preview.h

SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/lineedit.cpp \
           src/slider.cpp \
           src/combobox.cpp \
           src/checkbox.cpp \
           src/configpages.cpp \
           src/configdialog.cpp \
           src/aboutbox.cpp \
           src/lithophane.cpp \
           src/preview.cpp