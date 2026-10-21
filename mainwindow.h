#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QtSql/QSqlTableModel>
#include <QtSql/QSql>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDatabase>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
//#include <QtQml/QQmlEngine>

// One row of the previous run's table, kept in a hash so the update loop can
// look a coin up without querying the database once per coin.
struct PrevRow {
    int id = 0;
    QString symbol;
    double price = 0, volume_24h = 0, percent_change_1h = 0, market_cap = 0;
    QString last_updated;
};

// The simulator's starting stake, in one place so Settings and the simulator
// cannot disagree about it. 1000 when there is nothing to protect - no cash
// and no holdings. A balance spent to zero while coins are still held is a
// real position and comes back unchanged.
int stakedBalance(int stored);

extern QSqlDatabase db;
extern QTimer *timer;
extern QString exchange, appgroup,crypt;
extern double from1h,to1h,from24h,to24h,from7d,to7d,markedcap_percent,volume_percent_min,volume_percent_max,price_change_from,price_change_to,volum_min,pricemin,pricemax;
extern bool change_1h,change_24h,change_7d,marked_cap,use_volume,show_only_blacklisted,change_price,createdb,pricefilter,volume_min_check;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    QJsonArray ReadJson(const QString &path);
    QStringList readpairs();
    QStringList initializemodel();
    void combo_refresh(int index);
    void createdb();
    // What the Table dropdown shows for a table: when it was written, in
    // the local convention. The table name travels as the item's data.
    QString tableLabel(const QString &table);
    QDateTime tableMade(const QString &table);
    void initializeModel(QSqlTableModel *sqlmodel);
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    // Fetch prices from the selected exchange instead of CoinMarketCap. No API
    // key, and the exchange only lists what it actually trades.
    QString pairFilePath();
    bool writePairFile(const QString &filename);
    void fetchFromExchange();
    void fetchGeckoPage(int page);
    QByteArray exchangeToCmcJson(const QByteArray &tickers, const QJsonArray &gecko);
    ~MainWindow();

private slots:
    void searchmodel(const QString&);
    void on_filter_clicked();
    void reload_model();
    void tableage();
    void calc_profit();
    void on_actionUpdateJson_changed();
    void replyFinished (QNetworkReply *reply);
    void on_tables_activated(int index);

    void on_fileButton_clicked();

    void on_filterButton_clicked();

    void on_settingsButton_clicked();

    void on_stocksButton_clicked();

    void on_coffee_clicked();

private:
    Ui::MainWindow *ui;
    QSqlTableModel * sqlmodel;
    QStandardItemModel *model;
    QNetworkAccessManager *manager;
    // The exchange reply, held while CoinGecko is asked for the market caps
    // and ranks that an exchange ticker does not carry.
    QByteArray pendingTickers;
    // Kraken only: its pair list, held while the ticker is fetched.
    QByteArray pendingPairs;
    // CoinGecko returns at most 250 coins per request, and a busy exchange
    // lists far more than that, so the pages are collected before converting.
    QJsonArray geckoRows;
    int geckoPage = 0;
    // Guards the fetch-when-empty path against asking forever. See
    // initializemodel().
    bool exchangeFetchTried = false;
};
#endif // MAINWINDOW_H
