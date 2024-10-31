#ifndef WIDGET_H
#define WIDGET_H

#include "qpushbutton.h"
#include <QSerialPort>
#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    QList<QString> hisMsgs;//历史记录
    int sendCnt = 0;//发送数
    int receiveCnt = 0;//接收数
    int sendRate = 1000;//定时发送延时时间,默认1000ms
    QTimer *qTimerSend;//创建发送延时QTimer对象
    int dateRate = 1000;//设置时间显示 刷新时间
    int loopSendRate = 500;//设置循环发送多文本时间
    QTimer *qTimerDate;
    QString receive = NULL;
    QTimer *qTimerLoop;
    bool isSendHex = false;
    int loopBegin = 0;
    QList<QPushButton *> *buttons;
signals:
    void popupShown();
private slots:
    void on_pushButton_OpenSerial_clicked();

    void on_pushButton_Send_clicked();
    void getMessage();
    void on_pushButton_ClearReceive_clicked();

    void on_pushButton_HideHistory_clicked();
    void on_timeOut_Send();

    void on_checkBox_ScheduledSend_clicked(bool checked);

    void on_pushButton_SaveReceive_clicked();
    void on_Show_DateTime();
    void on_checkBox_HexShow_clicked(bool checked);

    void on_pushButton_HidePanel_clicked();
    void on_Refresh_Serials();
    void on_multiple_text();
    void on_checkBox_SendLoop_clicked(bool checked);
    void on_timeout_loop();
    void on_pushButton_Reset_clicked();

    void on_pushButton_Save_clicked();

    void on_pushButton_Load_clicked();

private:
    Ui::Widget *ui;
    QSerialPort *qSerialPort;
};
#endif // WIDGET_H
