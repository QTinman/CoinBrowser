#include "settingsdialog.h"
#include "mainwindow.h"
#include "ui_settingsdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore>
#include <QDate>
//#include </usr/local/include/QtSpell-qt5/QtSpell.hpp>
#include "string.h"
#include <QKeyEvent>
#include <QUrl>
#include <QtWidgets>


settingsDialog::settingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDialog)
{
    ui->setupUi(this);
    //MainWindow main;
    int index=0;
    QStringList blacklist,cryptolist,profilelist;
    QStringList exchanges={"Binance","Bittrex","Kraken","FTX"}, maincoins={"BTC","ETH","USDT"};
    ui->exchanges->clear();
    ui->exchanges->addItems(exchanges);

    ui->maincoins->clear();
    cryptolist = loadsettings("cryptolist").toStringList();
    if (cryptolist.isEmpty()) cryptolist = maincoins;
    else ui->maincoins->addItems(cryptolist);
    //qDebug() << ui->exchanges->currentText();
    ui->exchanges->setCurrentText(exchange);
    QString crypt = loadsettings(ui->exchanges->currentText()+"_stake").toString();
    ui->stake_coin->setText(crypt+" Price usd");
    ui->message->setText("If you find this program useful please donate");
    blacklist = loadsettings(exchange.toLower()+"_blacklist").toStringList();
    for(auto & a : blacklist) ui->blacklist->append(a);
    for(auto & a : cryptolist) {
         ui->maincoinslist->append(a);
         if (a==crypt) ui->maincoins->setCurrentIndex(index);
         index++;
    }
    ui->maincoins->setCurrentText(loadsettings(exchange.toLower()+"_stake").toString());
    bool report=loadsettings("report").toBool();
    ui->reports->setChecked(report);
    QString json_path = loadsettings("json_path").toString();
    ui->jsonpathstring->setText(json_path);
    QString cryptolistwrite = loadsettings("cryptolistwrite").toString();
    ui->cryptolistwrite->setText(cryptolistwrite);
    QString cryptolistread = loadsettings("cryptolistread").toString();
    ui->cryptolistread->setText(cryptolistread);
    QString reportPath = loadsettings("reportpath").toString();
    ui->reportPath->setText(reportPath);
    double btc_price = loadsettings(exchange.toLower()+"_stake_coin_price").toDouble();
    if (crypt.contains("USD")) btc_price=1;
    ui->stake_coin_price->setValue(btc_price);
    QString apikey = loadsettings("apikey").toString();
    ui->apikey->setText(apikey);
    bool autoupdatejson=loadsettings("autoupdatejson").toBool();
    ui->autoupdatejson->setChecked(autoupdatejson);
    int tableage = loadsettings("tableage").toInt();
    if (tableage == 0) {
        ui->tableage->setValue(999);
        tableage = 999;
    } else ui->tableage->setValue(tableage);
    int autojsonmin = loadsettings("autojsonmin").toInt();
    ui->autojsonmin->setValue(autojsonmin);
    if (autojsonmin == 0) {
        ui->autojsonmin->setValue(240);
        autojsonmin = 240;
    } else ui->autojsonmin->setValue(autojsonmin);
    int rowsintable = loadsettings("rowsintable").toInt();
    ui->rowsintable->setValue(rowsintable);
    if (rowsintable == 0) {
        ui->rowsintable->setValue(500);
        rowsintable = 500;
    } else ui->rowsintable->setValue(rowsintable);
    int updateinterval=loadsettings("updateinterval").toInt();
    ui->updateinterval->setValue(updateinterval);
}
settingsDialog::~settingsDialog()
{


    delete ui;
}


QVariant settingsDialog::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void settingsDialog::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

void settingsDialog::on_selectJsonFile_clicked()
{
    QString json_path = QFileDialog::getExistingDirectory(this,"Select Json file from CoinMarkedCap",".");
    ui->jsonpathstring->setText(json_path);
}

void settingsDialog::on_toolcryptolistwrite_clicked()
{

    QString cryptolistpath = QFileDialog::getExistingDirectory(this,"Select outfile where pairs are stored",".");
    ui->cryptolistwrite->setText(cryptolistpath);

}

void settingsDialog::on_toolcryptolistread_clicked()
{
    QString cryptolistpath = QFileDialog::getExistingDirectory(this,"Select infile a list of pairs from exhange",".");
    ui->cryptolistread->setText(cryptolistpath);
}

void settingsDialog::on_toolReportPath_clicked()
{
    QString reportpath = QFileDialog::getExistingDirectory(this,"Select reports path",".");
    ui->reportPath->setText(reportpath);
}

void settingsDialog::on_exchanges_activated()
{
    QStringList blacklist;
    //MainWindow main;
    blacklist = loadsettings(ui->exchanges->currentText().toLower()+"_blacklist").toStringList();
    ui->blacklist->clear();
    for(auto & a : blacklist) ui->blacklist->append(a);
    ui->maincoins->setCurrentText(loadsettings(ui->exchanges->currentText().toLower()+"_stake").toString());
}

void settingsDialog::on_pushButton_clicked()
{
    QMessageBox msgBox;
    QClipboard *clipboard=0;
    msgBox.setWindowTitle("Donate!");
    msgBox.setText("If you find this program useful please donate to.\nPaypal to jonssofh@hotmail.com\nBTC 1HJ5xJmePkfrYwixbZJaMUcXosiJhYRLbo\nDOT 12XHN5kYhSfCUdwiEAKMkW87L2kKV2AjerLMQukHJ4CnmKbL\nXRP rGzJmHraBUCWpncm3DGdscmAsuy3rDin4R\nADA addr1q9h424fgyqw3y0zer34myqn9lyr303nxcyvzttk8nyqmr7r0242jsgqazg79j8rtkgpxt7g8zlrxdsgcykhv0xgpk8uqh49hnw\nVET 0x136349A99A5a56617e7E7AdbE8c55a0712B0068F\nSupport is most appreciated.");
    QAbstractButton* pButtonYes = msgBox.addButton(tr("Copy to clipboard"), QMessageBox::YesRole);
    msgBox.exec();
    if (msgBox.clickedButton()==pButtonYes) {
        clipboard->setText("Paypal to jonssofh@hotmail.com\nBTC 1HJ5xJmePkfrYwixbZJaMUcXosiJhYRLbo\nDOT 12XHN5kYhSfCUdwiEAKMkW87L2kKV2AjerLMQukHJ4CnmKbL\nXRP rGzJmHraBUCWpncm3DGdscmAsuy3rDin4R\nADA addr1q9h424fgyqw3y0zer34myqn9lyr303nxcyvzttk8nyqmr7r0242jsgqazg79j8rtkgpxt7g8zlrxdsgcykhv0xgpk8uqh49hnw\nVET 0x136349A99A5a56617e7E7AdbE8c55a0712B0068F");
    }
}


void settingsDialog::on_buttonBox_accepted()
{
    QStringList profilelist,blacklist=ui->blacklist->toPlainText().split("\n"),cryptolist=ui->maincoinslist->toPlainText().split("\n");
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    QString ex=ui->exchanges->currentText();
    appsettings.setValue("cryptolist", QVariant::fromValue(cryptolist));
    appsettings.setValue(ui->exchanges->currentText().toLower()+"_blacklist", QVariant::fromValue(blacklist));
    appsettings.setValue(ui->exchanges->currentText().toLower()+"_stake", QVariant::fromValue(ui->maincoins->currentText()));
    crypt = loadsettings(exchange.toLower()+"_stake").toString();
    appsettings.endGroup();

    savesettings("json_path",ui->jsonpathstring->text());
    savesettings("cryptolistread",ui->cryptolistread->text());
    savesettings("cryptolistwrite",ui->cryptolistwrite->text());
    savesettings("reportpath",ui->reportPath->text());
    savesettings(ui->exchanges->currentText().toLower()+"_stake_coin_price",ui->stake_coin_price->value());
    savesettings("report",ui->reports->isChecked());
    savesettings("apikey",ui->apikey->text());
    savesettings("autoupdatejson",ui->autoupdatejson->isChecked());
    savesettings("autojsonmin",ui->autojsonmin->value());
    savesettings("rowsintable",ui->rowsintable->value());
    savesettings("updateinterval",ui->updateinterval->value());
    savesettings("tableage",ui->tableage->value());
    //crypt = ui->maincoins->currentText();
}
