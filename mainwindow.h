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

extern QSqlDatabase db;
extern QString exchange, appgroup,crypt;
extern double from1h,to1h,from24h,to24h,from7d,to7d,markedcap_percent,volume_percent,price_change_from,price_change_to,volum_min,pricemin,pricemax;
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
    void initializeModel(QSqlTableModel *sqlmodel);
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void searchmodel(const QString&);
    void on_filter_clicked();
    void reload_model();
    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_updatedb_clicked();

private:
    Ui::MainWindow *ui;
    QSqlTableModel * sqlmodel;
    QStandardItemModel *model;
};
#endif // MAINWINDOW_H
