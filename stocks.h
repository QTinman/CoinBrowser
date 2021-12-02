#ifndef STOCKS_H
#define STOCKS_H

#include <QObject>

class stocks : public QObject
{
    Q_OBJECT
public:
    explicit stocks(QObject *parent = nullptr);

signals:

};

#endif // STOCKS_H
