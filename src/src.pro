QT       += core network

TARGET = photoTweet
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainapp.cpp

HEADERS += \
    mainapp.h

CONFIG += console

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qtweetlib/lib/ -lqtweetlib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qtweetlib/lib/ -lqtweetlibd

win32:CONFIG(release, debug|release): PRE_TARGETDEPS = $$OUT_PWD/../qtweetlib/lib/qtweetlib.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS = $$OUT_PWD/../qtweetlib/lib/qtweetlibd.lib

INCLUDEPATH += $$PWD/../qtweetlib/src
DEPENDPATH += $$PWD/../qtweetlib/src
