#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "coinfilterdialog.h"
#include "settingsdialog.h"
#include <QtCore>
#include <QtGui>
#include <QtSql>


QProcess process;
QString crypt="BTC";
QString exchange,dbfile="coinhistory.db",dbtable="";
QSqlDatabase db;
int colums=10,maxcoins=0,addsec=1800;
QString appgroup="coinbrowser";
double from1h=-2,to1h=5,from24h=0,to24h=100,from7d=-2,to7d=100,btc_price=58338,markedcap_percent,volume_percent_min,volume_percent_max,price_change_from,price_change_to,volum_min,pricemin,pricemax;
bool change_1h,change_24h,change_7d,volume,marked_cap,use_volume,show_only_blacklisted,change_price,create_db=false,pricefilter,volume_min_check;


QJsonArray MainWindow::ReadJson(const QString &path)
{
    QFile file( path );
    QJsonArray jsonArray;
    if( file.open( QIODevice::ReadOnly ) )
    {
        QByteArray bytes = file.readAll();
        file.close();

        QJsonParseError parserError;
        QJsonDocument cryptolist = QJsonDocument::fromJson(bytes, &parserError);
        //qDebug() << parserError.errorString();

        QJsonObject jsonObject = cryptolist.object();
        jsonArray = jsonObject["data"].toArray();
        if ((ui->maxcoins->text() == "" || maxcoins == 0) || maxcoins > jsonArray.count()) ui->maxcoins->setText(QString::number(jsonArray.count()));
        //qDebug() << "Number of cryptocurrencies " << jsonArray.count();

    }
    return jsonArray;
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setGeometry(loadsettings("position").toRect());
    change_1h = loadsettings("change_1h").toBool();
    change_24h = loadsettings("change_24h").toBool();
    change_7d = loadsettings("change_7d").toBool();
    change_price = loadsettings("change_price").toBool();
    pricefilter = loadsettings("pricefilter").toBool();
    marked_cap = loadsettings("marked_cap").toBool();
    use_volume = loadsettings("use_volume").toBool();
    show_only_blacklisted = loadsettings("show_only_blacklisted").toBool();

    from1h = loadsettings("from1h").toDouble();
    to1h = loadsettings("to1h").toDouble();
    from24h = loadsettings("from24h").toDouble();
    to24h = loadsettings("to24h").toDouble();
    from7d = loadsettings("from7d").toDouble();
    to7d = loadsettings("to7d").toDouble();
    price_change_from = loadsettings("price_change_from").toDouble();
    price_change_to = loadsettings("price_change_to").toDouble();
    pricemin = loadsettings("pricemin").toDouble();
    pricemax = loadsettings("pricemax").toDouble();
    maxcoins = loadsettings("maxcoins").toInt();
    markedcap_percent = loadsettings("markedcap_percent").toDouble();
    volume_percent_min = loadsettings("volume_percent_min").toDouble();
    volume_percent_max = loadsettings("volume_percent_max").toDouble();
    volum_min  = loadsettings("volum_min").toDouble();
    volume_min_check  = loadsettings("volum_min_check").toBool();
    QStringList exchanges={"Binance","Bittrex","Kraken","FTX"};
    QString pwd = QDir::currentPath();
    QString json_path = loadsettings("json_path").toString();
    if (json_path == "") savesettings("json_path",pwd);
    QString cryptolistwrite = loadsettings("cryptolistwrite").toString();
    if (cryptolistwrite == "") savesettings("cryptolistwrite",pwd);
    QString cryptolistread = loadsettings("cryptolistread").toString();
    if (cryptolistread == "") savesettings("cryptolistread",pwd);
    QString reportPath = loadsettings("reportpath").toString();
    if (reportPath == "") savesettings("reportpath",pwd);
    for ( const auto& i : exchanges  ) {
      QString stake = loadsettings(i.toLower()+"_stake").toString();
      if (stake == "") {
          savesettings(i.toLower()+"_stake","USDT");
          savesettings(i.toLower()+"_stake_coin_price",1);
      }
    }
    QStringList cryptolist = loadsettings("cryptolist").toStringList();
    if (cryptolist.isEmpty()) {
        cryptolist = {"BTC","ETH","USDT"};
        savesettings("cryptolist",cryptolist);
    }
    ui->filter->setChecked(true);
    ui->comboBox->clear();
    ui->comboBox->addItems(exchanges);
    exchange = ui->comboBox->currentText();
    crypt = loadsettings(exchange.toLower()+"_stake").toString();
    dbfile="coinhistory.db";
    btc_price = loadsettings("stake_coin_price").toDouble();
    db = QSqlDatabase::addDatabase("QSQLITE"); //db start config
    //qDebug() << "db just configured";
    db.setDatabaseName(dbfile);
    if (!db.open()) {
         qDebug() << db.lastError();
    }
    tableage();
    ui->tables->setCurrentIndex(0);
    dbtable=ui->tables->currentText();
    if (dbtable=="") createdb();
    sqlmodel = new QSqlTableModel(this,db);
    initializeModel(sqlmodel);

    ui->messages->setText("");
    this->setWindowTitle("Cryptocurrency tool for Freqtrade, active stake coin "+crypt);


    QStringList modellist = initializemodel();
    //QModelIndex index;
    model = new QStandardItemModel(modellist.length()/colums,colums,this);
    connect(ui->search,SIGNAL(textEdited(const QString &)),this,SLOT(searchmodel(const QString&)));
    bool autoupdatejson=loadsettings("autoupdatejson").toBool();
    int updateinterval=loadsettings("updateinterval").toInt();
    if (updateinterval == 0) updateinterval=1;
    if (autoupdatejson) {
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(reload_model()));
        timer->start(updateinterval*60000);
    }

    reload_model();

    model->setHeaderData(0, Qt::Horizontal, "Id", Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, "Symbol", Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, "Name", Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, "Price "+crypt, Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "DB +/- %", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "Volume", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "1h change", Qt::DisplayRole);
    model->setHeaderData(7, Qt::Horizontal, "24h change", Qt::DisplayRole);
    model->setHeaderData(8, Qt::Horizontal, "7d change", Qt::DisplayRole);
    model->setHeaderData(9, Qt::Horizontal, "db/json time", Qt::DisplayRole);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index){ combo_refresh(index); });
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->sortByColumn(0,Qt::AscendingOrder);

}

MainWindow::~MainWindow()
{
    db.close();

    savesettings("position",this->geometry());
    if (ui->maxcoins->text() != "")
        savesettings("maxcoins",ui->maxcoins->text());
    else savesettings("maxcoins",QString::number(maxcoins));
    savesettings("change_1h",change_1h);
    savesettings("change_24h",change_24h);
    savesettings("change_7d",change_7d);
    savesettings("change_price",change_price);
    savesettings("pricefilter",pricefilter);

    savesettings("marked_cap",marked_cap);
    savesettings("use_volume",use_volume);
    savesettings("show_only_blacklisted",show_only_blacklisted);

    savesettings("from1h",from1h);
    savesettings("to1h",to1h);
    savesettings("from24h",from24h);
    savesettings("to24h",to24h);
    savesettings("from7d",from7d);
    savesettings("to7d",to7d);
    savesettings("price_change_from",price_change_from);
    savesettings("price_change_to",price_change_to);
    savesettings("pricemin",pricemin);
    savesettings("pricemax",pricemax);

    savesettings("markedcap_percent",markedcap_percent);
    savesettings("volume_percent_min",volume_percent_min);
    savesettings("volume_percent_max",volume_percent_max);
    savesettings("volum_min",volum_min);
    savesettings("volum_min_check",volume_min_check);
    delete ui;
}

QVariant MainWindow::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void MainWindow::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

void MainWindow::initializeModel(QSqlTableModel *sqlmodel)
{
    sqlmodel->setTable(dbtable);
    sqlmodel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    sqlmodel->select();
}

void MainWindow::tableage()
{
    int tableage = loadsettings("tableage").toInt();
    if (tableage == 0) tableage = 999;
    ui->tables->clear();
    QDateTime cdt = QDateTime::currentDateTime();
    for ( const auto& i : db.tables()  ) if (i.contains(crypt+"_")) {
        int mm,hh,dd,mo,s,u;
        u=i.indexOf("_");
        s=i.indexOf("‑");
        dd=i.mid(u+1,s-u-1).toInt();
        u=i.indexOf("_",u+1);
        mo=i.mid(s+1,u-s-1).toInt();
        s=i.indexOf("‑",s+1);
        hh=i.mid(u+1,s-u-1).toInt();
        mm=i.mid(s+1,i.length()-s-1).toInt();
        QDate datetable;
        QTime timetable;
        datetable.setDate(2021,mo,dd);
        timetable.setHMS(hh,mm,0);
        QDateTime cdttable;
        cdttable.setDate(datetable);
        cdttable.setTime(timetable);
        //qDebug() << cdt.addSecs(-tableage*3600) << " " << cdttable;
        if (cdt.addSecs(-tableage*3600) < cdttable) {

            ui->tables->addItem(i);
        }
    }
    ui->tables->setCurrentIndex(0);
}

void MainWindow::createdb()
{
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();
    dbtable=crypt+"_"+QString::number(cd.day())+"‑"+QString::number(cd.month())+"_"+QString::number(ct.hour())+"‑"+QString::number(ct.minute()).rightJustified(2,'0');
    ui->tables->addItem(dbtable);
    QString createTables = "create table "+dbtable+"(id integer primary key, "
          "symbol varchar(7), "
          "name varchar(30), "
          "price real, "
          "volume_24h real, "
          "percent_change_1h real, "
          "percent_change_24h real, "
          "percent_change_7d real, "
          "market_cap real, "
          "last_updated varchar(25))";
    qDebug() << "Creating database, please wait...";
    if (!db.open()) qDebug() << "Error " << db.lastError().text();
    QSqlQuery qry(db);
    if (!qry.exec(createTables)) ui->label->setText("Error "+qry.lastError().text());
    qry.finish();
    create_db=true;
}

QStringList MainWindow::readpairs()
{
    QFile filein;
    QString content,outstring;
    QStringList pairs,contentlist,blacklist={"SUSD","USD","EUR","USDC","BUSD","GBP","BNB","TUSD","UST"};
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    QStringList blacklist_exchange = appsettings.value(exchange.toLower()+"_blacklist").value<QStringList>();
    appsettings.endGroup();
    QStringList bittrex_blacklist = loadsettings("bittrex_blacklist").toStringList();
    bool blackl=false,inrank=true;
    int s=10,counter=0,added=0;
    QString cryptolistread = loadsettings("cryptolistread").toString();

    filein.setFileName(cryptolistread+"/raw_"+exchange.toLower()+".txt");
    if (filein.open(QIODevice::ReadOnly))
    {
       QTextStream in(&filein);

       while (!in.atEnd())
       {
          content = in.readLine();

          int middle = content.indexOf("/",s);
          int start = content.lastIndexOf(" ",middle);
          int end = content.indexOf(" ",middle);
          QString par1 = content.mid(start+1,middle-start-1);
          QString par2 = content.mid(middle+1,end-middle-1);
          for ( const auto& i : blacklist  )
            if (i == par1) blackl = true;

          for ( const auto& i : blacklist_exchange  ) //Bittrex blacklist
            if (i == par1 && !show_only_blacklisted) blackl = true;

          if (crypt == par1) blackl = true;
          if (inrank && !blackl  && crypt == par2 ) {
              outstring = content.mid(start+1,end-start-1).trimmed();
              pairs.append(par1);

              added++;
          }
          counter=0;
          blackl = false;
       }
       filein.close();
       ui->messages->setText("Pairs found for " + exchange + " " + QString::number(added));
       //qDebug() << "Coins Added " << added;

    } else {
        ui->messages->setText(cryptolistread+"/raw_"+exchange.toLower()+".txt not found, create with freqtrade list-pairs --exchange "+exchange.toLower()+" > raw_"+exchange.toLower()+".txt");
        //qDebug() << "Error " << filein.errorString();
    }

    return pairs;
}


QStringList MainWindow::initializemodel()
{
        bool weekplus=false,dayplus=false,hourplus=false,volumeok=false,inrank=false,marked_cap_ok=false,priceplus=false,priceok=false,volummin=false;


        double percent,price_change;
        int db_id,coincounts=0,coininlist=0,unique=0;
        QDate cd = QDate::currentDate();
        QDateTime ct = QDateTime::currentDateTime();
        QSqlRecord record;
        QFile csv_file;
        QString db_symbol,db_name, symbol,db_last_updated,sqlquery="";
        double db_volume_24h=0,db_percent_change_1h,db_market_cap=0,db_price=0;
        int json_year,json_mo,json_date,json_h, db_year,db_mo,db_date,db_h,db_min;
        //ui->messages->setText("Please wait.....");
        bool report=loadsettings("report").toBool();
        QString csv_string="Name,Volume new,Volume old,1h change,1h change old,Last updated, Old last updated";
        if (report) {
            QString csv_filename="report_"+exchange+"_"+cd.toString()+"_"+ct.toString()+".csv";
            QString reportPath = loadsettings("reportpath").toString();
            csv_file.setFileName(reportPath+"/"+csv_filename);
            csv_file.open(QIODevice::WriteOnly | QIODevice::Text);
        }
        QTextStream outStream(&csv_file);
        if (report) outStream << csv_string+"\n";
        QStringList pairs=readpairs(),modeldatalist;
        QString stake=crypt;
        if (stake.contains("USD")) stake="USD";
        QString path = loadsettings("json_path").toString();
        if (path =="") path="./crypto_"+stake+".json";
        int rowsintable = loadsettings("rowsintable").toInt(), top_1h=0, bottom_1h=0;
        QString apikey = loadsettings("apikey").toString(), top_symbol="",bottom_symbol="";
        bool autoupdatejson=loadsettings("autoupdatejson").toBool();
        int autojsonmin = loadsettings("autojsonmin").toInt();
        QFileInfo jsonfileinf(path+"/crypto_"+crypt+".json");
        //QFileInfo dbfileinf(dbfile);
        QDateTime jsondt = jsonfileinf.lastModified().addSecs(autojsonmin*60);
        if (!jsondt.isValid()) jsondt =QDateTime::currentDateTime();
        //QDateTime dbtime = dbfileinf.lastModified().addSecs(dbtimediffrance*60);
        //if (!dbtime.isValid()) dbtime =QDateTime::currentDateTime();
        //QDateTime dbtime = jsonfileinf.lastModified().addSecs(dbtimediffrance-autojsonmin*60);
        //dbtable=crypt+"_coins";
        dbtable=ui->tables->currentText();
        if ((ct >= jsondt && autoupdatejson) || ui->actionUpdateJson->isChecked()) {
            create_db=true;
            if (!db.open()) qDebug() << "Error " << db.lastError().text();
            ui->actionUpdateDB->setChecked(false);
            //sqlquery = "DROP TABLE "+dbtable+";";
            //QSqlQuery insert_qry(db);
            //if (!insert_qry.exec(sqlquery)) ui->label->setText("Error "+insert_qry.lastError().text());
            //insert_qry.finish();
            createdb();
        }

        btc_price = loadsettings(exchange.toLower()+"_stake_coin_price").toDouble();
        if (ui->maxcoins->text() == "") ui->maxcoins->setText(QString::number(maxcoins));
        else maxcoins = ui->maxcoins->text().toInt();

        QJsonArray jsonArray = ReadJson(path+"/crypto_"+crypt+".json");

        foreach (const QJsonValue & value, jsonArray) {

                QJsonObject data = value.toObject();
                int id = data["cmc_rank"].toInt();
                QString lastsymbol=symbol;
                symbol = data["symbol"].toString();
                QString name = data["name"].toString();
                QJsonObject quote = value["quote"].toObject();
                QJsonObject coin = quote[stake].toObject();
                double price = coin["price"].toDouble();
                double volume_24h = coin["volume_24h"].toDouble();
                double percent_change_1h = coin["percent_change_1h"].toDouble();
                double percent_change_24h = coin["percent_change_24h"].toDouble();
                double percent_change_7d = coin["percent_change_7d"].toDouble();
                double percent_change_30d = coin["percent_change_30d"].toDouble();
                double percent_change_60d = coin["percent_change_60d"].toDouble();
                double percent_change_90d = coin["percent_change_90d"].toDouble();
                QString last_updated = coin["last_updated"].toString();

                double market_cap = coin["market_cap"].toDouble();
                if (!db.open()) qDebug() << "Error " << db.lastError().text();

                QSqlQuery qry(db);
                sqlquery = "SELECT * FROM "+dbtable+" WHERE name=:name;";
                if (!qry.prepare(sqlquery)) {
                  qDebug() << "prepare failed\n";
                }
                qry.bindValue(":name",name);

                if (qry.exec()) {
                  if (qry.next()) {
                    db_id = qry.value(0).toInt();
                    db_symbol = qry.value(1).toString();
                    db_name = qry.value(2).toString();
                    db_price = qry.value(3).toDouble();
                    db_volume_24h = qry.value(4).toDouble();
                    db_percent_change_1h = qry.value(5).toDouble();
                    db_market_cap = qry.value(8).toDouble();
                    db_last_updated = qry.value(9).toString();

                  }
                }
                //if (symbol != db_symbol) qDebug() << symbol << " " << db_symbol;
                QString last_updated_time = db_last_updated.mid(11,5)+"/"+last_updated.mid(11,5);
                json_year = last_updated.mid(0,4).toInt();
                json_mo = last_updated.mid(5,2).toInt();
                json_date = last_updated.mid(8,2).toInt();
                json_h = last_updated.mid(11,2).toInt();
                //json_min = last_updated.mid(14,2).toInt();

                db_year = db_last_updated.mid(0,4).toInt();
                db_mo = db_last_updated.mid(5,2).toInt();
                db_date = db_last_updated.mid(8,2).toInt();
                db_h = db_last_updated.mid(11,2).toInt();
                db_min = db_last_updated.mid(14,2).toInt();


                QDate json_d(json_year,json_mo,json_date);
                QDate db_d(db_year,db_mo,db_date);
                QDateTime lastdbupdate(db_d,QTime(db_h,db_min));

                //double startprice = btc_price*db_price;
                //double endprice = btc_price*price;
                //price_change = endprice-startprice;
                //price_change = price_change/startprice*100;
                //price_change = 100/db_price*btc_price*(price-db_price);
                //price_change = 100/startprice;
                //price_change = price_change*(endprice-startprice);
                price_change=(price/db_price*100)-100;
                if (db_last_updated == last_updated || symbol != db_symbol) price_change=0;
                //qDebug() << db_price << " " << price;
                if (top_1h<percent_change_1h) {
                    top_1h = percent_change_1h;
                    top_symbol = symbol;
                }
                if (bottom_1h>percent_change_1h) {
                    bottom_1h = percent_change_1h;
                    bottom_symbol = symbol;
                }
                if ((json_date > db_date || (json_date == db_date && json_h >= db_h+10)) && symbol == "ETH") ui->messages->setText("DB is over 10h old! From " +QString::number(db_date,'g',2)+"/"+QString::number(db_mo)+", time "+QString::number(db_h)+":"+QString::number(db_min));

                if (!db.open())
                     qDebug() << db.lastError();

                if (create_db) {

                    QSqlQuery insert_qry(db);
                    sqlquery = "INSERT INTO "+dbtable+" (id,symbol,name,price,volume_24h,percent_change_1h,percent_change_24h,percent_change_7d,market_cap,last_updated) VALUES ("+QString::number(id)+", '"+symbol+"', '"+name+"', "+QString::number(price)+", "+QString::number(volume_24h)+", "+QString::number(percent_change_1h)+", "+QString::number(percent_change_24h)+", "+QString::number(percent_change_7d)+", "+QString::number(market_cap)+", '"+last_updated+"');";
                    //qDebug() << sqlquery;
                    if (!insert_qry.exec(sqlquery)) qDebug() << "Error " << insert_qry.lastError().text();
                    insert_qry.finish();
                }

                percent = (volume_24h/db_volume_24h)*100;
                if (crypt.contains("USD")) volume_24h = volume_24h/1000000;
                if ( db_market_cap < market_cap && marked_cap) marked_cap_ok = true;
                else if (!marked_cap) marked_cap_ok = true;

                if (percent >= volume_percent_min && percent < volume_percent_max && use_volume) volumeok=true;
                else if (!use_volume) volumeok=true;
                if (volum_min > volume_24h && volume_min_check) volummin=true;
                else if (!volume_min_check) volummin=true;
                if (price_change < price_change_to && price_change > price_change_from && change_price) priceplus=true;
                else if (!change_price) priceplus=true;
                if (percent_change_1h < to1h && percent_change_1h > from1h && change_1h) hourplus=true;
                else if (!change_1h) hourplus=true;
                if (percent_change_24h < to24h && percent_change_24h > from24h && change_24h) dayplus=true;
                else if (!change_24h) dayplus=true;
                if (percent_change_7d < to7d && percent_change_7d > from7d && change_7d) weekplus=true;
                else if (!change_7d) weekplus=true;
                if (price < pricemax && price > pricemin && pricefilter) priceok=true;
                else if (!pricefilter) priceok=true;
                //if (percent_change_7d < 0) weekplus=true;
                //weekplus=true;

                for ( const auto& i : pairs  )
                {
                    /*if (i == symbol && inrank) { // taka ut tvøfalda
                        inrank = false;
                        qDebug() << symbol;
                    }
                    else*/
                        if (i == symbol) inrank = true;
                    if (i == lastsymbol) unique++;

                 }

                for ( const auto& i : pairs  ) //Write to tableview
                {
                    if ((priceok && weekplus && dayplus && hourplus && volumeok && volummin && i==symbol && inrank && marked_cap_ok && unique<2 && priceplus) || (!ui->filter->isChecked() && i==symbol && unique<2)) {
                        modeldatalist << QString::number(id) << symbol << name << QString::number(price, 'F', 3) << QString::number(price_change, 'F', 2) << QString::number(volume_24h, 'F', 2) << QString::number(percent_change_1h, 'F', 2) << QString::number(percent_change_24h, 'F', 2) << QString::number(percent_change_7d, 'F', 2) << last_updated_time;
                        if (report) {
                            csv_string=name+","+QString::number(volume_24h)+","+QString::number(db_volume_24h)+","+QString::number(percent_change_1h)+","+QString::number(db_percent_change_1h)+","+last_updated+","+db_last_updated+","+cd.toString()+","+ct.toString();
                            outStream << csv_string+"\n";
                        }
                        coininlist++;

                    }
               }
                volumeok = false;
                volummin = false;
                weekplus = false;
                hourplus = false;
                dayplus = false;
                priceplus = false;
                priceok = false;
                unique = 0;
                marked_cap_ok = false;
                coincounts++;
                if ((maxcoins <= coincounts && !ui->actionUpdateJson->isChecked()) || (ui->actionUpdateDB->isChecked() && rowsintable <= coincounts)) break;


        }
        if (jsonArray.count() == 0) ui->messages->setText("Json file not found, please update json file");
        if (pairs.count() > 0 && jsonArray.count() > 0) ui->messages->setText("Database is from "+QString::number(db_date)+"/"+QString::number(db_mo)+", time "+QString::number(db_h,'G',2)+":"+QString::number(db_min).rightJustified(2,'0'));
        csv_file.close();
        create_db = false;
        if (((ct >= jsondt && autoupdatejson) || ui->actionUpdateJson->isChecked()) && !apikey.isEmpty()) {
            QString path = loadsettings("json_path").toString();
            if (path == "") path = ".";
            QString crypto=crypt;
            ui->actionUpdateJson->setChecked(false);
            if (crypto.contains("USD")) crypto="USD";
            QStringList commandlist;
            commandlist.append("-H");
            commandlist.append("X-CMC_PRO_API_KEY: "+apikey); //");
            commandlist.append("-H");
            commandlist.append("Accept: application/json");
            commandlist.append("-d");
            commandlist.append("start=1&limit=5000&convert="+crypto);
            commandlist.append("-G");
            commandlist.append("https://pro-api.coinmarketcap.com/v1/cryptocurrency/listings/latest");
            QProcess *myProcess = new QProcess(this);
            myProcess->setStandardOutputFile(path+"/crypto_"+crypt+".json");
            myProcess->start("curl",commandlist);
            if (myProcess->exitCode() > 0) ui->messages->setText("Error executing cURL : " + myProcess->errorString() + " Exitcode: " + QString::number(myProcess->exitCode()));
            myProcess->waitForFinished(-1);
        } else if (apikey.isEmpty()) ui->messages->setText("API key missing, please insert API key in settings.");
        if (pairs.count() > 0 && jsonArray.count() > 0 && !apikey.isEmpty()) ui->messages->setText(ui->messages->text()+", Found and added to list "+QString::number(coininlist) + " / Winner: " + top_symbol + "  " + QString::number(top_1h) + "% / Looser: " + bottom_symbol + "  " + QString::number(bottom_1h)+"%");
        return modeldatalist;
}



void MainWindow::combo_refresh(int comboindex)
{
    exchange = ui->comboBox->currentText();
    crypt = loadsettings(exchange.toLower() + "_stake").toString();
    ui->tables->clear();
    for ( const auto& i : db.tables()  ) if (i.contains(crypt+"_")) ui->tables->addItem(i);
    ui->tables->setCurrentIndex(ui->tables->count()-1);
    dbtable=ui->tables->currentText();
    if (dbtable=="") createdb();
    this->setWindowTitle("Cryptocurrency tool for Freqtrade, active stake coin "+crypt);
    //qDebug() << "ComboRefresh";
    reload_model();
}

void MainWindow::on_pushButton_clicked()
{
    QFile fileout;
    int row=0,col=1,rows=model->rowCount();
    QTextStream out(&fileout);
    QStringList pairs;
    bool doub=false;
    QModelIndex index=model->index(row,col,QModelIndex());
    QString cryptolistwrite = loadsettings("cryptolistwrite").toString();
    if (cryptolistwrite == "") cryptolistwrite="./";
    else cryptolistwrite = cryptolistwrite+"/";
    fileout.setFileName(cryptolistwrite+crypt.toLower()+"_"+exchange.toLower()+".txt");

    if (!fileout.open(QIODevice::WriteOnly | QIODevice::Text))
        qDebug() << "Error opening file!";

    while (model->data(index.sibling(index.row(),col)).toString() != "")
    {
        for ( const auto& i : pairs  ) if (i==model->data(index.sibling(index.row(),col)).toString()) doub = true;
        pairs << model->data(index.sibling(index.row(),col)).toString();

        if (row==rows-1 && !doub)
            out << "\t\t\t\"" << model->data(index.sibling(index.row(),col)).toString() << "/" << crypt <<"\"\n";
        else if (!doub)
            out << "\t\t\t\"" << model->data(index.sibling(index.row(),col)).toString() << "/" << crypt <<"\",\n";
        row++;
        doub=false;
        index=model->index(row,0,QModelIndex());
    }
    ui->messages->setText("Pairs written to file -> "+cryptolistwrite+crypt.toLower()+"_"+exchange.toLower()+".txt");
    fileout.close();
}

void MainWindow::on_filter_clicked()
{
        exchange = ui->comboBox->currentText();
        //qDebug() << "Filter reload model";
        reload_model();

}

void MainWindow::searchmodel(const QString& text)
{
    if(text.isEmpty())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selection().indexes();
    QModelIndexList matchList;
    int d_col=1;
    if (!indexes.isEmpty())
        matchList = ui->tableView->model()->match(ui->tableView->model()->index(0,ui->tableView->selectionModel()->selection().indexes().at(0).column()), Qt::EditRole, text, -1,  Qt::MatchFlags(Qt::MatchContains|Qt::MatchWrap));
    else
        matchList = ui->tableView->model()->match(ui->tableView->model()->index(0,d_col), Qt::EditRole, text, -1,  Qt::MatchFlags(Qt::MatchContains|Qt::MatchWrap));

    if(matchList.count()>=1){
        ui->tableView->setCurrentIndex(matchList.first());
        ui->tableView->scrollTo(matchList.first());
    }
}


void MainWindow::reload_model()
{
    int row=0,i=0,col;
    QModelIndex index;
    QStringList modellist = initializemodel();
    model->setRowCount(modellist.length()/colums);
    while (i < modellist.length()-1) {
       for (col=0;col<colums;col++) {
         index=model->index(row,col,QModelIndex());
         if (col == 0) model->setData(index,modellist[i].toInt());
         if (col < 4 & col > 0) model->setData(index,modellist[i]);
         if (col < 10 & col > 2) {
             model->setData(index,modellist[i]);
             model->setData(index, Qt::AlignRight, Qt::TextAlignmentRole);
         }
         i++;
        }
      row++;
    }
}


void MainWindow::on_pushButton_2_clicked()
{

    coinfilterDialog coinfilter;
    QObject::connect(&coinfilter, SIGNAL(destroyed()), this, SLOT(reload_model()));
    coinfilter.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    coinfilter.exec();
}


void calc_profit()
{
    //Adds daily profit to totalsum and calculate a new day.
    int i;
    float dailyprofit=24,percent,start=200;
    for (i=1;i<=20;i++) {
        percent = (dailyprofit/start)*100;
        start += dailyprofit;
        qDebug() << "Percent " << percent << "\n";
        dailyprofit = (start * (1 + (percent/100))) - start;
        //dailyprofit = (dailyprofit/percent)+dailyprofit;
        qDebug() << "Day: " << i << " - Daily: " << dailyprofit << " Total: " << start << "\n";
        //start = start+dailyprofit;
    }
    //((490−350) ÷350)×100
}

void MainWindow::on_pushButton_3_clicked()
{
    settingsDialog settingsdialog;
    QObject::connect(&settingsdialog, SIGNAL(destroyed()), this, SLOT(reload_model()));
    settingsdialog.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    settingsdialog.exec();
    ui->filter->setChecked(false);
    tableage();
    this->setWindowTitle("Cryptocurrency tool for Freqtrade, active stake coin "+crypt);
}

void MainWindow::on_actionUpdateDB_changed()
{
    ui->messages->setText("Database updating...Please wait.");
    ui->messages->show();
    reload_model();
    ui->messages->setText("Database updated.");
    reload_model();
}

void MainWindow::on_actionUpdateJson_changed()
{
    ui->messages->setText("Json updating...Please wait.");
    ui->messages->show();
    reload_model();
    ui->messages->setText("Json updated.");
    reload_model();
}

void MainWindow::on_tables_activated(int index)
{
    dbtable=ui->tables->currentText();
    initializeModel(sqlmodel);
    reload_model();
}
