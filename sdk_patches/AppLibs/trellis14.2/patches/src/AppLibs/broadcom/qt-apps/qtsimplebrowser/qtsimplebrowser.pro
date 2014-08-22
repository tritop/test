# -------------------------------------------------------------------
# Project file for the qtsimplebrowser binary
#
# -------------------------------------------------------------------

TEMPLATE = app

isEmpty(ROOT_WEBKIT_DIR){
    ROOT_WEBKIT_DIR=$$(ROOT_WEBKIT_DIR)
}

INCLUDEPATH += \
    $${ROOT_WEBKIT_DIR}/Source/WebCore/platform/qt \
    $${ROOT_WEBKIT_DIR}/Source/WebKit/qt/WebCoreSupport \
    $${ROOT_WEBKIT_DIR}/Source/WTF \
   ../../../../../../servicemanager/include/
    
SOURCES += \
    qtsimplebrowser.cpp \
    browserapp.cpp \
    browserwindow.cpp \
    utils.cpp \
    webpage.cpp \
    webview.cpp \
    fpstimer.cpp \
    cookiejar.cpp

HEADERS += \
    browserapp.h \
    browserwindow.h \
    utils.h \
    webinspector.h \
    webpage.h \
    webview.h \
    fpstimer.h \
    cookiejar.h

LIBS += -L../../../../../../servicemanager/build/servicemanager -lservicemanager

WEBKIT += wtf webcore

DESTDIR = .

QT += network webkitwidgets widgets

contains(QT_CONFIG, opengl) {
    QT += opengl
    DEFINES += QT_CONFIGURED_WITH_OPENGL
}

OTHER_FILES += \
    README \
    *.pro

TARGET     = qtsimplebrowser

INSTALL_PATH = $$(QT_INSTALL_APPS)

!isEmpty(INSTALL_PATH) {
  target.path   = $$INSTALL_PATH/qtsimplebrowser
  sources.path  = $$INSTALL_PATH/qtsimplebrowser
  sources.files = $$SOURCES $$HEADERS $$OTHER_FILES
  INSTALLS += target sources
}
