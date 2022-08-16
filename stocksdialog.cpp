#include "stocksdialog.h"
#include "ui_stocksdialog.h"
#include <QtCore>
#include <QtGui>
#include <QtSql>
#include <QGuiApplication>
#include "worker.h"

QStringList modellist;
Worker worker;
QString filename="cryptoInvest.json", table="cryptoInvest", pair, transfere_error="";
double closePrice;
int tableColums=8;

stocksDialog::stocksDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::stocksDialog)
{

    ui->setupUi(this);
    setGeometry(loadsettings("stockPosition").toRect());
    ui->balance->setText("$"+loadsettings("balance").toString());
    if (!db.open()) ui->messages->setText("Error " + db.lastError().text());
    if (!db.tables().contains(table)) createTable(table);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    this->setWindowTitle("Portfolio simulator");

    ui->crypto->addItems(readpairfromfile());
    //do_download(ui->crypto->currentText());
    //double close=closePrice;
    //closedelay(close);
    ui->getPrice->setText("Get price");
    QStringList modellist = initializemodel();
    model = new QStandardItemModel(modellist.length()/tableColums,tableColums,this);
    //connect(ui->search,SIGNAL(textEdited(const QString &)),this,SLOT(searchmodel(const QString&)));
    load_model();

    model->setHeaderData(0, Qt::Horizontal, "Symbol", Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, "Buy date", Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, "Buy price", Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, "Crypto amount", Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "Current price", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "USD buy price", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "Proffit", Qt::DisplayRole);
    model->setHeaderData(7, Qt::Horizontal, "Current price", Qt::DisplayRole);
    connect(ui->crypto, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index){ combo_refresh(index); });
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->sortByColumn(0,Qt::AscendingOrder);
}

stocksDialog::~stocksDialog()
{
    savesettings("stockPosition",this->geometry());
    delete ui;
}

QVariant stocksDialog::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void stocksDialog::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

void stocksDialog::createTable(QString table)
{
    QString createTables = "create table "+table+"(id integer primary key, "
          "symbol varchar(7), "
          "date varchar(25), "
          "buyPrice real, "
          "cryptoAmount real, "
          "currentPrice real, "
          "USD_amount integer)";
    ui->messages->setText("Creating database, please wait...");
    if (!db.open()) ui->messages->setText("Error " + db.lastError().text());
    QSqlQuery qry(db);
    if (!qry.exec(createTables)) ui->label->setText("Error "+qry.lastError().text());
    qry.finish();
    ui->messages->clear();
}


void stocksDialog::do_download(QString pair)  // Docs https://binance-docs.github.io/apidocs/spot/en/
{
    QUrl url;
    int limit=1;
    pair+="USDT";
    long startdate = QDateTime(QDate::currentDate(),QTime::currentTime()).toMSecsSinceEpoch()-(3600000);
    url = QUrl(QString("https://www.binance.com/api/v3/klines?symbol="+pair+"&interval=5m&limit="+QString::number(limit)+"&startTime="+QString::number(startdate)));
    QNetworkRequest request(url);
    manager->get(request);
}

QJsonDocument stocksDialog::ReadJson(const QString &path)
{
    QFile file( path );
    QJsonArray jsonArray;
    QJsonDocument cryptolist;
    if( file.open( QIODevice::ReadOnly ) )
    {
        QByteArray bytes = file.readAll();
        file.close();

        QJsonParseError parserError;
        cryptolist = QJsonDocument::fromJson(bytes, &parserError);
        QJsonObject jsonObject = cryptolist.object();

    }
    return cryptolist;
}

void stocksDialog::replyFinished (QNetworkReply *reply)
{

    if(reply->error())
    {
        QByteArray rawtable=reply->readAll();
        ui->messages->setText("ERROR!"+reply->errorString()+rawtable);
        qDebug() << "Error: " << reply->error() <<
        ", Message: " << reply->errorString() <<
        ", Code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
        ", Description: " << rawtable;
        transfere_error="error";
    }
    else
    {
        QFile *file = new QFile(filename);
        if (file->error()) ui->messages->setText(file->errorString());
        QByteArray data=reply->readAll();
        QJsonParseError parserError;
        QJsonDocument cryptolist;
        cryptolist = QJsonDocument::fromJson(data, &parserError);
        if(file->open(QFile::WriteOnly))
        {
            file->write(data);
            file->flush();
            file->close();
        }
        delete file;
        QString Qclose = cryptolist[0][4].toString();
        closePrice=Qclose.toDouble();
        transfere_error="";
        process_dataframe(filename);
    }

    reply->deleteLater();
}

void stocksDialog::process_dataframe(QString fname)
{
    QJsonDocument jsonDoc = ReadJson(fname);
    double low=0,high=0,open=0,close,volumn=0;
    double opentimestamp=0,closetimestamp=0;

    QString Qopen,Qhigh,Qlow,Qclose,Qvolum;
    int trades=0;
    QDateTime dt;
    QTime ct=QTime::currentTime();
    Qopen = jsonDoc[0][1].toString(); // 1=open
    Qhigh = jsonDoc[0][2].toString(); // 2=high
    Qlow = jsonDoc[0][3].toString(); // 3=low
    Qclose = jsonDoc[0][4].toString(); // 4=close
    Qvolum = jsonDoc[0][5].toString(); // 4=volumn
    trades = jsonDoc[0][8].toInt(); // 4=close
    opentimestamp = jsonDoc[0][0].toDouble(); // open time
    closetimestamp = jsonDoc[0][6].toDouble(); // close time
    low = Qlow.toDouble();
    high = Qhigh.toDouble();
    open = Qopen.toDouble();
    close = Qclose.toDouble();
    volumn = Qvolum.toDouble();
    closePrice=close;

}

QStringList stocksDialog::initializemodel()
{
    QString sqlquery, db_symbol, db_date;
    double db_buyPrice, db_cryptoAmount, db_currentPrice, db_USD_amount, totalproffit;
    int sumProffit=0;
    QStringList modeldatalist;
    QSqlQuery qry(db);
    sqlquery="SELECT * FROM "+table+";";
    int db_id;
    totalproffit=0;
    if (!qry.prepare(sqlquery)) {
      ui->messages->setText("prepare failed");
    }
    if (qry.exec()) {
      while (qry.next()) {
        db_id = qry.value(0).toInt();
        db_symbol = qry.value(1).toString();
        db_date = qry.value(2).toString();
        db_buyPrice = qry.value(3).toDouble();
        db_cryptoAmount = qry.value(4).toDouble();
        db_currentPrice = qry.value(5).toDouble();
        db_USD_amount = qry.value(6).toInt();
        if (!db_symbol.isEmpty()) {
            //closePrice=0;
            //do_download(db_symbol);
            //closedelay(closePrice);
            double proffit=(db_currentPrice-db_buyPrice)*db_cryptoAmount;
            int totalUSD=db_cryptoAmount*db_currentPrice;
            totalproffit+=proffit;
            sumProffit+=totalUSD;
            modeldatalist << db_symbol << db_date << QString::number(db_buyPrice, 'F', 3) << QString::number(db_cryptoAmount, 'F', 2) << QString::number(db_currentPrice, 'F', 2) << QString::number(db_USD_amount) << QString::number(proffit, 'F', 2) << QString::number(totalUSD);
        }
      }
    }
    ui->proffit->setText("$"+QString::number(totalproffit, 'F', 3));
    ui->sumProfit->setText("$"+QString::number(sumProffit));
    return modeldatalist;
}

void stocksDialog::load_model()
{
    int row=0,i=0,col;
    QModelIndex index;
    QDateTime cdt=QDateTime::currentDateTime();
    QStringList modellist = initializemodel();
    model->setRowCount(modellist.length()/tableColums);
    while (i < modellist.length()-1) {
       for (col=0;col<tableColums;col++) {
         index=model->index(row,col,QModelIndex());
         if (col < 2) model->setData(index,modellist[i]);
         if (col < 5 & col > 1) model->setData(index,modellist[i].toDouble());
         if (col == 5) model->setData(index,modellist[i].toInt());
         if (col == 6) model->setData(index,modellist[i].toDouble());
         if (col == 7) model->setData(index,modellist[i].toInt());
        // model->setData(index, Qt::ColorOnly) color on numbers
         model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         i++;
        }
      row++;
    }
}

void stocksDialog::delay(int msec)
{
    QTime dieTime= QTime::currentTime().addMSecs(msec);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void stocksDialog::closedelay(double close)
{
    QTime dieTime= QTime::currentTime().addMSecs(1000);
    while (close == closePrice) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        if (QTime::currentTime() > dieTime) break;
    }
    dieTime= QTime::currentTime().addMSecs(150);
    while (QTime::currentTime() < dieTime) QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

QStringList stocksDialog::readpairfromfile()
{
    QString content="";
    QStringList symbolLists;
    QString filename=loadsettings("cryptoInvestFile").toString();
    if (!filename.isEmpty()) {
        QFile file;
        file.setFileName(filename);
        if (file.error()) ui->messages->setText(file.errorString());
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            content = in.readAll();
            file.close();
        }
        content.remove(",");
        content.remove("\t");
        content.remove("/");
        content.remove("\"");
        content.remove(" ");
        content.remove(" \n");
        content.remove("USDT");
        content.remove("BUSD");
        content=content.trimmed();
        symbolLists = content.split("\n");
        symbolLists.sort();
    } else ui->messages->setText("Pairlist file missing, please add in settings");
    return symbolLists;
}

void stocksDialog::combo_refresh(int comboindex)
{
    ui->getPrice->setText("Get price");
}

void stocksDialog::on_buyCrypto_clicked()
{
    if (ui->getPrice->text() != "Get price") {
        int usd=ui->usd->value();
        double amount=usd/ui->getPrice->text().toDouble();
        QString balance=loadsettings("balance").toString();
        int id=0;
        QSqlQuery q;
        q.prepare("SELECT COUNT (*) FROM "+table);
        q.exec();
        id= 0;
        if (q.next()) {
        id= q.value(0).toInt();
        }
        if (usd>balance.toInt()) ui->messages->setText("Error, not enough balance");
        else {
            QString sqlquery;
            QSqlQuery insert_qry(db);
            QString cdt=QDateTime::currentDateTime().toString("d MMM - hh:mm");
            sqlquery = "INSERT INTO "+table+" (id,symbol,date,buyPrice,cryptoAmount,currentPrice,USD_amount) VALUES ("+QString::number(id)+", '"+ui->crypto->currentText()+"', '"+cdt+"', "+QString::number(ui->getPrice->text().toDouble())+", "+QString::number(amount)+", "+QString::number(ui->getPrice->text().toDouble())+", "+QString::number(usd)+");";
            if (!insert_qry.exec(sqlquery)) ui->messages->setText("Error " + insert_qry.lastError().text());
            insert_qry.finish();
            savesettings("balance",balance.toInt()-usd);
            ui->balance->setText(loadsettings("balance").toString());
            load_model();
        }
    } else ui->messages->setText("Press Get price before purhace");
}


void stocksDialog::on_sellCrypto_clicked()
{
    int usd=0;
    QString balance=loadsettings("balance").toString();
    QModelIndexList indexes = ui->tableView->selectionModel()->selection().indexes();
    if (!indexes.isEmpty()) {
        QModelIndex index = indexes.at(0);
        crypt = model->data(index.sibling(index.row(),0)).toString();
        double amount = model->data(index.sibling(index.row(),3)).toDouble();
        double price = model->data(index.sibling(index.row(),4)).toDouble();
        int USD_amount = model->data(index.sibling(index.row(),5)).toInt();
        if (ui->usd->value()>0 && amount*price >= ui->usd->value()) {
            usd=ui->usd->value();
            double rest=amount-(usd/price);
            USD_amount-=usd;
            QString sqlquery;
            QSqlQuery update_qry(db);
            if (USD_amount>0)
                sqlquery = "UPDATE "+table+" SET cryptoAmount = "+QString::number(rest)+", USD_amount = "+QString::number(USD_amount)+" WHERE symbol='"+crypt+"';";
            else
                sqlquery = "DELETE FROM "+table+" WHERE symbol='"+crypt+"';";
            if (!update_qry.exec(sqlquery)) ui->messages->setText("Error " + update_qry.lastError().text());
            else savesettings("balance",balance.toInt()+usd);
            update_qry.finish();
    } else {
            usd=price*amount;
            QString sqlquery;
            QSqlQuery delete_qry(db);
            sqlquery = "DELETE FROM "+table+" WHERE symbol='"+crypt+"';";
            if (!delete_qry.exec(sqlquery)) ui->messages->setText("Error " + delete_qry.lastError().text());
            else savesettings("balance",balance.toInt()+usd);
            delete_qry.finish();
        }
        ui->balance->setText(loadsettings("balance").toString());
        load_model();
    } else ui->messages->setText("Select record");
}

void stocksDialog::on_getPrice_clicked()
{
    closePrice=0;
    QString url="https://www.binance.com/api/v3/klines?symbol="+ui->crypto->currentText()+"USDT&interval=5m&limit=1&startTime="+QString::number(QDateTime::currentDateTime().addMSecs(-300000).toMSecsSinceEpoch());
    worker.get(url);
    delay(1000);
    process_dataframe("./files/"+ui->crypto->currentText()+"USDT.json");
    ui->getPrice->setText(QString::number(closePrice));
}


void stocksDialog::on_getCurrentPrices_clicked()
{
    QString sqlquery, db_symbol, db_date;
    double db_buyPrice, db_amount, totalproffit;
    QStringList modeldatalist;
    int mSecDelay = loadsettings("mSecDelay").toInt();
    if (mSecDelay==0) mSecDelay=20;
    QSqlQuery qry(db);
    sqlquery="SELECT * FROM "+table+";";
    int db_id;
    totalproffit=0;
    if (!qry.prepare(sqlquery)) {
      ui->messages->setText("prepare failed");
    }
    if (qry.exec()) {
      while (qry.next()) {
        db_id = qry.value(0).toInt();
        db_symbol = qry.value(1).toString();
        db_date = qry.value(2).toString();
        db_buyPrice = qry.value(3).toDouble();
        db_amount = qry.value(4).toDouble();
        if (!db_symbol.isEmpty()) {
            ui->messages->setText("Updating "+db_symbol);
            QString url="https://www.binance.com/api/v3/klines?symbol="+db_symbol+"USDT&interval=5m&limit=1&startTime="+QString::number(QDateTime::currentDateTime().addMSecs(-300000).toMSecsSinceEpoch());
            worker.get(url);
            delay(mSecDelay);
        }
      }
      //qDebug() << db_symbol;
      delay(1000);
    if (qry.exec()) {
      while (qry.next()) {
          db_id = qry.value(0).toInt();
          db_symbol = qry.value(1).toString();
          process_dataframe("./files/"+db_symbol+"USDT.json");
          double proffit=(closePrice-db_buyPrice)*db_amount;
          totalproffit+=proffit;
          QSqlQuery query;
          query.exec("UPDATE "+table+" SET currentPrice = "+QString::number(closePrice)+" WHERE id = "+QString::number(db_id));
      }
    }
      ui->proffit->setText("$"+QString::number(totalproffit, 'F', 3));
      ui->messages->setText("Update done");
      load_model();
    }
}

