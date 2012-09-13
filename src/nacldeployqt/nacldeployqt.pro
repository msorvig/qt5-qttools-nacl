
#option(host_build)
# Disabled - host build outside of qtbase did not work, plus we depend on QtNetwork

# Get the NaCl toolchain location
#load(device_config) #disabled - gets the host Qt device config

# Manually load qdevice.pri (duplicated from device_config.pri)
DEVICE_PRI = $$PWD/../../../qtbase/mkspecs/qdevice.pri
exists($$DEVICE_PRI):include($$DEVICE_PRI)
unset(DEVICE_PRI)

# message($$CROSS_COMPILE)
DEFINES += NACL_TOOLCHAIN_PATH=$$CROSS_COMPILE

TEMPLATE = app
CONFIG  += qt warn_on
QT =core network concurrent

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

DEPENDPATH += .
INCLUDEPATH += .
TARGET = nacldeployqt

mac {
    QMAKE_INFO_PLIST=Info_mac.plist
    CONFIG-=app_bundle
}

include(../naclshared/naclshared.pri)

# Input
SOURCES += main.cpp \
           httpserver.cpp

HEADERS += httpserver.h

RESOURCES += ../naclshared/naclshared.qrc
OTHER_FILES = \
    ../naclshared/fullwindowtemplate.html \
    ../naclshared/naclnmftemplate.nmf \
    ../naclshared/check_browser.js \
    ../naclshared/qtnaclloader.js \
    ../naclshared/manifest.json \


target.path=$$PWD/../../../qtbase/bin/ # hackety (QT_INSTALL_BINS is the host Qt bin)
INSTALLS += target
