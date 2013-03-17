QT       += core gui network

TARGET = photoTweet
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qtweetlib/lib/ -lqtweetlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qtweetlib/lib/ -lqtweetlibd
else:symbian: LIBS += -lqtweetlib
else:unix: LIBS += -L$$OUT_PWD/../qtweetlib/src/ -lqtweetlib

INCLUDEPATH += $$PWD/../qtweetlib/src
DEPENDPATH += $$PWD/../qtweetlib/src
