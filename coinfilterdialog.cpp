#include "coinfilterdialog.h"
#include "ui_coinfilterdialog.h"
#include "mainwindow.h"

coinfilterDialog::coinfilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::coinfilterDialog)
{
    ui->setupUi(this);
    ui->change_1h->setChecked(change_1h);
    ui->change_24h->setChecked(change_24h);
    ui->change_7d->setChecked(change_7d);
    ui->pricechange->setChecked(change_price);
    ui->pricefilter->setChecked(pricefilter);
    ui->use_last_volume->setChecked(use_volume);
    ui->volume_greater->setChecked(marked_cap);
    ui->show_only_blacklisted->setChecked(show_only_blacklisted);
    ui->change_1h_from->setValue(from1h);
    ui->change_1h_to->setValue(to1h);
    ui->change_24h_from->setValue(from24h);
    ui->change_24h_to->setValue(to24h);
    ui->change_7d_from->setValue(from7d);
    ui->change_7d_to->setValue(to7d);
    ui->pricechange_from->setValue(price_change_from);
    ui->pricechange_to->setValue(price_change_to);
    ui->pricemin->setValue(pricemin);
    ui->pricemax->setValue(pricemax);
    ui->last_volume_percent->setValue(volume_percent);
    ui->volum_min->setValue(volum_min);
    ui->volume_min_check->setChecked(volume_min_check);
    ui->crypt->setText(crypt);
}

coinfilterDialog::~coinfilterDialog()
{
    //qDebug() << "Filter exit";
    delete ui;
}

void coinfilterDialog::on_buttonBox_accepted()
{
    //qDebug() << "Filter Accept";
    change_1h = ui->change_1h->isChecked();
    change_24h = ui->change_24h->isChecked();
    change_7d = ui->change_7d->isChecked();
    change_price = ui->pricechange->isChecked();
    use_volume = ui->use_last_volume->isChecked();
    marked_cap = ui->volume_greater->isChecked();
    pricefilter = ui->pricefilter->isChecked();
    show_only_blacklisted = ui->show_only_blacklisted->isChecked();
    from1h =  ui->change_1h_from->value();
    to1h =  ui->change_1h_to->value();
    from24h =  ui->change_24h_from->value();
    to24h =  ui->change_24h_to->value();
    from7d =  ui->change_7d_from->value();
    to7d =  ui->change_7d_to->value();
    price_change_from = ui->pricechange_from->value();
    price_change_to = ui->pricechange_to->value();
    pricemin = ui->pricemin->value();
    pricemax = ui->pricemax->value();
    volume_percent = ui->last_volume_percent->value();
    volum_min = ui->volum_min->value();
    volume_min_check = ui->volume_min_check->isChecked();
}
