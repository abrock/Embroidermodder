########################
#    everything.pro    #
########################
#  This Qt4/5 project  #
#  file will build all #
#  applications        #
########################

TEMPLATE = subdirs
CONFIG += ordered
CONFIG += qt debug

##########################
# Linux/Unix             #
##########################

unix:!macx {
SUBDIRS  = \
../../thumbnailer-kde4 \
../../libembroidery-convert \
../../libembroidery-optimize \
../../libembroidery-optimize/tests.pro \
../../embroidermodder2 \

}

##########################
# Windows                #
##########################

win32 {
SUBDIRS  = \
../../libembroidery-convert \
../../embroidermodder2 \

}

##########################
# Mac OSX                #
##########################

macx {
SUBDIRS  = \
../../libembroidery-convert \
../../embroidermodder2 \

}
