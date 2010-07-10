TEMPLATE = app

# Configuration
CONFIG += warn_on \
    qt
QT += opengl
LIBS += -lfftw3

# Source files
SOURCES += src/colorbox.cpp \
    src/configdialog.cpp \
    src/configpages.cpp \
    src/dataanalyzer.cpp \
    src/dockwindows.cpp \
    src/dsocontrol.cpp \
    src/dsowidget.cpp \
    src/exporter.cpp \
    src/glgenerator.cpp \
    src/glscope.cpp \
    src/helper.cpp \
    src/levelslider.cpp \
    src/main.cpp \
    src/openhantek.cpp \
    src/settings.cpp \
    src/hantek/control.cpp \
    src/hantek/device.cpp \
    src/hantek/types.cpp
HEADERS += src/colorbox.h \
    src/configdialog.h \
    src/configpages.h \
    src/constants.h \
    src/dataanalyzer.h \
    src/dockwindows.h \
    src/dsocontrol.h \
    src/dsowidget.h \
    src/exporter.h \
    src/glscope.h \
    src/glgenerator.h \
    src/helper.h \
    src/levelslider.h \
    src/openhantek.h \
    src/settings.h \
    src/hantek/control.h \
    src/hantek/device.h \
    src/hantek/types.h

# Ressource files
RESOURCES += res/application.qrc \
    res/configdialog.qrc

# Doxygen files
DOXYFILES += Doxyfile \
    mainpage.dox

# Files copied into the distribution package
DISTFILES += ChangeLog \
    COPYING \
    INSTALL \
    res/images/*.png \
    res/images/*.svg \
    translations/*.qm \
    translations/*.ts \
    $${DOXYFILES}

# Translations
TRANSLATIONS += translations/openhantek_de.ts

# Program version
VERSION = 0.2.0-pre

# Destination directory for built binaries
DESTDIR = bin

# Prefix for installation
PREFIX = $$(PREFIX)

# Build directories
OBJECTS_DIR = build/obj
UI_DIR = build/ui
MOC_DIR = build/moc

# Include directory
QMAKE_CXXFLAGS += "-iquote src"

# libusb version
LIBUSB_VERSION = $$(LIBUSB_VERSION)
contains(LIBUSB_VERSION, 0):LIBS += -lusb
else { 
    LIBUSB_VERSION = 1
    LIBS += -lusb-1.0
    DEFINES += LIBUSB_VERSION=1
}
DEFINES += LIBUSB_VERSION=$${LIBUSB_VERSION}

# Settings for different operating systems
unix:!macx { 
    isEmpty(PREFIX):PREFIX = /usr/local
    TARGET = openhantek
    
    # Installation directories
    target.path = $${PREFIX}/bin
    translations.path = $${PREFIX}/share/apps/openhantek/translations
    INCLUDEPATH += /usr/include/libusb
    DEFINES += QMAKE_TRANSLATIONS_PATH=\\\"$${translations.path}\\\" \
        OS_UNIX
}
macx { 
    isEmpty(PREFIX):PREFIX = OpenHantek.app
    TARGET = OpenHantek
    
    # Installation directories
    target.path = $${PREFIX}/Contents/MacOS
    translations.path = $${PREFIX}/Contents/Resources/translations
    DEFINES += QMAKE_TRANSLATIONS_PATH=\\\"Contents/Resources/translations\\\" \
        OS_DARWIN
}
win32 { 
    isEmpty(PREFIX):PREFIX = OpenHantek
    TARGET = OpenHantek
    
    # Installation directories
    target.path = $${PREFIX}
    translations.path = $${PREFIX}/translations
    DEFINES += QMAKE_TRANSLATIONS_PATH=\\\"translations\\\" \
        OS_WINDOWS
}
translations.files += translations/*.qm
INSTALLS += target \
    translations
DEFINES += VERSION=\\\"$${VERSION}\\\"

# Custom target "doc" for Doxygen
doxygen.target = doc
doxygen.commands = rm \
    -r \
    doc/; \
    env \
    DEFINES=\"$${DEFINES}\" \
    doxygen \
    Doxyfile
doxygen.depends = $${SOURCES} \
    $${HEADERS} \
    $${DOXYFILES}
QMAKE_EXTRA_UNIX_TARGETS += doxygen
