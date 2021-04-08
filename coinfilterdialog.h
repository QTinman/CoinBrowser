#ifndef COINFILTERDIALOG_H
#define COINFILTERDIALOG_H

#include <QDialog>

namespace Ui {
class coinfilterDialog;
}

class coinfilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit coinfilterDialog(QWidget *parent = nullptr);
    ~coinfilterDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::coinfilterDialog *ui;
};

#endif // COINFILTERDIALOG_H
