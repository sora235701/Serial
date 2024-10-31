#include "mycombobox.h"


MyComboBox::MyComboBox() {

}

MyComboBox::MyComboBox(QWidget *parent): QComboBox(parent)
{
}


void MyComboBox::showPopup()
{
    QComboBox::showPopup();
    emit popupShown();
}
