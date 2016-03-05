TEMPLATE = app
CONFIG -= debug_and_release qt
CONFIG += console
CONFIG -= app_bundle
CONFIG += silent #Comment this out for verbose output
#CONFIG += deploy #Uncomment this to create a release build or by running this from the terminal: qmake CONFIG+=deploy

deploy {
    message("release build")
    CONFIG -= debug
    CONFIG += release
} else {
    message("debug build")
    CONFIG += debug #This adds -g to the compiler flags so valgrind can locate the exact line.
    CONFIG -= release
}

!msvc {
    !macx { #TODO: better clang support
        #QMAKE_LFLAGS += -static-libgcc #TODO: Only static link when targeting Windows and building with MinGW (natively or cross-compile)
    }
}

TARGET = libembroidery-convert

LIBS += -L/usr/local/lib \
-lopencv_core \
-lopencv_imgproc \
-lopencv_imgcodecs \
-lopencv_highgui \
-lopencv_ml \
-lopencv_video \
-lopencv_features2d \
-lopencv_calib3d \
-lopencv_objdetect \
-lopencv_flann

QMAKE_CXXFLAGS += -std=c++11

OBJECTS_DIR = .obj
MOC_DIR = .moc

INCLUDEPATH += \
../libembroidery \
../libcgeometry \
$$PWD \

SOURCES += \
    libembroidery-convert-main.cpp \
    drawing.cpp

include( ../libembroidery/libembroidery.pri )

#Install Linux/Unix
unix:!macx {
QMAKE_STRIP    = echo                       #Suppress strip errors "File format not recognized"
QMAKE_DEL_DIR += --ignore-fail-on-non-empty #Suppress rmdir errors "Directory not empty"

convertbin.path  = "/usr/bin"
convertbin.files = "libembroidery-convert"
convertbin.extra = "strip libembroidery-convert; cp -f libembroidery-convert /usr/bin/libembroidery-convert" #ensure the binary gets stripped of debug symbols

INSTALLS += convertbin \

}
