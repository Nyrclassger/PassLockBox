#include "key_missing_warning.h"
#include "ui_key_missing_warning.h"

#include <QFile>

key_missing_warning::key_missing_warning(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::key_missing_warning)
{
    ui->setupUi(this);
    setFixedSize(521, 205);
}

key_missing_warning::~key_missing_warning()
{
    delete ui;
}

void key_missing_warning::on_encryptionKeyButton_clicked()
{
    QString keyData = ui->encryptionKeyEdit->text().trimmed(); // Remove leading and trailing spaces from the input

    if (keyData.isEmpty()) {
        // Empty value, set an error message
        ui->encryptionKeyEdit->setStyleSheet("border: 1px solid red;"); // Set a red border
        ui->encryptionKeyEdit->setPlaceholderText("Error: Key cannot be empty"); // Display an error message in the input field
        return; // Exit the function
    }

    QFile file("key.dat");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << keyData;
        file.close();
        qDebug() << "Key data saved to key.dat";

        // Reset styles and error text if the key is successfully saved
        ui->encryptionKeyEdit->setStyleSheet(""); // Remove styles
        ui->encryptionKeyEdit->setPlaceholderText(""); // Remove the error message text
    } else {
        qDebug() << "Failed to create or open key.dat: " << file.errorString();
    }
}
