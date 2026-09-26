QT += core gui widgets

CONFIG += c++17

TARGET   = pulse_widget
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/PulseWidget.cpp

HEADERS += \
    src/PulseWidget.h

# Platform-specific
linux {
    LIBS += -lm
}

# Build output dirs
OBJECTS_DIR = build/.obj
MOC_DIR     = build/.moc
RCC_DIR     = build/.rcc
DESTDIR     = build
