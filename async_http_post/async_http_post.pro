#-------------------------------------------------
#
# Project created by QtCreator 2016-12-15T15:05:04
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = async_http_post
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp

INCLUDEPATH += /usr/local/include/

LIBS += -lboost_system -lboost_thread -lboost_filesystem -lhiredis -levent -leasycpp
