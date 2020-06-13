QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    DetailRecords.cpp \
    Invoice.cpp \
    Report.cpp \
    initialpage.cpp \
    invoicepage.cpp \
    main.cpp \
    mainwindow.cpp \
    queryinvoiceinputdialog.cpp \
    reportpage.cpp

HEADERS += \
    DetailRecords.h \
    Invoice.h \
    Report.h \
    SocketConstants.h \
    initialpage.h \
    invoicepage.h \
    mainwindow.h \
    queryinvoiceinputdialog.h \
    reportpage.h

FORMS += \
    initialpage.ui \
    invoicepage.ui \
    mainwindow.ui \
    queryinvoiceinputdialog.ui \
    reportpage.ui

TRANSLATIONS += \
    ManagerClient_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    picture.qrc

DISTFILES += \
    logo.rc

RC_FILE += logo.rc
