#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "coinfilterdialog.h"
#include "settingsdialog.h"
#include "stocksdialog.h"
#include "stocks.h"
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QTimer>
#include <QString>
#include <QIODevice>

#include <QSqlRecord>

#include <QMessageBox>
#include <QGuiApplication>
#include <QAbstractButton>
#include <QPushButton>


QProcess process;
QString crypt="BTC";
QString exchange,dbfile="coinhistory.db",dbtable="";
QDateTime timeroff;
QSqlDatabase db;
QTimer *timer;
int colums=11,coin_from=1,coin_to=1,addsec=1800,cycles=0;
QString appgroup="coinbrowser";
double from1h=-2,to1h=5,from24h=0,to24h=100,from7d=-2,to7d=100,btc_price=58338,markedcap_percent,volume_percent_min,volume_percent_max,price_change_from,price_change_to,volum_min,pricemin,pricemax;
bool change_1h,change_24h,change_7d,volume,marked_cap,use_volume,show_only_blacklisted,change_price,create_db=false,pricefilter,volume_min_check ,timeupdatedb=false;


int stakedBalance(int stored)
{
    if (stored > 0) return stored;
    QSqlQuery held(db);
    int rows = 0;
    // A missing table means a simulator never used, which is exactly the case
    // that wants staking - exec fails, rows stays 0, and 1000 comes back.
    if (held.exec("SELECT COUNT(*) FROM \"cryptoInvest\"") && held.next())
        rows = held.value(0).toInt();
    return rows == 0 ? 1000 : stored;
}

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
        // Clamping the range to the data must not look like the user moving
        // the spinbox: valueChanged now triggers a reload, and a reload is
        // what called this, so an unblocked setValue here recurses forever.
        if ((ui->cointo->value() == 0 || coin_to == 0) || coin_to > jsonArray.count()) {
            QSignalBlocker block(ui->cointo);
            ui->cointo->setValue(jsonArray.count());
        }
        //qDebug() << "Number of cryptocurrencies " << jsonArray.count();

    }
    return jsonArray;
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //calc_profit();
    timer = new QTimer(this);
    manager = new QNetworkAccessManager(this);
    // Connected here, not after the first initializemodel() call below. That
    // call can now ask the exchange for prices, and a reply that arrives with
    // nothing connected to it is simply lost - leaving the window empty with
    // no way to notice why.
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    bool autoupdatejson=loadsettings("autoupdatejson").toBool();
    int autojsonmin=loadsettings("autojsonmin").toInt();
    if (autojsonmin == 0) autojsonmin=60;
    //qDebug() << autojsonmin << " " << autoupdatejson << " " << timer->interval() << " " << timer->isActive() << " " << timer->isSingleShot() << " " << timer->interval();
    if (!timer->isActive()) {
        connect(timer, SIGNAL(timeout()), this, SLOT(reload_model()));
        if (autoupdatejson) timer->start(autojsonmin*60000);
    }
    QTime ct=QTime::currentTime().addMSecs(timer->remainingTime());
    timeroff=QDateTime::currentDateTime().addMSecs(timer->remainingTime());
    //qDebug() << autojsonmin << " " << autoupdatejson << " " << timer->interval() << " " << timer->isActive() << " " << timer->isSingleShot() << " " << ct.toString();
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
    coin_from = loadsettings("coinfrom").toInt();
    coin_to = loadsettings("cointo").toInt();
    ui->cointo->setValue(coin_to);
    ui->coinfrom->setValue(coin_from);
    // Nothing was connected to these, so changing the range did nothing until
    // something else happened to reload the model. keyboardTracking(false)
    // means a typed number arrives once, on Enter or focus out, instead of
    // reloading after every digit - typing "20" would otherwise reload for 2
    // and again for 20.
    ui->cointo->setKeyboardTracking(false);
    ui->coinfrom->setKeyboardTracking(false);
    markedcap_percent = loadsettings("markedcap_percent").toDouble();
    volume_percent_min = loadsettings("volume_percent_min").toDouble();
    volume_percent_max = loadsettings("volume_percent_max").toDouble();
    volum_min  = loadsettings("volum_min").toDouble();
    volume_min_check  = loadsettings("volum_min_check").toBool();
    QStringList exchanges={"Binance","Bittrex","Kraken","FTX","Kucoin","GateIO"};
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
    // Write-ahead logging, and only sync at checkpoints rather than on every
    // commit. Costs nothing here - the database is a local cache that can be
    // rebuilt from the exchange at any time - and takes a large bite out of
    // the time an update spends waiting on the disk.
    {
        QSqlQuery pragma(db);
        pragma.exec("PRAGMA journal_mode=WAL");
        pragma.exec("PRAGMA synchronous=NORMAL");
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
    // Connected after model exists, because reload_model writes into it.
    connect(ui->cointo, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [=](int){ reload_model(); });
    connect(ui->coinfrom, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [=](int){ reload_model(); });
    connect(ui->search,SIGNAL(textEdited(const QString &)),this,SLOT(searchmodel(const QString&)));
    reload_model();

    // The simulator needs a list of coins, which until now meant pressing the
    // write-to-file button and then pointing Settings at whatever it produced.
    // If no file is configured, point at the one that button writes and write
    // it, so the simulator has something to open on a first run.
    if (loadsettings("cryptoInvestFile").toString().isEmpty()) {
        const QString f = pairFilePath();
        if (QFile::exists(f) || writePairFile(f)) savesettings("cryptoInvestFile",f);
    }

    model->setHeaderData(0, Qt::Horizontal, "Rank", Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, "Symbol", Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, "Name", Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, "Price "+crypt, Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "DB +/- %", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "Volume", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "1h change", Qt::DisplayRole);
    model->setHeaderData(7, Qt::Horizontal, "24h change", Qt::DisplayRole);
    model->setHeaderData(8, Qt::Horizontal, "7d change", Qt::DisplayRole);
    model->setHeaderData(9, Qt::Horizontal, "DB rank move", Qt::DisplayRole);
    model->setHeaderData(10, Qt::Horizontal, "db/json time", Qt::DisplayRole);
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
    savesettings("cointo",coin_to);
    savesettings("coinfrom",coin_from);
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
    QDate cd = QDate::currentDate();
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
        datetable.setDate(cd.year(),mo,dd);
        timetable.setHMS(hh,mm,0);
        QDateTime cdttable;
        cdttable.setDate(datetable);
        cdttable.setTime(timetable);
        //qDebug() << cdt.addSecs(-tableage*3600) << " " << cdttable;
        if (cdt.addSecs(-(tableage*3600)) < cdttable) {

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
    QSqlQuery qry(db);
    if (!qry.exec(createTables)) ui->label->setText("Error "+qry.lastError().text());
    // Coins are looked up by name, never by id. Without this the lookup is a
    // full table scan.
    qry.exec("CREATE INDEX IF NOT EXISTS \"idx_"+dbtable+"_name\" ON \""+dbtable+"\"(name)");
    qry.finish();
    create_db=true;
}
QStringList MainWindow::readpairs()
{
    QFile filein;
    QStringList pairs;

    // Convert blacklist to QSet for O(1) lookups
    QSet<QString> blacklist = {"SUSD", "USD", "EUR", "USDC", "BUSD", "GBP", "BNB", "TUSD", "UST"};

    QSettings appsettings("QTinman", appgroup);
    appsettings.beginGroup(appgroup);
    QStringList blacklistList = appsettings.value(exchange.toLower()+"_blacklist").toStringList();
    QSet<QString> blacklist_exchange(blacklistList.begin(), blacklistList.end());
    appsettings.endGroup();

    QStringList bittrexList = loadsettings("bittrex_blacklist").toStringList();
    QSet<QString> bittrex_blacklist(bittrexList.begin(), bittrexList.end());

    QString cryptolistread = loadsettings("cryptolistread").toString();
    QString filename = cryptolistread + "/raw_" + exchange.toLower() + ".txt";

    filein.setFileName(filename);
    if (!filein.open(QIODevice::ReadOnly)) {
        ui->messages->setText(filename + " not found, create with freqtrade list-pairs --exchange " + exchange.toLower() + " > raw_" + exchange.toLower() + ".txt");
        return pairs;
    }

    QTextStream in(&filein);
    int added = 0;

    while (!in.atEnd())
    {
        QString content = in.readLine();

        // Find the first "/" after position 10
        int middle = content.indexOf("/", 10);
        if (middle == -1) continue; // Skip invalid lines

        int start = content.lastIndexOf(" ", middle);
        int end = content.indexOf(" ", middle);

        if (start == -1 || end == -1) continue; // Invalid format

        QStringView par1 = QStringView(content).mid(start + 1, middle - start - 1);
        QStringView par2 = QStringView(content).mid(middle + 1, end - middle - 1);

        // Check blacklists using QSet for fast lookups
        if (blacklist.contains(par1.toString()) || blacklist_exchange.contains(par1.toString()) || crypt == par1)
            continue;

        if (crypt == par2) {
            pairs.append(par1.toString());  // Convert QStringView to QString
            added++;
        }
    }

    filein.close();
    ui->messages->setText("Pairs found for " + exchange + " " + QString::number(added));

    return pairs;
}



QStringList MainWindow::initializemodel()
{
        bool weekplus=false,dayplus=false,hourplus=false,volumeok=false,inrank=false,marked_cap_ok=false,priceplus=false,priceok=false,volummin=false;


        double percent,price_change;
        int db_id,coincounts=1,coininlist=0,unique=0;
        QDate cd = QDate::currentDate();
        QDateTime cdt = QDateTime::currentDateTime();
        QSqlRecord record;
        QFile csv_file;
        QString db_symbol,db_name, symbol,db_last_updated;
        // db_percent_change_1h had no initialiser and was only assigned when a
        // previous row was found, then written into the CSV report regardless.
        double db_volume_24h=0,db_percent_change_1h=0,db_market_cap=0,db_price=0;
        int json_year,json_mo,json_date,json_h, db_year,db_mo,db_date,db_h,db_min;
        //ui->messages->setText("Please wait.....");
        bool report=loadsettings("report").toBool();
        QString csv_string="Name,Volume new,Volume old,1h change,1h change old,Last updated, Old last updated";
        if (report) {
            QString csv_filename="report_"+exchange+"_"+cd.toString()+"_"+cdt.toString()+".csv";
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
        QString top_symbol="",bottom_symbol="";
        bool autoupdatejson=loadsettings("autoupdatejson").toBool();
        dbtable=ui->tables->currentText();
        // The table to compare against. createdb() below points dbtable at a
        // brand new empty table, so the previous one has to be remembered here
        // or every lookup in the loop finds nothing and the comparison columns
        // are all computed against zero.
        QString prevtable = dbtable;
        if ((timeupdatedb && autoupdatejson) || ui->actionUpdateJson->isChecked()) {
            create_db=true;
            cycles++;
            createdb();
        }
        coin_to = ui->cointo->value();
        coin_from = ui->coinfrom->value();

        // One query instead of one per coin. The old code ran
        // "SELECT * FROM t WHERE name=:name" inside the loop against a table
        // whose only index is its primary key - a full scan every time, so
        // 5000 coins meant 5000 scans of up to 5000 rows each.
        QHash<QString, PrevRow> prev;
        // The previous table's own timestamp, taken from its rows. The status
        // line used to report db_date/db_mo/db_h/db_min, which are parsed per
        // coin inside the loop and then read after it - so it showed whatever
        // the last coin happened to leave behind, and 0/0 0:00 whenever that
        // coin had no previous row.
        QString prevStamp;
        if (!prevtable.isEmpty()) {
            QSqlQuery prevqry(db);
            if (prevqry.exec("SELECT id,symbol,name,price,volume_24h,percent_change_1h,"
                             "market_cap,last_updated FROM \""+prevtable+"\"")) {
                while (prevqry.next()) {
                    PrevRow r;
                    r.id                = prevqry.value(0).toInt();
                    r.symbol            = prevqry.value(1).toString();
                    r.price             = prevqry.value(3).toDouble();
                    r.volume_24h        = prevqry.value(4).toDouble();
                    r.percent_change_1h = prevqry.value(5).toDouble();
                    r.market_cap        = prevqry.value(6).toDouble();
                    r.last_updated      = prevqry.value(7).toString();
                    if (prevStamp.isEmpty()) prevStamp = r.last_updated;
                    prev.insert(prevqry.value(2).toString(), r);
                }
            } else qDebug() << "Error reading previous table:" << prevqry.lastError().text();
        }

        btc_price = loadsettings(exchange.toLower()+"_stake_coin_price").toDouble();
        QJsonArray jsonArray = ReadJson(path+"/crypto_"+crypt+".json");

        // One transaction for the whole insert run. Without it SQLite commits
        // and fsyncs each row on its own, which is what made an update take
        // minutes: 5000 rows meant 5000 separate disk syncs.
        QSqlQuery insert_qry(db);
        if (create_db) {
            if (!db.transaction()) qDebug() << "No transaction:" << db.lastError().text();
            if (!insert_qry.prepare("INSERT INTO \""+dbtable+"\" (id,symbol,name,price,volume_24h,"
                                    "percent_change_1h,percent_change_24h,percent_change_7d,"
                                    "market_cap,last_updated) VALUES (?,?,?,?,?,?,?,?,?,?)"))
                qDebug() << "Insert prepare failed:" << insert_qry.lastError().text();
        }

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

                // Previous values for this coin, from the hash built once above.
                // Cleared each time round: leaving the last coin's values in
                // place is how a coin missing from the previous table used to
                // be compared against whichever coin came before it.
                db_id = 0; db_symbol.clear(); db_name.clear();
                db_price = 0; db_volume_24h = 0; db_percent_change_1h = 0;
                db_market_cap = 0; db_last_updated.clear();
                QHash<QString, PrevRow>::const_iterator pit = prev.constFind(name);
                if (pit != prev.constEnd()) {
                    db_id                = pit->id;
                    db_symbol            = pit->symbol;
                    db_name              = name;
                    db_price             = pit->price;
                    db_volume_24h        = pit->volume_24h;
                    db_percent_change_1h = pit->percent_change_1h;
                    db_market_cap        = pit->market_cap;
                    db_last_updated      = pit->last_updated;
                }
                QString last_updated_time = db_last_updated.mid(11,5)+"/"+last_updated.mid(11,5);
                json_year = last_updated.mid(0,4).toInt();
                json_mo = last_updated.mid(5,2).toInt();
                json_date = last_updated.mid(8,2).toInt();
                json_h = last_updated.mid(11,2).toInt();

                db_year = db_last_updated.mid(0,4).toInt();
                db_mo = db_last_updated.mid(5,2).toInt();
                db_date = db_last_updated.mid(8,2).toInt();
                db_h = db_last_updated.mid(11,2).toInt();
                db_min = db_last_updated.mid(14,2).toInt();


                QDate json_d(json_year,json_mo,json_date);
                QDate db_d(db_year,db_mo,db_date);
                QDateTime lastdbupdate(db_d,QTime(db_h,db_min));

                // db_price is 0 for a coin with no previous row - every coin,
                // on the run that creates the table. Dividing gave inf, which
                // then sorted and filtered as if it were a real number.
                price_change = (db_price > 0) ? (price/db_price*100)-100 : 0;
                if (db_last_updated == last_updated || symbol != db_symbol) price_change=0;
                if (top_1h<percent_change_1h) {
                    top_1h = percent_change_1h;
                    top_symbol = symbol;
                }
                if (bottom_1h>percent_change_1h) {
                    bottom_1h = percent_change_1h;
                    bottom_symbol = symbol;
                }
                if ((json_date > db_date || (json_date == db_date && json_h >= db_h+10)) && symbol == "ETH") ui->messages->setText("DB is over 10h old! From " +QString::number(db_date,'g',2)+"/"+QString::number(db_mo)+", time "+QString::number(db_h)+":"+QString::number(db_min));

                if (create_db) {
                    // Bound, not concatenated. The old string-built INSERT put
                    // the name between single quotes, so any coin named with an
                    // apostrophe produced a syntax error and was dropped.
                    insert_qry.addBindValue(id);
                    insert_qry.addBindValue(symbol);
                    insert_qry.addBindValue(name);
                    insert_qry.addBindValue(price);
                    insert_qry.addBindValue(volume_24h);
                    insert_qry.addBindValue(percent_change_1h);
                    insert_qry.addBindValue(percent_change_24h);
                    insert_qry.addBindValue(percent_change_7d);
                    insert_qry.addBindValue(market_cap);
                    insert_qry.addBindValue(last_updated);
                    if (!insert_qry.exec()) qDebug() << "Error " << insert_qry.lastError().text();
                }

                percent = (db_volume_24h > 0) ? (volume_24h/db_volume_24h)*100 : 0;
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
                // A coin with no row in the previous table has db_id 0, which
                // made the move read as minus its own rank - the column was
                // full of -121, -145, -691 on any newly built table.
                QString rankmove = (db_id == 0 || db_id-id == 0) ? "-"
                                                                 : QString::number(db_id-id);
                for ( const auto& i : pairs  )
                {
                    if (i == symbol) inrank = true;
                    if (i == lastsymbol) unique++;
                 }
                for ( const auto& i : pairs  ) //Write to tableview
                {
                    if ((priceok && weekplus && dayplus && hourplus && volumeok && volummin && i==symbol && inrank && marked_cap_ok && unique<2 && priceplus && coin_from <= coincounts) || (!ui->filter->isChecked() && i==symbol && unique<2 && coin_from <= coincounts)) {
                        modeldatalist << QString::number(id) << symbol << name << QString::number(price, 'F', 3) << QString::number(price_change, 'F', 2) << QString::number(volume_24h, 'F', 2) << QString::number(percent_change_1h, 'F', 2) << QString::number(percent_change_24h, 'F', 2) << QString::number(percent_change_7d, 'F', 2) << rankmove << last_updated_time;
                        if (report) {
                            csv_string=name+","+QString::number(volume_24h)+","+QString::number(db_volume_24h)+","+QString::number(percent_change_1h)+","+QString::number(db_percent_change_1h)+","+last_updated+","+db_last_updated+","+cd.toString()+","+cdt.toString();
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
                if ((coin_to <= coincounts && !ui->actionUpdateJson->isChecked()) || (ui->actionUpdateJson->isChecked() && rowsintable <= coincounts)) break;


        }
        if (jsonArray.count() == 0) ui->messages->setText("Json file not found, please update json file");
        if (pairs.count() > 0 && jsonArray.count() > 0) {
            // Says which table the comparison columns are against and how many
            // coins it had, so an empty previous table is visible rather than
            // showing up as a screen full of zeroes.
            QString when = prevStamp.isEmpty()
                         ? QString("no earlier table to compare against")
                         : QString("compared against %1 (%2 coins), written %3")
                               .arg(prevtable).arg(prev.count())
                               .arg(QDateTime::fromString(prevStamp, Qt::ISODateWithMs).isValid()
                                    ? QDateTime::fromString(prevStamp, Qt::ISODateWithMs)
                                          .toLocalTime().toString("d MMM HH:mm")
                                    : prevStamp.left(16));
            ui->messages->setText(when);
        }
        // Close the transaction opened before the loop. Everything the run
        // inserted reaches disk here, in one commit.
        if (create_db && !db.commit()) {
            qDebug() << "Commit failed:" << db.lastError().text();
            db.rollback();
        }
        csv_file.close();
        create_db = false;
        if ((timeupdatedb && autoupdatejson) || ui->actionUpdateJson->isChecked()) {
            timeupdatedb=false;
            ui->actionUpdateJson->setChecked(false);
            if (autoupdatejson) autoupdatejson = false;
            fetchFromExchange();
        } else if (jsonArray.count() == 0 && !exchangeFetchTried) {
            // Nothing to show and no data file. Rather than sit empty telling
            // the user to go and find an API key, ask the exchange they have
            // already selected - no key needed, and it is the exchange whose
            // pairs they are about to trade.
            //
            // Once only. The reply calls reload_model, which lands back here,
            // and an exchange that returns nothing we can use would otherwise
            // have us asking it forever. Reset when the user picks a different
            // exchange or asks for an update.
            exchangeFetchTried = true;
            fetchFromExchange();
        }
        if (pairs.count() > 0 && jsonArray.count() > 0) ui->messages->setText(ui->messages->text()+", Found and added to list "+QString::number(coininlist) + " / Winner: " + top_symbol + "  " + QString::number(top_1h) + "% / Looser: " + bottom_symbol + "  " + QString::number(bottom_1h)+"%");
        return modeldatalist;
}



void MainWindow::combo_refresh(int comboindex)
{
    exchange = ui->comboBox->currentText();
    crypt = loadsettings(exchange.toLower() + "_stake").toString();
    exchangeFetchTried = false;   // a different exchange is worth asking again
    ui->tables->clear();
    for ( const auto& i : db.tables()  ) if (i.contains(crypt+"_")) ui->tables->addItem(i);
    ui->tables->setCurrentIndex(ui->tables->count()-1);
    dbtable=ui->tables->currentText();
    if (dbtable=="") createdb();
    this->setWindowTitle("Cryptocurrency tool for Freqtrade, active stake coin "+crypt);
    model->setHeaderData(3, Qt::Horizontal, "Price "+crypt, Qt::DisplayRole);
    //qDebug() << "ComboRefresh";
    reload_model();
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
    QDateTime cdt=QDateTime::currentDateTime();
    if (timeroff <= cdt) {
        timeupdatedb=true;
        timeroff=QDateTime::currentDateTime().addMSecs(timer->remainingTime());
    }
    QStringList modellist = initializemodel();
    model->setRowCount(modellist.length()/colums);
    while (i < modellist.length()-1) {
       for (col=0;col<colums;col++) {
         index=model->index(row,col,QModelIndex());
         if (col == 0) model->setData(index,modellist[i].toInt());
         if (col < 4 & col > 0) model->setData(index,modellist[i]);
         if (col < 9 & col > 2) model->setData(index,modellist[i].toDouble());
         if (col == 9) model->setData(index,modellist[i].toInt());
         if (col == 10) model->setData(index,modellist[i]);
         model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         i++;
        }
      row++;
    }
   // if (loadsettings("autopudatejson", autoupd)
}


void MainWindow::calc_profit()
{
    //Adds daily profit to totalsum and calculate a new day.
    int i;
    float dailyprofit=5,percent,start=250,days=30;
    for (i=1;i<=days;i++) {
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

void MainWindow::on_actionUpdateJson_changed()
{
    ui->messages->setText("Updating prices, please wait...");
    ui->messages->show();
    exchangeFetchTried = false;   // an explicit Update always asks again
    reload_model();
}

void MainWindow::on_tables_activated(int index)
{
    dbtable=ui->tables->currentText();
    initializeModel(sqlmodel);
    reload_model();
}

// Ask the selected exchange for every symbol it trades, in one request and
// with no API key. CoinMarketCap needed a key, charged for it, rate limited
// it, and reported a price averaged across exchanges rather than the price
// this exchange would actually fill at.
void MainWindow::fetchFromExchange()
{
    QString ex = exchange.toLower();
    QUrl url;
    if (ex == "binance")      url = QUrl("https://api.binance.com/api/v3/ticker/24hr");
    else if (ex == "kucoin")  url = QUrl("https://api.kucoin.com/api/v1/market/allTickers");
    else {
        ui->messages->setText("No exchange source for "+exchange+" yet - Binance and KuCoin are supported.");
        return;
    }
    ui->messages->setText("Fetching prices from "+exchange+"...");
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    manager->get(request);
}

// Turn an exchange ticker into the shape the rest of the program already
// reads, so ReadJson and the whole display path stay as they are. Market cap
// and rank are not something an exchange knows, so they come from CoinGecko,
// which also needs no key; without it the coins are ranked by turnover.
// CoinGecko, one page of 250 at a time. Four pages covers the top 1000 by
// market cap, which is more than any of these exchanges lists against a
// single stake coin.
void MainWindow::fetchGeckoPage(int page)
{
    QString vs = crypt; if (vs.contains("USD")) vs = "usd";
    QNetworkRequest gq(QUrl("https://api.coingecko.com/api/v3/coins/markets?vs_currency="
                            +vs.toLower()+"&order=market_cap_desc&per_page=250&page="
                            +QString::number(page)+"&price_change_percentage=1h,24h,7d"));
    gq.setRawHeader("Accept", "application/json");
    manager->get(gq);
}

QByteArray MainWindow::exchangeToCmcJson(const QByteArray &tickers, const QJsonArray &gecko)
{
    QString stake = crypt;
    if (stake.contains("USD")) stake = "USD";
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    // symbol -> (rank, market cap, 1h, 7d). CoinGecko lists several coins under
    // one symbol, so the first wins: the response is ordered by market cap.
    struct Extra { int rank=0; double cap=0, h1=0, d7=0; bool have=false; };
    QHash<QString, Extra> extra;
    for (const QJsonValue &v : gecko) {
        QJsonObject o = v.toObject();
        QString sym = o["symbol"].toString().toUpper();
        if (sym.isEmpty() || extra.contains(sym)) continue;
        Extra e;
        e.rank = o["market_cap_rank"].toInt();
        e.cap  = o["market_cap"].toDouble();
        e.h1   = o["price_change_percentage_1h_in_currency"].toDouble();
        e.d7   = o["price_change_percentage_7d_in_currency"].toDouble();
        e.have = true;
        extra.insert(sym, e);
    }

    struct Row { QString sym; double price, vol, ch24; };
    QList<Row> rows;
    QJsonDocument doc = QJsonDocument::fromJson(tickers);

    if (exchange.compare("binance", Qt::CaseInsensitive) == 0) {
        for (const QJsonValue &v : doc.array()) {
            QJsonObject o = v.toObject();
            QString s = o["symbol"].toString();
            // The ticker concatenates base and quote with no separator, so the
            // stake coin is the suffix to strip.
            if (!s.endsWith(crypt) || s.size() <= crypt.size()) continue;
            Row r;
            r.sym   = s.left(s.size()-crypt.size());
            r.price = o["lastPrice"].toString().toDouble();
            r.vol   = o["quoteVolume"].toString().toDouble();
            r.ch24  = o["priceChangePercent"].toString().toDouble();
            if (r.vol > 0) rows.append(r);
        }
    } else {                                   // KuCoin: BASE-QUOTE, rate as a fraction
        for (const QJsonValue &v : doc.object()["data"].toObject()["ticker"].toArray()) {
            QJsonObject o = v.toObject();
            QStringList parts = o["symbol"].toString().split('-');
            if (parts.size() != 2 || parts[1] != crypt) continue;
            Row r;
            r.sym   = parts[0];
            r.price = o["last"].toString().toDouble();
            r.vol   = o["volValue"].toString().toDouble();
            r.ch24  = o["changeRate"].toString().toDouble()*100;
            if (r.vol > 0) rows.append(r);
        }
    }

    // Rank by turnover, then let CoinGecko's own rank win where it has one.
    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b){ return a.vol > b.vol; });

    // Rank has to mean one thing. CoinGecko gives a real market cap rank for
    // the coins it covers; the rest had been numbered by their position in
    // this volume-sorted list, which put a long-tail coin at "5" beside a
    // genuine number 5. Anything unranked is numbered after the ranked ones
    // instead, keeping the column a single ordering.
    int maxrank = 1000;
    for (const Row &r : rows) {
        const Extra e = extra.value(r.sym);
        if (e.have && e.rank > maxrank) maxrank = e.rank;
    }
    int unranked = maxrank;

    QJsonArray data;
    for (int i = 0; i < rows.size(); i++) {
        const Row &r = rows[i];
        const Extra e = extra.value(r.sym);
        QJsonObject q;
        q["price"]              = r.price;
        q["volume_24h"]         = r.vol;
        q["percent_change_1h"]  = e.have ? e.h1 : 0.0;
        q["percent_change_24h"] = r.ch24;
        q["percent_change_7d"]  = e.have ? e.d7 : 0.0;
        q["market_cap"]         = e.cap;
        q["last_updated"]       = now;
        QJsonObject quote; quote[stake] = q;
        QJsonObject coin;
        coin["cmc_rank"] = (e.have && e.rank > 0) ? e.rank : ++unranked;
        coin["symbol"]   = r.sym;
        coin["name"]     = r.sym;        // an exchange ticker carries no long name
        coin["quote"]    = quote;
        data.append(coin);
    }
    QJsonObject root; root["data"] = data;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void MainWindow::replyFinished (QNetworkReply *reply)
{
    if(reply->error())
    {
        // CoinGecko only supplies market cap, rank and the 1h/7d figures. If it
        // is down or rate limiting us, the exchange prices are still good and
        // are worth writing without it.
        if (reply->url().host().contains("coingecko") && !pendingTickers.isEmpty()) {
            // Whatever pages did arrive are still worth using.
            QByteArray body = exchangeToCmcJson(pendingTickers, geckoRows);
            pendingTickers.clear();
            geckoRows = QJsonArray();
            QString path = loadsettings("json_path").toString();
            if (path == "") path = ".";
            QFile file(path+"/crypto_"+crypt+".json");
            if (file.open(QFile::WriteOnly)) { file.write(body); file.close(); }
            ui->messages->setText("Prices from "+exchange+"; no market cap (CoinGecko: "
                                  +reply->errorString()+")");
            reload_model();
        } else ui->messages->setText("ERROR! "+reply->errorString());
    }
    else
    {
        //ui->transferLog->appendPlainText(reply->header(QNetworkRequest::ContentTypeHeader).toString());
        //ui->transferLog->appendPlainText(reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toString());
        //ui->transferLog->appendPlainText(reply->header(QNetworkRequest::ContentLengthHeader).toString());
        //ui->transferLog->appendPlainText(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
        //ui->transferLog->appendPlainText(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
        //ui->transferLog->appendPlainText("OK");
        const QString host = reply->url().host();
        QByteArray body = reply->readAll();

        // An exchange ticker is only half the answer: ask CoinGecko for the
        // market caps and ranks, then convert both together.
        if (host.contains("binance") || host.contains("kucoin")) {
            pendingTickers = body;
            geckoRows = QJsonArray();
            geckoPage = 1;
            fetchGeckoPage(geckoPage);
            reply->deleteLater();
            return;
        }
        if (host.contains("coingecko")) {
            const QJsonArray page = QJsonDocument::fromJson(body).array();
            for (const QJsonValue &v : page) geckoRows.append(v);
            // Keep asking while the pages are full and we are inside the top
            // 1000. A short page means the list has run out.
            if (page.count() == 250 && geckoPage < 4) {
                ui->messages->setText("Fetching market caps ("+QString::number(geckoRows.count())+")...");
                fetchGeckoPage(++geckoPage);
                reply->deleteLater();
                return;
            }
            body = exchangeToCmcJson(pendingTickers, geckoRows);
            pendingTickers.clear();
            geckoRows = QJsonArray();
        }

        QString path = loadsettings("json_path").toString();
        if (path == "") path = ".";
        QFile *file = new QFile(path+"/crypto_"+crypt+".json");
        if(file->open(QFile::WriteOnly))
        {
            file->write(body);
            file->flush();
            file->close();
        }
        delete file;
        reload_model();
    }

    reply->deleteLater();
}



// The path the pair list is written to, and the one the simulator reads.
QString MainWindow::pairFilePath()
{
    QString dir = loadsettings("cryptolistwrite").toString();
    if (dir == "") dir = ".";
    return dir+"/"+crypt.toLower()+"_"+exchange.toLower()+".txt";
}

// Lifted out of on_fileButton_clicked so that the startup path can write the
// same file without a button press.
bool MainWindow::writePairFile(const QString &filename)
{
    QFile fileout(filename);
    int row=0,col=1,rows=model->rowCount();
    QStringList pairs;
    bool doub=false;
    QModelIndex index=model->index(row,col,QModelIndex());

    if (!fileout.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error opening file!" << fileout.errorString();
        return false;
    }
    QTextStream out(&fileout);
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
    fileout.close();
    return !pairs.isEmpty();
}

void MainWindow::on_fileButton_clicked()
{
    const QString f = pairFilePath();
    if (writePairFile(f)) ui->messages->setText("Pairs written to file -> "+f);
    else ui->messages->setText("Nothing to write - no coins in the table.");
}


void MainWindow::on_filterButton_clicked()
{
    coinfilterDialog coinfilter;
    QObject::connect(&coinfilter, SIGNAL(destroyed()), this, SLOT(reload_model()));
    coinfilter.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    coinfilter.exec();
}


void MainWindow::on_settingsButton_clicked()
{
    settingsDialog settingsdialog;
    QObject::connect(&settingsdialog, SIGNAL(destroyed()), this, SLOT(reload_model()));
    settingsdialog.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    settingsdialog.exec();
    ui->filter->setChecked(false);
    tableage();
    this->setWindowTitle("Cryptocurrency tool for Freqtrade, active stake coin "+crypt);
}


void MainWindow::on_stocksButton_clicked()
{
    stocksDialog stocksdialog;
    stocksdialog.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    stocksdialog.exec();
}

void MainWindow::on_coffee_clicked()
{
    QMessageBox msgBox;
    QClipboard *clipboard=0;
    msgBox.setWindowTitle("Coffee");
    msgBox.setText("A coffee for creator \nBTC bc1qt4jv89m5gy62e64j9kwyhu7n65s0yhp4nqayag\nETH 0x9129B88AA1fEE2545d1123e71e9Fa2c43C075979\nSOL 3s4nqsu2WiPFrXSmxKY7U3zX4RtKCEgczaNkVBwcgdXS\nZEC t1fb8QAsRdNT2sF6LXoYsHSmHLv18UpPW9D\nAppreciated");
    QAbstractButton* pButtonYes = msgBox.addButton("Copy to clipboard", QMessageBox::YesRole);
    msgBox.exec();
    if (msgBox.clickedButton()==pButtonYes) {
        clipboard->setText("BTC bc1qt4jv89m5gy62e64j9kwyhu7n65s0yhp4nqayag\nETH 0x9129B88AA1fEE2545d1123e71e9Fa2c43C075979\nSOL 3s4nqsu2WiPFrXSmxKY7U3zX4RtKCEgczaNkVBwcgdXS\nZEC t1fb8QAsRdNT2sF6LXoYsHSmHLv18UpPW9D");
    }
}
