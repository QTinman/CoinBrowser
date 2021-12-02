#ifndef STOCKSDIALOG_H
#define STOCKSDIALOG_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class stocksDialog;
}

class stocksDialog : public QDialog
{
    Q_OBJECT

public:
    explicit stocksDialog(QWidget *parent = nullptr);
    ~stocksDialog();

private slots:
    void on_buyCrypto_clicked();
    void replyFinished (QNetworkReply *reply);

    void on_sellCrypto_clicked();

private:
    Ui::stocksDialog *ui;
    void createTable(QString table);
    void do_download(QString pair);
    QJsonDocument ReadJson(const QString &path);
    void process_dataframe();
    void load_model();
    QStringList initializemodel();
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    void combo_refresh(int comboindex);
    void delay(int msec);
    void closedelay(double close);
    QStringList readpairfromfile();
    QStandardItemModel *model;
    QNetworkAccessManager *manager;
};

#endif // STOCKSDIALOG_H
