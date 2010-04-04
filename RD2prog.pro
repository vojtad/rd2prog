TEMPLATE = app
SOURCES += main.cpp \
    MainWindow.cpp \
    MyTextEdit.cpp \
    MyTreeView.cpp \
    MyTreeModel.cpp \
    MyFile.cpp \
    CloseDialog.cpp \
    MyCloseModel.cpp \
    SerialPortInterface.cpp \
    SettingsDialog.cpp \
    ISPFrame.cpp \
    WriteDialog.cpp \
    SynchronizationThread.cpp \
    CompileIssueModel.cpp
HEADERS += MainWindow.h \
    MyTextEdit.h \
    MyTreeView.h \
    MyTreeModel.h \
    MyFile.h \
    CloseDialog.h \
    MyCloseModel.h \
    SerialPortInterface.h \
    SettingsDialog.h \
    ISPFrame.h \
    WriteDialog.h \
    SynchronizationThread.h \
    CompileIssueModel.h
FORMS += MainWindow.ui \
    CloseDialog.ui \
    SettingsDialog.ui \
    WriteDialog.ui
RESOURCES += icons.qrc
unix:SOURCES += 
unix:LIBS += -ludev
win32:DEFINES += WINVER=0x0501
win32:LIBS += -lsetupapi
QT += xml
OBJECTS_DIR = build
MOC_DIR = build
TRANSLATIONS = RD2prog_cs_CZ.ts
CODECFORTR = UTF-8
