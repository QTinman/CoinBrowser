#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class settingsDialog;
}

class settingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit settingsDialog(QWidget *parent = nullptr);
    ~settingsDialog();

private slots:
    void on_selectJsonFile_clicked();

    void on_toolcryptolistwrite_clicked();

    void on_toolcryptolistread_clicked();

    void on_toolReportPath_clicked();

    void on_exchanges_activated();

    void on_pushButton_clicked();

    QVariant loadsettings(QString settings);

    void savesettings(QString settings, QVariant attr);

    void on_profilelist_activated();

private:
    Ui::settingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
