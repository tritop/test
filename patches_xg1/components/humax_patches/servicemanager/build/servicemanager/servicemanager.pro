#QMAKE_CXXFLAGS += -g
#QMAKE_CFLAGS += -g

QT       += network webkit

build_directfb {
    LIBS += -L"$(DFB_LIB)"
    LIBS += -L"$(BCMAPP)/lib"
    message(Building with DirectFB support...)
}
else {
    message(Building with OpenGL support...)
    QT       += opengl
}

defineTest(isQtModuleAvailable) {
    unset(module)
    module = $$1

    isEmpty(QT.$${module}.name): return(false)
    return(true)
}

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += webkitwidgets 
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
    isQtModuleAvailable(websockets): DEFINES += ENABLE_WEBSOCKET_SERVICE
}
    
TARGET = servicemanager
TEMPLATE = lib

DEFINES += SERVICEMANAGER_LIBRARY

message(Building Service Manager...)

INCLUDEPATH += ${QTROOT}/include/QtCore \
               ${QTROOT}/include/QtNetwork \
               ${QTROOT}/include/QtGui \
               ${QTROOT}/include/QtOpenGL \
               ${QTROOT}/include/phonon \
               ${QTROOT}/include/QtWebKit \
               ${QTROOT}/include \
               ${QTROOT}/include/phonon_compat

LIBS += -L"${QTROOT}/lib"

INCLUDEPATH += "../../include"
INCLUDEPATH += "../../include/helpers"
INCLUDEPATH += "../../include/services"
INCLUDEPATH += "../../include/humax"

SOURCES += ../../src/servicemanager.cpp \
    ../../src/servicewebacl.cpp \
    ../../src/abstractservice.cpp \
    #../../src/services/homenetworkingservice.cpp \
    ../../src/services/devicesettingservice.cpp \
    ../../src/services/screencaptureservice.cpp \
    ../../src/httpnetworkaccessmgr.cpp \
    ../../src/helpers/screencapture.cpp \
    ../../src/servicemanagerlogger.cpp \
    ../../src/humax/baseservice.cpp \
    ../../src/humax/newservice.cpp
#    ../../src/helpers/persistentcookiejar.cpp \
#    ../../src/services/browsersettingsservice.cpp
HEADERS += ../../include/servicemanager.h\
    ../../include/servicewebacl.h \
        ../../include/servicemanager_global.h \
    ../../include/service.h \
    ../../include/servicedelegate.h \
    ../../include/servicelistener.h \
    ../../include/abstractservice.h \
    ../../include/servicemanagerlogger.h \
    #../../include/services/homenetworkingservice.h \
        ../../include/services/devicesettingservice.h \
    ../../include/services/screencaptureservice.h \
    ../../include/httpnetworkaccessmgr.h \
    ../../include/helpers/screencapture.h \
    ../../include/humax/baseservice.h \
    ../../include/humax/newservice.h
#    ../../include/helpers/persistentcookiejar.h \
#    ../../include/services/browsersettingsservice.h

    #DEFINES += USE_DISPLAY_SETTINGS
    contains(DEFINES,USE_DISPLAY_SETTINGS) {
        INCLUDEPATH += "../../../iarm"
        SOURCES += ../../src/services/displaysettingsservice.cpp
        HEADERS += ../../include/services/displaysettingsservice.h
		INCLUDEPATH += "../../../devicesettings/ds/"
        INCLUDEPATH += "../../../devicesettings/ds/include"
        INCLUDEPATH += "../../../devicesettings/hal/include"
        LIBS += -L"../../../devicesettings/install/lib"
        LIBS += -ldshalcli -lds
        INCLUDEPATH += "../../../logger/include"
        LIBS += -L"../../../logger/install"
        LIBS += -lrdklogger
        LIBS += -L"../../../opensource/lib"
        LIBS += -llog4c
    }

    contains(DEFINES, USE_TSB_SETTINGS) {
        HEADERS += ../../include/services/tsbsettingsservice.h
        SOURCES += ../../src/services/tsbsettingsservice.cpp
     }

    contains(DEFINES, ENABLE_WEBSOCKET_SERVICE) {
        HEADERS += ../../include/services/websocketservice.h
        SOURCES += ../../src/services/websocketservice.cpp
        QT += websockets
    }

# to enable qt breakpad support we need to configure release build
# for details see mkspecs/features/default_post.prf
QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    CONFIG  += release
    CONFIG  -= debug
}
