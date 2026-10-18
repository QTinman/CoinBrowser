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

bool timeractive;
int timerinteval;

settingsDialog::settingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDialog)
{
    ui->setupUi(this);
    int index=0;
    QStringList blacklist,cryptolist,profilelist;
    QStringList exchanges={"Binance","Bittrex","Kraken","FTX","Kucoin"}, maincoins={"BTC","ETH","USDT"};
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
    bool autoupdatejson=loadsettings("autoupdatejson").toBool();
    timeractive=autoupdatejson;
    ui->cryptoInvestFile->setText(loadsettings("cryptoInvestFile").toString());
    // Shows 1000 rather than 0 on a simulator that has never been staked,
    // so the default is visible where the balance is set.
    ui->balance->setValue(stakedBalance(loadsettings("balance").toInt()));
    ui->autoupdatejson->setChecked(autoupdatejson);
    int tableage = loadsettings("tableage").toInt();
    if (tableage == 0) {
        ui->tableage->setValue(999);
        tableage = 999;
    } else ui->tableage->setValue(tableage);
    int autojsonmin = loadsettings("autojsonmin").toInt();
    int mSecDelay = loadsettings("mSecDelay").toInt();
    if (mSecDelay==0) ui->mSecDelay->setValue(20);
    else ui->mSecDelay->setValue(mSecDelay);
    ui->autojsonmin->setValue(autojsonmin);
    if (autojsonmin == 0) {
        ui->autojsonmin->setValue(240);
        autojsonmin = 240;
    } else ui->autojsonmin->setValue(autojsonmin);
    timerinteval=autojsonmin;
    //int updateinterval=loadsettings("updateinterval").toInt();
    //ui->updateinterval->setValue(updateinterval);
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
    msgBox.setText("If you find this program useful please donate to.\nBTC bc1qt4jv89m5gy62e64j9kwyhu7n65s0yhp4nqayag\nETH 0x9129B88AA1fEE2545d1123e71e9Fa2c43C075979\nSOL 3s4nqsu2WiPFrXSmxKY7U3zX4RtKCEgczaNkVBwcgdXS\nZEC t1fb8QAsRdNT2sF6LXoYsHSmHLv18UpPW9D\nSupport is most appreciated.");
    QAbstractButton* pButtonYes = msgBox.addButton(tr("Copy to clipboard"), QMessageBox::YesRole);
    msgBox.exec();
    if (msgBox.clickedButton()==pButtonYes) {
        clipboard->setText("BTC bc1qt4jv89m5gy62e64j9kwyhu7n65s0yhp4nqayag\nETH 0x9129B88AA1fEE2545d1123e71e9Fa2c43C075979\nSOL 3s4nqsu2WiPFrXSmxKY7U3zX4RtKCEgczaNkVBwcgdXS\nZEC t1fb8QAsRdNT2sF6LXoYsHSmHLv18UpPW9D");
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
    savesettings("mSecDelay",ui->mSecDelay->value());
    savesettings("json_path",ui->jsonpathstring->text());
    savesettings("cryptolistread",ui->cryptolistread->text());
    savesettings("cryptolistwrite",ui->cryptolistwrite->text());
    savesettings("reportpath",ui->reportPath->text());
    savesettings(ui->exchanges->currentText().toLower()+"_stake_coin_price",ui->stake_coin_price->value());
    savesettings("report",ui->reports->isChecked());
    savesettings("autoupdatejson",ui->autoupdatejson->isChecked());
    savesettings("autojsonmin",ui->autojsonmin->value());
    savesettings("cryptoInvestFile",ui->cryptoInvestFile->text());
    savesettings("balance",ui->balance->value());
    if ((ui->autoupdatejson->isChecked() && !timeractive) || (timerinteval != ui->autojsonmin->value() && ui->autoupdatejson->isChecked())) {
        // start(), not setInterval(). setInterval on a stopped timer sets the
        // period and leaves it stopped, so ticking Auto upd. here did nothing
        // until the program was restarted - the constructor was the only place
        // that ever started it. start() also restarts a running timer on the
        // new period, which is what changing the interval is asking for.
        timer->start(ui->autojsonmin->value()*60000);
    }
    if (!ui->autoupdatejson->isChecked() && timeractive) timer->stop();
    savesettings("tableage",ui->tableage->value());
}

void settingsDialog::on_cryptoInvestPath_clicked()
{
    QString cryptoInvestPath = QFileDialog::getOpenFileName(this,"Select file exported by CoinBrowser",".");
    ui->cryptoInvestFile->setText(cryptoInvestPath);
}

