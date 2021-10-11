#CONFIG += c++11

QT += core gui sql network printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
##DEFINES += QT_MESSAGELOGCONTEXT     # for redirect debug info to a file

## message($$[QT_INSTALL_PREFIX])
## INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib

HEADERS += \
    admin_sales/lxsalesmanage.h \
    comm/pinyincode.h \
    comm/expresscalc.h \
    comm/bsflowlayout.h \
    comm/lxstringtablemodel.h \
    comm/barlabel.h \
    comm/qrlabel.h \
    third/tinyAES/aes.h \
    third/code128/code128.h \
    third/code128/code128item.h \
    third/codeqr/codeqr.hpp \
    main/bailicode.h \
    main/bailicustom.h \
    main/bailidata.h \
    main/bailifunc.h \
    main/bailiwins.h \
    main/bailigrid.h \
    main/bailiedit.h \
    main/baililabel.h \
    main/bailisql.h \
    main/bailiterminator.h \
    main/bailipublisher.h \
    main/bailiserver.h \
    main/bailishare.h \
    main/bailidialog.h \
    main/bsmain.h \
    tools/bsbarcodemaker.h \
    tools/bsbatchrename.h \
    tools/bsbatchrecheck.h \
    tools/bslabeldesigner.h \
    tools/bstoolstockreset.h \
    dialog/bsabout.h \
    dialog/bsnetloading.h \
    dialog/bspapersizedlg.h \
    dialog/bsbarcodesimportdlg.h \
    dialog/bsloginguide.h \
    dialog/bspickdatedlg.h \
    dialog/lxwelcome.h \
    dialog/bssetpassword.h \
    dialog/bslicwarning.h \
    dialog/bsloginerbaseinfodlg.h \
    dialog/bscopyimportsheetdlg.h \
    dialog/bslinkbookdlg.h \
    dialog/bscreatecolortype.h \
    dialog/bsshoplocdlg.h \
    dialog/bstagselectdlg.h \
    misc/bsimportr15dlg.h \
    misc/bsimportr16dlg.h \
    misc/bsimportregdlg.h \
    misc/lxbzprinter.h \
    misc/lxbzprintsetting.h \
    misc/bsoption.h \
    misc/bsalarmsetting.h \
    misc/bsalarmreport.h \
    misc/bshistorywin.h \
    misc/bsmdiarea.h \
    misc/bssetloginer.h \
    misc/bsfielddefinedlg.h \
    misc/bssetservice.h \
    misc/bsdebug.h

SOURCES += main/main.cpp \
    admin_sales/lxsalesmanage.cpp \
    comm/pinyincode.cpp \
    comm/expresscalc.cpp \
    comm/bsflowlayout.cpp \
    comm/lxstringtablemodel.cpp \
    comm/barlabel.cpp \
    comm/qrlabel.cpp \
    third/tinyAES/aes.c \
    third/code128/code128.cpp \
    third/code128/code128item.cpp \
    third/codeqr/codeqr.cpp \
    main/bailicode.cpp \
    main/bailicustom.cpp \
    main/bailidata.cpp \
    main/baililabel.cpp \
    main/bailifunc.cpp \
    main/bailiwins.cpp \
    main/bailigrid.cpp \
    main/bailiedit.cpp \
    main/bailisql.cpp \
    main/bailiterminator.cpp \
    main/bailipublisher.cpp \
    main/bailiserver.cpp \
    main/bailishare.cpp \
    main/bailidialog.cpp \
    main/bsmain.cpp \
    dialog/bsabout.cpp \
    tools/bsbarcodemaker.cpp \
    tools/bsbatchrename.cpp \
    tools/bsbatchrecheck.cpp \
    tools/bslabeldesigner.cpp \
    tools/bstoolstockreset.cpp \
    dialog/bsnetloading.cpp \
    dialog/bspapersizedlg.cpp \
    dialog/bsbarcodesimportdlg.cpp \
    dialog/bsloginguide.cpp \
    dialog/bspickdatedlg.cpp \
    dialog/lxwelcome.cpp \
    dialog/bssetpassword.cpp \
    dialog/bslicwarning.cpp \
    dialog/bsloginerbaseinfodlg.cpp \
    dialog/bscopyimportsheetdlg.cpp \
    dialog/bslinkbookdlg.cpp \
    dialog/bscreatecolortype.cpp \
    dialog/bsshoplocdlg.cpp \
    dialog/bstagselectdlg.cpp \
    misc/bsimportr15dlg.cpp \
    misc/bsimportr16dlg.cpp \
    misc/bsimportregdlg.cpp \
    misc/lxbzprinter.cpp \
    misc/lxbzprintsetting.cpp \
    misc/bsoption.cpp \
    misc/bsalarmsetting.cpp \
    misc/bsalarmreport.cpp \
    misc/bshistorywin.cpp \
    misc/bsmdiarea.cpp \
    misc/bssetloginer.cpp \
    misc/bsfielddefinedlg.cpp \
    misc/bssetservice.cpp

RESOURCES += \
    resources/all.qrc

DISTFILES +=


############################ Below is platform difference ############################

#message($$QMAKESPEC)    #Used to show what's default spec when execute qmake without -spec option.

TARGET = BailiR17       #历史已用原因，此名不再加Server后缀，以免更新麻烦

win32 {     #win32 means all windows platform not only win_x86
    RC_ICONS = resources/winlogo.ico

    INCLUDEPATH += $$PWD/third/RockeyDog/win

    SPECVALUE_X64FLAG = $$find(QMAKESPEC, _64)         #test to see $$QMAKESPEC's value
    isEmpty(SPECVALUE_X64FLAG) {
        #DESTDIR = $$PWD/../../BuildOuts/R17_distribute/win32
        #TARGET = BailiR17Win32
        LIBS += -L$$PWD/third/winscard/x86 -lwinscard
        LIBS += -L$$PWD/third/RockeyDog/win/x86 -lDongle_d
        message("WIN x32 compiler")
    }
    else {
        #DESTDIR = $$PWD/../../BuildOuts/R17_distribute/win64
        #TARGET = BailiR17Win64
        LIBS += -L$$PWD/third/winscard/x64 -lwinscard
        LIBS += -L$$PWD/third/RockeyDog/win/x64 -lDongle_d
        message("WIN x64 compiler")
    }
}
else {
    macx {
        #INCLUDEPATH += $$PWD/third/RockeyDog/mac
        #DESTDIR = /Users/roger/BailiR17Dist
        #TARGET = BailiR17Mac64
        #LIBS += -L$$PWD/third/RockeyDog/mac -lRockeyARM
        ICON = resources/maclogo.icns        
        #QMAKE_POST_LINK += $$quote(cp /Users/roger/Downloads/br17demo.dat $$DESTDIR/BailiR17.app/Contents/MacOS/$$escape_expand(\n\t))
        message("Mac OS X compiler")
    }
    else {
        #INCLUDEPATH += $$PWD/third/RockeyDog/linux

        SPECVALUE_X64FLAG = $$find(QMAKESPEC, 64)            #test to see $$QMAKESPEC's value
        isEmpty(SPECVALUE_X64FLAG) {
            #DESTDIR = /home/roger/BailiR17Dist32
            #TARGET = BailiR17Unx32
            #LIBS += -L$$PWD/third/RockeyDog/linux/api32 -llibRockeyARM
            message("UNIX x32 compiler")
        }
        else {
            #DESTDIR = /home/roger/BailiR17Dist64
            #TARGET = BailiR17Unx64
            #LIBS += -L$$PWD/third/RockeyDog/linux/api64 -llibRockeyARM
            message("UNIX x64 compiler")
        }
        #QMAKE_POST_LINK += $$quote(cp /home/roger/Downloads/br17demo.dat $$DESTDIR/$$escape_expand(\n\t))
    }
}

