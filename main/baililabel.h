#ifndef BSLABEL_H
#define BSLABEL_H

#include <QtWidgets>
#include <QtSql>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>

namespace BailiSoft {

class BsLabelDef {
public:
    BsLabelDef(
            const int       nid,
            const int       nobjtype,
            const QString   &svalue,
            const QString   &sexp,
            const int       nposx,
            const int       nposy,
            const int       nwidth,
            const int       nheight,
            const QString   &sfontname,
            const int       nfontpoint,
            const int       nfontalign
            ) :
           nId(nid),
           nObjType(nobjtype),
           sValue(svalue),
           sExp(sexp),
           nPosX(nposx),
           nPosY(nposy),
           nWidth(nwidth),
           nHeight(nheight),
           sFontName(sfontname),
           nFontPoint(nfontpoint),
           nFontAlign(nfontalign)
    {}
    int         nId;
    int         nObjType;
    QString     sValue;
    QString     sExp;
    int         nPosX;
    int         nPosY;
    int         nWidth;
    int         nHeight;
    QString     sFontName;
    int         nFontPoint;
    int         nFontAlign;
};


class BsLabelPrinter
{
public:
    BsLabelPrinter();
    ~BsLabelPrinter();

    //测试打印
    static void doPrintTest(const QString &printerName,
                            const int fromX,
                            const int fromY,
                            const int unitxCount,
                            const int unitWidth,
                            const int unitHeight,
                            const int spaceX,
                            const int spaceY,
                            const int flowNum,
                            const QList<BsLabelDef*> &defines,
                            const QString &cargo,
                            const QString &color,
                            const QString &sizer,
                            const bool testTwoRow,
                            QWidget *dlgParent
            );

    //单据打印
    static void doPrintSheet(const QString &sheetTable,
                             const int &sheetId,
                             QWidget* dlgParent
                             );

    //根据中文名取得字段名
    static QString getFieldOf(const QString &cname);

    //取得货品所有字段
    static QList<QPair<QString, QString> > getCargoFieldsList();

private:

    //样式确认对话框————样式选择、定款、定色、定码
    bool confirmPattern(QWidget *dlgParent);

    //准备样式定义
    bool readySheetLabelDefine();

    //准备测试数据，返回错误或无效数据列表
    QStringList readyTestSkuMaps();

    //准备单据数据，返回错误或无效数据列表
    QStringList readySheetSkuMaps();

    //添加一个SKU单元到数据池
    QString readySku(const QString &cargo, const QString &color, const QString &sizer,
                const int qty, const int priceDots);

    //按数据打印，返回错误
    QString paintDrawOutput();

    //打印一个SKU
    void drawSku(const int xOff, const int yOff, const QString &sku, QPainter *painter);

    //返回单位毫米的打印机像素
    inline int mmUnits(const int prMm) { return int((mPritnerDpi * prMm) / 25.4); }

    //测试条件
    QString         mTestingCargo;
    QString         mTestingColor;
    QString         mTestingSizer;
    bool            mTestingTwoRow;         //测试正常一排，特勾则打印两排测试间距

    //货品条件
    QString         mSheetPattern;          //测试为空，兼用于判断是否测试
    QString         mSheetTable;            //测试为空
    int             mSheetId;               //测试为空
    QString         mSheetLimCargo;
    QString         mSheetLimColor;
    QString         mSheetLimSizer;

    //样式定义
    QString             mPrinterName;
    int                 mFromX;
    int                 mFromY;
    int                 mUnitxCount;
    int                 mUnitWidth;
    int                 mUnitHeight;
    int                 mSpaceX;
    int                 mSpaceY;
    int                 mFlowNum;
    QString             mCargoExp;

    QList<BsLabelDef*>      mTestDefines;       //不需要new
    QList<BsLabelDef*>      mLoadDefines;       //需要new
    QList<BsLabelDef*>*     mppUsingDefines;    //共用指针（两种情况变换）
    BsLabelDef*             mppBarcodeDef;
    BsLabelDef*             mppQrcodeDef;

    int                 mPritnerDpi;

    //打印数据，准备好的顺序表。数组项格式【cargo \t color \t sizer \t barcode \t qrcode】
    QStringList                 mSkus;

    //货号字段列表<中名, 字段名>
    QList<QPair<QString, QString> >         mCargoFields;

    //货号字段字典<中名, 字段名>
    QMap<QString, QString>                  mCargoFieldMap;

    //单例
    static BsLabelPrinter*      instance;
    static BsLabelPrinter&      getInstance();

};

}


#endif // BSLABEL_H
