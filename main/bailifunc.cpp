#include "bailifunc.h"
#include "comm/pinyincode.h"

#include <math.h>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QtWidgets>
#include <QNetworkReply>
#include <QHttpMultiPart>

namespace BailiSoft {

const QString               hostAddr = QStringLiteral("https://www.aimeiwujia.com");

QNetworkAccessManager       netManager;

//小端判断
bool is_little_endian()
{
  unsigned short flag = 0x4321;
  unsigned char * p = reinterpret_cast<unsigned char *>(&flag);
  return (*p == 0x21);
}

//判断使用系统是否Win64
bool osIsWin64 ()
{
    SYSTEM_INFO si;
    memset(&si, 0, sizeof(si));
    typedef void (WINAPI *LPFN_PGNSI)(LPSYSTEM_INFO);
    LPFN_PGNSI pGNSI = LPFN_PGNSI(GetProcAddress(GetModuleHandleA(("kernel32.dll")),"GetNativeSystemInfo"));

    if (pGNSI)
        pGNSI(&si);

    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        return true;

    return false;
}

//产生随机字符串
QString generateReadableRandomString(const quint8 byteLen)
{
    const int COUNT = 54;
    static const char charset[COUNT + 1] = "abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789";
    QString result;
    for (int i = 0; i < byteLen; i++) {
        result += charset[ QRandomGenerator::securelySeeded().generate() % COUNT ];
    }
    return result;
}

QString generateRandomString(const quint8 byteLen)
{
    const int COUNT = 62;
    static const char charset[COUNT + 1] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    QString result;
    for (int i = 0; i < byteLen; i++) {
        result += charset[ QRandomGenerator::securelySeeded().generate() % COUNT ];
    }
    return result;
}

QByteArray generateRandomAsciiBytes(const quint32 byteLen)
{
    const int COUNT = 92;
    static const char charset[COUNT + 1] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890~`!@#$%^&*()_+-={}|[]:;'<>?,./";
    QString result;
    for (int i = 0; i < int(byteLen); i++) {
        result += charset[ QRandomGenerator::securelySeeded().generate() % COUNT ];
    }
    return result.toLatin1();
}

QByteArray generateRandomBytes(const quint32 byteLen)
{
    QByteArray result;
    for (int i = 0; i < int(byteLen); i++) {
        result += char((QRandomGenerator::securelySeeded().generate() % 256));
    }
    return result;
}

QString fetchAsiicPrefix(const QString text)
{
    QString ascii;
    for ( int i = 0, iLen = text.length(); i < iLen; ++i ) {
        QChar c = text.at(i);
        if ( c.unicode() < 127 ) {
            ascii += c;
        } else {
            break;
        }
    }
    return ascii;
}

QString fetchCodePrefix(const QString text)
{
    QString codes;
    for ( int i = 0, iLen = text.length(); i < iLen; ++i ) {
        QChar c = text.at(i);
        int x = c.unicode();
        if ( (x >= 48 && x <= 57) || (x >= 65 && x <= 90) || (x >= 97 && x <= 122) ) {
            codes += c;
        } else {
            break;
        }
    }
    return codes;
}

QDate dateOfFormattedText(const QString &text, const QChar splittor)
{
    QDate date;
    QStringList ls = text.split(splittor);
    if (ls.length() == 3) {
        bool yOk, mOk, dOk;
        int y = QString(ls.at(0)).toInt(&yOk);
        int m = QString(ls.at(1)).toInt(&mOk);
        int d = QString(ls.at(2)).toInt(&dOk);
        if ( yOk && mOk && dOk ) {
            date = QDate(y, m, d);
        }
    }
    if ( date.isValid() )
        return date;
    else
        return QDateTime::currentDateTime().date();
}



bool copyDir(const QString &source, const QString &destination, bool forceOverride)
{
    QDir directory(source);
    if (!directory.exists()) {
        return false;
    }

    QString srcPath = QDir::toNativeSeparators(source);
    if (!srcPath.endsWith(QDir::separator()))
        srcPath += QDir::separator();
    QString dstPath = QDir::toNativeSeparators(destination);
    if (!dstPath.endsWith(QDir::separator()))
        dstPath += QDir::separator();

    bool foundError = false;
    QStringList fileNames = directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    for ( QStringList::size_type i = 0; i != fileNames.size(); ++i ) {
        QString fileName = fileNames.at(i);
        QString srcFilePath = srcPath + fileName;
        QString dstFilePath = dstPath + fileName;
        QFileInfo fileInfo(srcFilePath);
        if (fileInfo.isFile() || fileInfo.isSymLink()) {
            if (forceOverride) {
                QFile::setPermissions(dstFilePath, QFile::WriteOwner);
            }
            QFile::copy(srcFilePath, dstFilePath);
        }
        else if (fileInfo.isDir()) {
            QDir dstDir(dstFilePath);
            dstDir.mkpath(dstFilePath);
            if (!copyDir(srcFilePath, dstFilePath, forceOverride)) {
                foundError = true;
            }
        }
    }

    return !foundError;
}

void disableEditInputMethod(QLineEdit *edt)
{
#ifdef Q_OS_WIN
    edt->setAttribute(Qt::WA_InputMethodEnabled, false);
#else
    Q_UNUSED(edt);
#endif
}

//QMessageBox::warning(...);
//QMessageBox::critical(...);
//QMessageBox::information(...);
bool confirmDialog(QWidget *parent,
                   const QString &intro,
                   const QString &ask,
                   const QString &yes,
                   const QString &no,
                   QMessageBox::Icon icon)
{
    QMessageBox msgBox(parent);
    msgBox.setText(intro);
    msgBox.setInformativeText(ask);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.button(QMessageBox::Ok)->setText(yes);
    msgBox.button(QMessageBox::Cancel)->setText(no);
    msgBox.setIcon(icon);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setFixedSize(msgBox.sizeHint());  //仅仅为了去掉讨厌的Unable to set geometry...的提示
    return (msgBox.exec() == QMessageBox::Ok);
}


QString openLoadTextFile(const QString &openWinTitle,
                         const QString &openFileInitDir,
                         const QString &openFileType,
                         QWidget *parentWin)
{
    QString fileName = QFileDialog::getOpenFileName(parentWin,
                                                    openWinTitle,
                                                    openFileInitDir,
                                                    openFileType
#ifdef Q_OS_MAC
                                                    ,0
                                                    ,QFileDialog::DontUseNativeDialog
#endif
                                                    );
    if (fileName.length() < 1)
        return QString();

    //判断文件是否为unicode编码
    if ( LxSoft::isTextUnicode(fileName) ) {
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    }

    //打开数据文件，并读入表格
    QString fileData;
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream strm(&f);
        fileData = strm.readAll();
        f.close();
    }
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("System"));

    //跨平台处理
    fileData.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    fileData.replace(QStringLiteral("\r"), QStringLiteral("\n"));

    //返回
    return fileData;
}


//专用安全验证请求头
void setVerifyHeader(QNetworkRequest *req, const QString &userName, const QString &userPassHash)
{
    qint64  csec  = QDateTime::currentMSecsSinceEpoch() / 1000;
    QString nonce = generateRandomString(64);
    QString shatext  = QString("%1%2%3%4").arg(userName).arg(userPassHash).arg(csec).arg(nonce);
    QString shasumm  = QCryptographicHash::hash(shatext.toUtf8(), QCryptographicHash::Sha256).toHex();
    req->setRawHeader("Baili-User",      userName.toUtf8());
    req->setRawHeader("Baili-Seconds",   QString::number(csec).toLatin1());
    req->setRawHeader("Baili-Nonce",     nonce.toLatin1());
    req->setRawHeader("Baili-Sha256",    shasumm.toLatin1());
}

//发起网络请求
QNetworkReply* httpRequest(const QString &path, const QString &reqData, const QString &uploadFile,
                           const QString &userName, const QString &userPassHash)
{
    //支持协议检测
//    qDebug() << "supportedSchemes:" << netManager.supportedSchemes();
//    qDebug() << "sslLibraryBuildVersionString:" << QSslSocket::sslLibraryBuildVersionString();
//    qDebug() << "supportsSsl:" << QSslSocket::supportsSsl();
//    qDebug() << "sslLibraryVersionString:" << QSslSocket::sslLibraryVersionString();

    //约定使用\x01与\x02格式化参数的，使用POST，否则当作GET参数处理
    if ( reqData.indexOf(QChar(1)) < 0 && reqData.length() < 1024 && uploadFile.isEmpty() ) {

        QString urlText = hostAddr + path;
        if ( !reqData.isEmpty() ) {
            QString antiCache = generateRandomString(16);
            urlText += QStringLiteral("?%1&q=%2").arg(reqData).arg(antiCache);
        }
        const QUrl url = QUrl::fromUserInput(urlText);
        QNetworkRequest request(url);

        QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
        sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConf.setProtocol(QSsl::TlsV1SslV3);
        request.setSslConfiguration(sslConf);

        //安全验证头
        if ( !userName.isEmpty() && !userPassHash.isEmpty() ) {
            setVerifyHeader(&request, userName, userPassHash);
        }

        //发送请求
        return netManager.get(request);

    }
    else {

        //请求结构
        const QUrl url = QUrl::fromUserInput(hostAddr + path);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

        //POST数据体
        QByteArray body = reqData.toUtf8();

        //追加数据
        if ( ! uploadFile.isEmpty()  && QFile(uploadFile).exists() ) {

            //打开文件
            QFile file(uploadFile);
            if ( file.open(QFile::ReadOnly) ) {

                //识别头（约定32位）
                QString binBoundary = "=AiMeiWuJia.Com=" + generateReadableRandomString(16);
                request.setRawHeader("Baili-Boundary", binBoundary.toLatin1());

                //文件数据
                QByteArray binData = file.readAll();

                //追加数据
                body += binBoundary.toLatin1();
                body += binData;
            }
        }

        //安全验证头
        if ( !userName.isEmpty() && !userPassHash.isEmpty() ) {
            setVerifyHeader(&request, userName, userPassHash);
        }

        //发送请求
        return netManager.post(request, body);

        /*
        //复合表单
        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
        textPart.setBody(reqData.toUtf8());
        multiPart->append(textPart);

        if ( ! uploadFile.isEmpty() && QFile(uploadFile).exists() ) {
            QHttpPart imagePart;
            imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
            imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\""));
            QFile *file = new QFile(uploadFile);
            file->open(QIODevice::ReadOnly);
            imagePart.setBodyDevice(file);
            file->setParent(multiPart);
            multiPart->append(imagePart);
        }

        //安全验证头
        if ( !reqData.isEmpty() || !uploadFile.isEmpty() ) {
            setVerifyHeader(&request, userName, userPassHash);
        }

        //发送请求
        QNetworkReply *reply = netManager.post(request, multiPart);
        multiPart->setParent(reply);
        return reply;
        */
    }
}

}

