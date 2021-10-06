使用QLiabrary调用

typedef struct _MH_DLL_PARA
{
    WORD/*BYTE*/	Command;
    WORD/*BYTE*/	Cascade;
    WORD            DogAddr;
    WORD            DogBytes;
    DWORD           DogPassword;
    DWORD           DogResult;
    DWORD/*BYTE*/   NewPassword;
    BYTE            DogData[200];
} MH_DLL_PARA,  *PMH_DLL_PARA;





    //检查R15老狗
    QLibrary microdoglib("MicroDogWin32.dll");
    if (microdoglib.load())
    {
        //取狗通用函数
        typedef unsigned long (PASCAL *R15DogFunc)(PMH_DLL_PARA);
        R15DogFunc r15dogfunc = (R15DogFunc)microdoglib.resolve("GS_MHDog");
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
                    memcpy(readBuf, (char *)pmhp->DogData, 6);
                    readBuf[6] = '\0';
                    QString yearMonth = QString((const char *)readBuf);
                    dogSkyR15 = ( yearMonth == QStringLiteral("201906") ||
                                  yearMonth == QStringLiteral("201907") ||
                                  yearMonth == QStringLiteral("201908") );
                }
            }
        }
    }
    microdoglib.unload();
