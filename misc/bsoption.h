#ifndef BSOPTION_H
#define BSOPTION_H

#include <QtWidgets>

namespace BailiSoft {

class BsOption : public QDialog
{
    Q_OBJECT
public:
    BsOption(QWidget *parent);
    QTableWidget    *mpGrid;
    bool    netEncryptionKeyUpdated;
    bool    bossNameChanged;

private slots:
    void doHelp();
    void saveData();

private:
    void loadData();
};

//图片选择存为Base64项
class LxPicturePickDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LxPicturePickDelegate(const int colIdx, const int rowIdx, QTableWidget *parent);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private slots:
    void commitAndCloseEditor();
private:
    QTableWidget *mppView;
    int mColIdx;
    int mRowIdx;
};

//主题色
class LxColorPickDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LxColorPickDelegate(const int colIdx, const int rowIdx, QTableWidget *parent);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private slots:
    void commitAndCloseEditor();
private:
    QTableWidget *mppView;
    int mColIdx;
    int mRowIdx;
};

}

#endif // BSOPTION_H
