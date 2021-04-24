#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "coinfilterdialog.h"
#include "settingsdialog.h"
#include <QtCore>
#include <QtGui>
#include <QtSql>


QProcess process;
QString crypt="BTC";
QString exchange,dbfile="coinhistory.db",dbtable=crypt+"_coins";
QSqlDatabase db;
int colums=9,maxcoins=0,addsec=1800;
QString appgroup="coinbrowser",profile;
double from1h=-2,to1h=5,from24h=0,to24h=100,from7d=-2,to7d=100,btc_price=58338,markedcap_percent,volume_percent,price_change_from,price_change_to,volum_min,pricemin,pricemax;
bool change_1h,change_24h,change_7d,volume,marked_cap,use_volume,show_only_blacklisted,change_price,create_db=false,pricefilter;


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
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup("General");
    profile = appsettings.value("profile").toString();
    if (profile == "") {
        profile = appgroup;
        appsettings.setValue("profile",QVariant::fromValue(profile));
    }
    appsettings.endGroup();
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
    volume_percent = loadsettings("volume_percent").toDouble();
    volum_min  = loadsettings("volum_min").toDouble();
    crypt = loadsettings("crypt").toString();
    dbfile="coinhistory.db";
    dbtable=crypt+"_coins";
    btc_price = loadsettings("stake_coin_price").toDouble();
    db = QSqlDatabase::addDatabase("QSQLITE"); //db start config
    //qDebug() << "db just configured";
    db.setDatabaseName(dbfile);
    if (!db.open()) {
         qDebug() << db.lastError();
    } else if ( !db.tables().contains( QString(dbtable) ))  createdb();
    sqlmodel = new QSqlTableModel(this,db);
    initializeModel(sqlmodel);
    QStringList exchanges={"Binance","Bittrex","Kraken"};
    ui->filter->setChecked(true);
    ui->comboBox->clear();
    ui->comboBox->addItems(exchanges);
    ui->messages->setText("");
    this->setWindowTitle("Cryptocurrency tool for Freqtrade, active stake coin "+crypt);
    exchange = ui->comboBox->currentText();
    QStringList modellist = initializemodel();
    //QModelIndex index;
    model = new QStandardItemModel(modellist.length()/colums,colums,this);
    connect(ui->search,SIGNAL(textEdited(const QString &)),this,SLOT(searchmodel(const QString&)));
    //qDebug() << "initial reload model";
    reload_model();

    model->setHeaderData(0, Qt::Horizontal, "Id", Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, "Symbol", Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, "Name", Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, "100$ +/-", Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "Volume", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "1h change", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "24h change", Qt::DisplayRole);
    model->setHeaderData(7, Qt::Horizontal, "7d change", Qt::DisplayRole);
    model->setHeaderData(8, Qt::Horizontal, "db/json time", Qt::DisplayRole);
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index){ combo_refresh(index); });
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->sortByColumn(1,Qt::AscendingOrder);
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
    savesettings("volume_percent",volume_percent);
    savesettings("volum_min",volum_min);
    delete ui;
}

QVariant MainWindow::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(profile);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void MainWindow::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(profile);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

void MainWindow::initializeModel(QSqlTableModel *sqlmodel)
{
    sqlmodel->setTable(dbtable);
    sqlmodel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    sqlmodel->select();
}

void MainWindow::createdb()
{
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
     appsettings.beginGroup(profile);
    QStringList blacklist_binance = appsettings.value("binance_blacklist").value<QStringList>();
    QStringList blacklist_bittrex = appsettings.value("bittrex_blacklist").value<QStringList>();
     appsettings.endGroup();
    QStringList bittrex_blacklist = loadsettings("bittrex_blacklist").toStringList();
    bool blackl=false,inrank=true;
    int s=10,counter=0,added=0;
    QString cryptolistread = loadsettings("cryptolistread").toString();

    QString rawfilepath=cryptolistread+"/"+crypt.toLower()+"_raw_"+exchange.toLower()+".txt";
    filein.setFileName(cryptolistread+"/"+crypt.toLower()+"_raw_"+exchange.toLower()+".txt");
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

          for ( const auto& i : blacklist_bittrex  ) //Bittrex blacklist
            if (i == par1 && exchange == "Bittrex" && !show_only_blacklisted) blackl = true;

          for ( const auto& i : blacklist_binance  ) //Binance blacklist
            if (i == par1 && exchange == "Binance" && !show_only_blacklisted) blackl = true;
          for ( const auto& i : pairs  )
          {
              counter++;
              if (i == par1) inrank = false;

           }

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
        qDebug() << "Error " << filein.errorString();
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
        double db_volume_24h,db_percent_change_1h,db_market_cap,db_price;
        int json_year,json_mo,json_date,json_h,json_min, db_year,db_mo,db_date,db_h,db_min;
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

        QString apikey = loadsettings("apikey").toString();
        bool autoupdatejson=loadsettings("autoupdatejson").toBool();
        int autojsonmin = loadsettings("autojsonmin").toInt();
        QFileInfo jsonfileinf(path+"/crypto_"+crypt+".json");
        QDateTime jsondt = jsonfileinf.lastModified().addSecs(autojsonmin*60);
        //jsondt.time() = jsondt.time().addSecs(autojsonmin*60);

        if (ui->updatedb->isChecked() || (ct > jsondt && autoupdatejson)) {
            create_db=true;
            ui->updatedb->setChecked(false);
            sqlquery = "DROP TABLE "+dbtable+";";
            QSqlQuery insert_qry(db);
            if (!insert_qry.exec(sqlquery)) ui->label->setText("Error "+insert_qry.lastError().text());
            createdb();
        }


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
                  while (qry.next()) {
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
                QString last_updated_time = db_last_updated.mid(11,5)+"/"+last_updated.mid(11,5);
                json_year = last_updated.mid(0,4).toInt();
                json_mo = last_updated.mid(5,2).toInt();
                json_date = last_updated.mid(8,2).toInt();
                json_h = last_updated.mid(11,2).toInt();
                json_min = last_updated.mid(14,2).toInt();

                db_year = db_last_updated.mid(0,4).toInt();
                db_mo = db_last_updated.mid(5,2).toInt();
                db_date = db_last_updated.mid(8,2).toInt();
                db_h = db_last_updated.mid(11,2).toInt();
                db_min = db_last_updated.mid(14,2).toInt();


                QDate json_d(json_year,json_mo,json_date);
                QDate db_d(db_year,db_mo,db_date);


                //double startprice = ((100/btc_price)/db_price);
                double startprice = btc_price*db_price;
                double endprice = btc_price*price;
                price_change = endprice-startprice;
                price_change = price_change/startprice*100;
                price_change = 100/db_price*btc_price*(price-db_price);
                price_change = 100/startprice;
                price_change = price_change*(endprice-startprice);
                if (price_change < -100 || price_change > 100) price_change=0;
                //qDebug() << db_price << " " << price;

                if ((json_date > db_date || (json_date == db_date && json_h >= db_h+10)) && symbol == "ETH") ui->messages->setText("DB is over 10h old! From " +QString::number(db_date)+"/"+QString::number(db_mo)+", time "+QString::number(db_h)+":"+QString::number(db_min));
                ui->messages->setText("Database is from "+QString::number(db_date)+"/"+QString::number(db_mo)+", time "+QString::number(db_h,'G',2)+":"+QString::number(db_min));
                /*if (ui->updatedb->isChecked() && 4==5)
                {
                    QSqlQuery update_qry(db);

                    sqlquery = "UPDATE "+dbtable+" SET id=:id, symbol=:symbol, name=:name, price=:price, volume_24h=:volume_24h, percent_change_1h=:percent_change_1h,"
                    " percent_change_24h=:percent_change_24h, percent_change_7d=:percent_change_7d, market_cap=:market_cap, last_updated=:last_updated WHERE name=:name;";
                    if (!update_qry.prepare(sqlquery))
                      qDebug() << "prepare failed " << update_qry.lastError();
                    update_qry.bindValue(":id",id);
                    update_qry.bindValue(":symbol",symbol);
                    update_qry.bindValue(":name",name);
                    update_qry.bindValue(":price",price);
                    update_qry.bindValue(":volume_24h",volume_24h);
                    update_qry.bindValue(":percent_change_1h",percent_change_1h);
                    update_qry.bindValue(":percent_change_24h",percent_change_24h);
                    update_qry.bindValue(":percent_change_7d",percent_change_7d);
                    update_qry.bindValue(":market_cap",percent_change_7d);
                    update_qry.bindValue(":last_updated",last_updated);
                    update_qry.exec();

                }*/


               if (!db.open())
                     qDebug() << db.lastError();

                //sqlquery = "SELECT FROM "+dbtable+" WHERE ID="+QString::number(id)+";";
                //QSqlQuery insert_qry(db);
                /*if (!insert_qry.exec(sqlquery)) ui->label->setText("Error "+insert_qry.lastError().text());
                insert_qry.finish();*/

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

                if (percent >= volume_percent && use_volume) volumeok=true;
                else if (!use_volume) volumeok=true;
                if (volum_min < volume_24h) volummin=true;
                //else if (!pricefilter) volummin=true;
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

                        modeldatalist << QString::number(id) << symbol << name << QString::number(price_change) << QString::number(volume_24h, 'g', 7) << QString::number(percent_change_1h) << QString::number(percent_change_24h) << QString::number(percent_change_7d) << last_updated_time;
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
                if (maxcoins < coincounts && !ui->updatedb->isChecked()) break;


        }
        csv_file.close();
        create_db = false;
        if (ct > jsondt && autoupdatejson) {
            QString path = loadsettings("json_path").toString();
            if (path == "") path = ".";
            QString crypto=crypt;
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
            myProcess->waitForFinished(-1);
        }
        ui->messages->setText(ui->messages->text()+", Found and added to list "+QString::number(coininlist));
        return modeldatalist;
}



void MainWindow::combo_refresh(int comboindex)
{
    exchange = ui->comboBox->currentText();
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
    //qDebug() << "reload_model";
    while (i < modellist.length()-1) {
       for (col=0;col<colums;col++) {
         index=model->index(row,col,QModelIndex());
         if (col == 0) model->setData(index,modellist[i].toInt());
         if (col < 3 & col > 0) model->setData(index,modellist[i]);
         if (col < 8 & col > 2) model->setData(index,modellist[i].toDouble());
         if (col == 8) model->setData(index,modellist[i]);
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
}

void MainWindow::on_updatedb_clicked()
{
    ui->messages->setText("Database updating...Please wait.");
    ui->messages->show();
    reload_model();
    ui->messages->setText("Database updated.");
}
