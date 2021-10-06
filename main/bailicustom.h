#ifndef BAILICUSTOM_H
#define BAILICUSTOM_H

#include "Dongle_API.h"     //省略路径，则pro文件中必须有INCLUDEPATH+=...
#include <QtCore>

//由于网络服务市场方案，不需要防盗版，只需识别，故而未用到狗数据区，只用到数据文件。

namespace BailiSoft {

//版本名称
extern QString                  versionLicenseName;

//狗信息
extern bool                     dogOk;
extern bool                     dogFoundMother; //是否检测到开发母狗（自用烧录）
extern QString                  dogUserName;
extern QString                  dogNetName;
extern QString                  dogNetPass;
extern QString                  httpUserName;
extern QString                  httpPassHash;

//仅程序启动运行一次，读取狗所有信息
void checkLicenseDog();
void checkLicenseSoftKey();

/*=======================================定制登记与识别函数=======================================*/
void initCustomOptions();   //程序启动初始化定制功能表
void initCustomLicenses();  //程序启动初始化用户定制记录
bool getCustomLicenseOf(const QString &customOptionName);  //判断某锁用户是否有某定制功能

}


/***************************************由于微狗没有64位驱动，拉到***************************************

    //bool                    dogR15          = false;
    //bool                    dogSkyR15       = false;

    //R15微狗数据结构
    typedef struct _MH_DLL_PARA
    {
        WORD            Command;        //BYTE
        WORD            Cascade;        //BYTE
        WORD            DogAddr;
        WORD            DogBytes;
        DWORD           DogPassword;
        DWORD           DogResult;
        DWORD           NewPassword;    //BYTE
        BYTE            DogData[200];
    } MH_DLL_PARA,  *PMH_DLL_PARA;

    //检查R15老狗
    QLibrary microdoglib("MicroDogWin32.dll");
    if (microdoglib.load())
    {
        //取狗通用函数
        typedef unsigned long (PASCAL *R15DogFunc)(PMH_DLL_PARA);
        R15DogFunc r15dogfunc = reinterpret_cast<R15DogFunc>(microdoglib.resolve("GS_MHDog"));
        if ( r15dogfunc ) {
            MH_DLL_PARA     mhp;
            PMH_DLL_PARA    pmhp = &mhp;

            //检查狗
            pmhp->Command = 1;
            pmhp->Cascade = 0;
            pmhp->DogPassword = 117042223;
            dogR15 = (0 == r15dogfunc(pmhp));

            //读取年月
            if ( dogR15 ) {
                char readBuf[7];
                pmhp->Command = 2;
                pmhp->DogAddr = 0x70;
                pmhp->DogBytes = 6;
                dogR15 = (0 == r15dogfunc(pmhp));
                if ( dogR15 ) {
                    memcpy(readBuf, pmhp->DogData, 6);
                    readBuf[6] = '\0';
                    QString yearMonth = QString(readBuf);
                    dogSkyR15 = ( yearMonth == QStringLiteral("201906") ||
                                  yearMonth == QStringLiteral("201907") ||
                                  yearMonth == QStringLiteral("201908") );
                }
            }
        }
    }
    microdoglib.unload();

*****************************************************************************************************/

#endif // BAILICUSTOM_H
