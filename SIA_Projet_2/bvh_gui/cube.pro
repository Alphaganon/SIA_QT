QT += core gui widgets opengl

TARGET = tpose
TEMPLATE = app

SOURCES += main.cpp

SOURCES += \
    mainwidget.cpp \
    geometryengine.cpp \
    ../joint.cpp \
    ../super_parser.cpp \
    ui/ui_main_widget.cpp

HEADERS += \
    mainwidget.h \
    geometryengine.h \
    ../joint.h \
    ../super_parser.h \
    ui/ui_main_widget.h


RESOURCES += \
    shaders.qrc \
    textures.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/tpose
INSTALLS += target
