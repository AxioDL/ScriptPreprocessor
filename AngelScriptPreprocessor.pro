TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    CLexer.cpp \
    CPreprocessor.cpp \
    CLineTranslator.cpp

HEADERS += \
    CLexer.hpp \
    CPreprocessor.hpp \
    CLineTranslator.hpp

