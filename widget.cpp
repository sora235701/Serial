#include "widget.h"
#include "ui_widget.h"
#include "mycombobox.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QTimer>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    qSerialPort = new QSerialPort(this);

    ui->setupUi(this);
    this->setLayout(ui->gridLayout_3);
    // setFixedSize(790,600);
    this->setWindowTitle("串口调试助手");
    //定时发送信号槽连接
    qTimerSend = new QTimer(this);
    connect(qTimerSend,&QTimer::timeout,this,&Widget::on_timeOut_Send);
    qTimerSend->stop();//默认停止状态
    //接受信息-信号槽连接
    connect(qSerialPort,&QSerialPort::readyRead,this,&Widget::getMessage);
    //初始化串口信息
    QList<QSerialPortInfo> qSerialPortInfos = QSerialPortInfo::availablePorts();//从电脑获取到可用的串口信息
    for(QSerialPortInfo &qSerialPortInfo : qSerialPortInfos){
        // qDebug()<<qSerialPortInfo.portName();
        ui->comboBox_Serial->addItem(qSerialPortInfo.portName());//将串口名称循环输入串口选择框
    }
    //设置打开复选框自动更新串口信息
    connect(ui->comboBox_Serial,&MyComboBox::popupShown,this,&Widget::on_Refresh_Serials);

    // 设置日期时间
    // QDateTime currentDateTime = QDateTime::currentDateTime();
    qTimerDate = new QTimer(this);
    connect(qTimerDate,&QTimer::timeout,this,&Widget::on_Show_DateTime);
    qTimerDate->start();

    //多文本发送
    buttons = new QList<QPushButton *>;//创建一个QList存储多文本按钮
    for(int i = 1;i <= 9;i++){
        QString btnName = QString("pushButton_num%1").arg(i);//根据命名循环获取每一个多文本按钮的名字
        QPushButton *btn = findChild<QPushButton*>(btnName);//findChild根据按钮名字找到对应控件
        if(btn){//如果找到控件
            btn->setProperty("buttonId",i);//设置对应序号
            buttons->append(btn);//添加找到的btn按钮到buttons按钮列表中
            connect(btn,SIGNAL(clicked()),this,SLOT(on_multiple_text()));//给每一个多文本按钮绑定信号槽
        }
    }
    //定时循环发送多文本
    qTimerLoop = new QTimer(this);
    connect(qTimerLoop,&QTimer::timeout,this,&Widget::on_timeout_loop);
    qTimerLoop->stop();
}

Widget::~Widget()
{
    hisMsgs.clear();//每次析构都把这些成员变量置空
    sendCnt = 0;
    receiveCnt = 0;
    delete qTimerSend;
    delete ui;
}
//打开串口按钮点击槽函数
void Widget::on_pushButton_OpenSerial_clicked()
{
    ui->label_SendState->clear();//清空底下左下角发送状态标签
    if(qSerialPort->isOpen()){//如果当前串口已经打开
        qDebug()<<"isopen";
        QMessageBox checkOpen;
        checkOpen.setWindowTitle("提示");
        checkOpen.setText("该串口已经打开！是否要先关闭？");
        checkOpen.setStandardButtons(QMessageBox::Cancel|QMessageBox::Ok);//设置弹出框，是否先关闭当前串口
        if(checkOpen.exec()==QMessageBox::Cancel){
        }else{
            qSerialPort->close();
            qDebug()<<"serial close";
        }
        return ;
    }
    //设置停止位Map
    std::map<QString,int> stopBitMap;
    stopBitMap["One"] = 1;
    stopBitMap["OnePointFive"] = 3;
    stopBitMap["Two"] = 2;

    //设置校验位Map
    std::map<QString,int> parityMap;
    parityMap["None"] = 0;
    parityMap["Even"] = 2;
    parityMap["Odd"] = 3;
    parityMap["Space"] = 4;
    parityMap["Mark"] = 5;
    //     NoParity = 0,
    // EvenParity = 2,
    // OddParity = 3,
    // SpaceParity = 4,
     // MarkParity = 5
    //设置串口号
    QString serialName = static_cast<QString>(ui->comboBox_Serial->currentText());
    //设置波特率
    if(!qSerialPort->setBaudRate(ui->comboBox_BautRate->currentText().toInt())){//获取设置波特率选择框的文本，转成int类型，下列同上，不再做解释。
        qDebug()<<"setBaudRate err";
    }else{
        qDebug()<<"setBaudRate"<<qSerialPort->baudRate()<<"success";
    }

    //设置数据位
    if(!qSerialPort->setDataBits(static_cast<QSerialPort::DataBits>(ui->comboBox_DataBit->currentText().toUInt()))){
        qDebug()<<"setDataBits err";
    }else{
        qDebug()<<"setDataBits"<<qSerialPort->dataBits()<<"success";
    }
    //设置校验位
    if(!qSerialPort->setParity(static_cast<QSerialPort::Parity>(parityMap[ui->comboBox_VeriBit->currentText()]))){
        qDebug()<<"setParity err";
    }else{
        qDebug()<<"setParity"<<qSerialPort->parity()<<"success";
    }

    //设置停止位
    if(!qSerialPort->setStopBits(static_cast<QSerialPort::StopBits>(stopBitMap[ui->comboBox_StopBit->currentText()]))){
        qDebug()<<"setStopBits err";
    }else{
        qDebug()<<"setStopBits"<<qSerialPort->stopBits()<<"success";
    }
    //设置流控
    QString fc = ui->comboBox_FlowControll->currentText();
    if(fc=="No"){
        qSerialPort->setFlowControl(QSerialPort::NoFlowControl);
        qDebug()<<"No FlowControll";
    }else if(fc=="Hard"){
        qSerialPort->setFlowControl(QSerialPort::HardwareControl);
        qDebug()<<"None HardFlowControll";
    }else if(fc == "Soft"){
        qSerialPort->setFlowControl(QSerialPort::SoftwareControl);
        qDebug()<<"None SoftFlowControll";
    }else{
        qSerialPort->setFlowControl(QSerialPort::NoFlowControl);
        qDebug()<<"None FlowControll";
    }
    //打开串口
        qSerialPort->setPortName(serialName);//根据上文从串口选择框内获取到的串口名打开串口
        if(!qSerialPort->open(QIODeviceBase::ReadWrite)){//设置打开方式为读写模式
            qDebug()<<"open serial"<<qSerialPort->portName()<<"Err";
            QString openStateErr = qSerialPort->portName()+":Err";
            ui->label_SendState->setText(openStateErr);//设置底下Open串口状态错误
        }else{
            qDebug()<<"open serial"<<qSerialPort->portName()<<":Open";
            QString openStateSuccess = qSerialPort->portName()+":Open";
            ui->label_SendState->setText(openStateSuccess);//设置底下Open串口状态打开
        }

}

//发送按钮点击槽函数
void Widget::on_pushButton_Send_clicked()
{
    ui->label_SendState->clear();
    QByteArray sendText = ui->lineEdit_Send->text().toUtf8(); // 将发送文本转换为UTF-8编码的QByteArray
    if(ui->checkBox_HexSend->isChecked()){
        QString hexSend = ui->lineEdit_Send->text();
        //判断是否偶数位
        QByteArray hexSendArray = hexSend.toLocal8Bit();
        if(hexSendArray.size() % 2 != 0){
            qDebug()<<"hex digits not true";
            return;
        }
        //判断是否符合十六进制格式
        for(char c : hexSendArray){
            if(!std::isxdigit(c)){
                qDebug()<<"not hex";
                return ;
            }
        }
        //避免hex转hex
        isSendHex = true;
        sendText = QByteArray::fromHex(hexSendArray);
    }else{
        isSendHex = false;
    }
        if(qSerialPort->write(sendText)){//发送信息
            qDebug()<<"send message success:"<<sendText;
            ui->label_SendState->setText(QString("Send:Ok"));//设置底下发送状态
            ui->label_SentNum->setText(QString("Sent:")+QString::number(sendCnt+=QString(sendText).size()));//累加输出设置底下sent发送数据量
        }else{
            qDebug()<<"send err";
            ui->label_SendState->setText(QString("Send:Err"));
        }


}
QString hexToStringSimple(const QString &hexStr) {
    QByteArray hexData = hexStr.toLocal8Bit();
    QByteArray byteData = QByteArray::fromHex(hexData);
    return QString::fromUtf8(byteData);
}
void Widget::getMessage()//信息接收,历史记录-槽函数
{
    QString msg = qSerialPort->readAll();
    // qDebug()<<msg;
    if(receiveCnt!=0){//这段比较复杂，如果接收数为0，则是第一次接收，不需要添加换行
        receive.append('\n');//否则添加换行，目的是为了下文的转换Hex部分，防止出现utf8和hex格式同时转换出现的乱码问题
    }

    receive.append(msg);//将每次收到的数据放入receive成员变量
    ui->textEditRev->append(msg);
    if(ui->checkBox_HexShow->isChecked()){//联动hex显示
        qDebug()<< "sss";
        on_checkBox_HexShow_clicked(true);
    }else{
        on_checkBox_HexShow_clicked(false);
    }

    //设置历史记录
    if(hisMsgs.count(msg)==0){
        if(isSendHex){//如果是十六进制，历史记录上标明
            msg = msg.toUtf8().toHex()+"(hex code)";
        }
        if(ui->checkBox->isChecked()){//如何设置接收时间被勾选
            ui->textEditHis->append(msg+"      "+ui->label_DateTime->text());//输出到历史记录栏同时添加时间
        }else{
            ui->textEditHis->append(msg);
        }
        qDebug()<<msg;
    }
    hisMsgs.append(msg);
    ui->label_ReceivedNum->setText(QString("Receive:")+QString::number(receiveCnt+=msg.size()));//累加输出设置底下receive接收数据量
    qDebug()<<hisMsgs.count(msg);

}


void Widget::on_pushButton_ClearReceive_clicked()//清除接收按钮
{
    QMessageBox clearRecieve;//设置提示选择框
    clearRecieve.setWindowTitle("提示");
    clearRecieve.setText("是否清除接收");
    clearRecieve.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    if(clearRecieve.exec()==QMessageBox::Ok){
        ui->textEditRev->clear();
        receiveCnt = 0;
        receive = NULL;//每次清空也将receive清空
        ui->label_ReceivedNum->setText(QString("Receive:")+QString::number(receiveCnt));//设置底下receive接收数据量为0
    }
}


void Widget::on_pushButton_HideHistory_clicked()//隐藏接收历史按钮
{
    if(!ui->textEditHis->isHidden()){//如果已经被隐藏，则打开，否则隐藏
      ui->textEditHis->hide();
    }else{
        ui->textEditHis->show();
    }
}

void Widget::on_timeOut_Send()//超时处理槽函数
{
    on_pushButton_Send_clicked();//超时后执行发送函数
}

void Widget::on_checkBox_ScheduledSend_clicked(bool checked)//设置定时发送
{
    if(checked){
        sendRate = ui->lineEdit_SendRate->text().toInt();//每次勾选，更新定时发送时间为sendrate文本框设置的时间
        qTimerSend->start(sendRate);//设置延时时间为定时发送框中的时间
    }else{
        qTimerSend->stop();//如果不勾选，则停止定时器操作
    }
}


void Widget::on_pushButton_SaveReceive_clicked()//保存接收
{
    QString fileName = QFileDialog::getSaveFileName(this,tr("Save File"),"F:/Qt_code",tr("Text (*.txt)"));//标准文件操作，获取文件名
    if(fileName != QString("")){//如果文件名不为空
        QFile qFile(fileName);//打开一个文件对象
        if(qFile.open(QIODevice::ReadWrite | QIODevice::Text)){
            QTextStream out(&qFile);//文本输出流
            out << ui->textEditRev->toPlainText();//输出接受框记录到文件
        }
    }
}

void Widget::on_Show_DateTime()//刷新设置日期时间
{
    QDateTime currentDateTime = QDateTime::currentDateTime();//获取当前日期时间
    QString cdt = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");//将日期时间按照标准化格式转换为日期时间格式
    ui->label_DateTime->setText(cdt);//设置当前时间到label
}


void Widget::on_checkBox_HexShow_clicked(bool checked)//转换为Hex十六进制
{
    if(checked){
        QString receiveHex = receive.toUtf8().toHex();//转换为16进制
        ui->textEditRev->clear();
        ui->textEditRev->setPlainText(receiveHex);//设置为十六进制文本
    }else{
        ui->textEditRev->clear();
        ui->textEditRev->setPlainText(receive);//取消勾选，设置回原字符串
        qDebug()<<receive;//receive相当于是一个专门存储原始接收格式的字符串，转换编码时不更改它，这样可以防止无谓的编码问题，准确来说不是转换，而是用数据的刷新来替换格式转换。
    }
}
void Widget::on_pushButton_HidePanel_clicked()//隐藏面板
{
    if(ui->groupBoxText->isHidden()){
        ui->groupBoxText->show();
    }else{
        ui->groupBoxText->hide();
    }
}

void Widget::on_Refresh_Serials()//每次打开串口选择框，自动更新串口列表
{
    ui->comboBox_Serial->clear();
    QList<QSerialPortInfo> qSerialPortInfos = QSerialPortInfo::availablePorts();//从电脑获取到可用的串口信息
    for(QSerialPortInfo &qSerialPortInfo : qSerialPortInfos){
        // qDebug()<<qSerialPortInfo.portName();
        ui->comboBox_Serial->addItem(qSerialPortInfo.portName());//将串口名称循环输入串口选择框
    }
    qDebug()<<"refresh";
}

void Widget::on_multiple_text()//设置多文本按钮按下事件
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());//重点，将发送信号的控件从qobject转为qpushbutton类型，也就是获取到按下的对应多文本按钮
    int num = btn->property("buttonId").toInt();//获取多文本按钮的id

    QString lineEditName = QString("lineEdit_num%1").arg(num);//根据多文本按钮的id设置文本框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
    QLineEdit *lineEdit = findChild<QLineEdit*>(lineEditName);//找到对应的多文本文本框控件
    if(lineEdit){
        ui->lineEdit_Send->setText(lineEdit->text());//设置发送框文本为多文本框文本
    }
    QString checkBoxName = QString("checkBox_num%1").arg(num);//同样的，根据序号这只checkbox名字，根据名字找到对应空间
    QCheckBox *checkBox = findChild<QCheckBox *>(checkBoxName);
    ui->checkBox_HexSend->setChecked(checkBox->isChecked());//根据多文本hex选择框的选择情况设置hex发送选择框的状态。
    on_pushButton_Send_clicked();//执行发送点击槽函数
}




void Widget::on_checkBox_SendLoop_clicked(bool checked)//如果循环发送框被勾选，则启动延时函数
{
    if(checked){
        loopSendRate = ui->spinBox_SendLoop->text().toInt();
        QMessageBox loopContinue;//设置提示框
        loopContinue.setWindowTitle("提示");
        loopContinue.setText("是否重新开始循环？");
        loopContinue.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(loopContinue.exec()==QMessageBox::Ok){
            loopBegin = 0;
        }
        qTimerLoop->start(loopSendRate);//设置延时时间，延时时间loopSendRate是成员变量
    }else{
        qTimerLoop->stop();
    }
}

void Widget::on_timeout_loop()//循环发送槽函数
{
        loopBegin %= 9;//实现1-9循环发送
        QPushButton *btnLoop = buttons->at(loopBegin);//根据loopbegin获取buttons里面的每个button
        emit btnLoop->click();//发送点击信号
        loopBegin++;//每次跳转下一个文本框
}


void Widget::on_pushButton_Reset_clicked()//重置多文本框
{
    QMessageBox resetMessageBox;
    resetMessageBox.setText("是否重置多文本框");
    resetMessageBox.setWindowTitle("提示");
    resetMessageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    if(resetMessageBox.exec() == QMessageBox::Ok){
        qDebug()<<"reset";
        for(int i = 1;i<=9;i++){
            QString lineEditName = QString("lineEdit_num%1").arg(i);//根据多文本按钮的id设置文本框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
            QLineEdit *lineEdit = findChild<QLineEdit*>(lineEditName);//找到对应的多文本文本框控件
            if(lineEdit){//如果有文本则清空
                lineEdit->clear();
            }

            QString checkBoxName = QString("checkBox_num%1").arg(i);//根据多文本hex选择框的id设置选择框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
            QCheckBox *checkBox = findChild<QCheckBox*>(checkBoxName);//找到对应的多文本hex选择框控件
            if(checkBox->isChecked()){//如果是勾选状态则关闭
                checkBox->setChecked(false);
            }
        }
    }else{
        qDebug()<<"cancel reset";
    }
}





void Widget::on_pushButton_Save_clicked()//保存指令集
{
    QString fileName = QFileDialog::getSaveFileName(this,tr("Save File"),"F:/Qt_code",tr("Text (*.txt)"));//标准文件操作，获取文件名
    if(!fileName.isEmpty()){
        QFile qFile(fileName);
        if(qFile.open(QIODevice::ReadWrite | QIODevice::Text)){
            QTextStream out(&qFile);
            for(int i = 1;i<=9;i++){
                QString checkBoxName = QString("checkBox_num%1").arg(i);//根据多文本hex选择框的id设置选择框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
                QCheckBox *checkBox = findChild<QCheckBox*>(checkBoxName);//找到对应的多文本hex选择框控件

                QString lineEditName = QString("lineEdit_num%1").arg(i);//根据多文本按钮的id设置文本框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
                QLineEdit *lineEdit = findChild<QLineEdit*>(lineEditName);//找到对应的多文本文本框控件

                out<<checkBox->isChecked()<<","<<lineEdit->text()<<"\n";//按照 0,11这种格式保存
            }
        }
    }
}




void Widget::on_pushButton_Load_clicked()//加载指令集
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open File"),"F:/Qt_code",tr("Text (*.txt)"));//标准文件操作，获取文件名
    if(!fileName.isEmpty()){
        QFile qFile(fileName);
        if(qFile.open(QIODevice::ReadWrite | QIODevice::Text)){
            QTextStream in(&qFile);
            int i = 1;
            while(!in.atEnd() && i<=9){
                QString line = in.readLine();//逐行获取指令文本
                QStringList parts = line.split(",");//用，进行分割，分成两部分的字符串数组，前一部分是checkbox选择情况，后一部分是文本内容
                QString checkBoxName = QString("checkBox_num%1").arg(i);//根据多文本hex选择框的id设置选择框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
                QCheckBox *checkBox = findChild<QCheckBox*>(checkBoxName);//找到对应的多文本hex选择框控件

                QString lineEditName = QString("lineEdit_num%1").arg(i);//根据多文本按钮的id设置文本框名字，这个名字在ui布局控件中设置，所以只需要更改后一位的数字即可
                QLineEdit *lineEdit = findChild<QLineEdit*>(lineEditName);//找到对应的多文本文本框控件
                if(parts.count()==2){//如果是两部分，说明文本不为空
                    checkBox->setChecked(parts[0].toInt());//根据parts数组第一部分设置checkBox选择情况
                    lineEdit->setText(parts[1]);//第二部分设置文本框内容
                }
                i++;
            }
        }
    }
}

