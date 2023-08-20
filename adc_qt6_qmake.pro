QT += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#unix|win32: LIBS += -lftd2xx
#unix|win32: LIBS += -L$$PWD/./ -lftd2xx

win32: LIBS += -L$$PWD/./ -lftd2xx

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

unix:!macx: LIBS += -L$$PWD/./ -lftd2xx

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

#RC_ICONS = adc_icon.ico

RESOURCES += \
    images.qrc
