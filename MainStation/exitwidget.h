#ifndef EXITWIDGET_H
#define EXITWIDGET_H

#include <QWidget>

namespace Ui {
class ExitWidget;
}

class ExitWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExitWidget(QWidget *parent = 0);
    ~ExitWidget();

protected slots:
    void onLockStation();
    void onExitStation();
    void onShutDown();
    void onReboot();

private:
    Ui::ExitWidget *ui;
};

#endif // EXITWIDGET_H
