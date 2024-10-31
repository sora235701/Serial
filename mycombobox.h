#ifndef MYCOMBOBOX_H
#define MYCOMBOBOX_H

#include <QComboBox>
#include <QWidget>

class MyComboBox : public QComboBox
{
     Q_OBJECT
public:
    MyComboBox();
    MyComboBox(QWidget *parent = nullptr);
signals:
    void popupShown();

protected:
    void showPopup() override;
};

#endif // MYCOMBOBOX_H
