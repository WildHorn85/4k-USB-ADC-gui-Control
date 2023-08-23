QT += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    adc_control.cpp \
    dialog.cpp \
    errormsg.cpp \
    main.cpp \
    mainwindow.cpp \
    worker.cpp

HEADERS += \
    WinTypes.h \
    adc_control.h \
    dialog.h \
    errormsg.h \
    ftd2xx.h \
    mainwindow.h \
    worker.h

FORMS += \
    dialog.ui \
    errormsg.ui \
    mainwindow.ui

RESOURCES += \
    images.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -L$$PWD/./ -lftd2xx
unix:!macx: LIBS += -L$$PWD/./ -lftd2xx

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

RC_ICONS = adc_icon.ico
