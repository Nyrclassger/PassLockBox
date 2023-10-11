#ifndef KEY_MISSING_WARNING_H
#define KEY_MISSING_WARNING_H

#include <QWidget>

namespace Ui {
class key_missing_warning;
}

class key_missing_warning : public QWidget
{
    Q_OBJECT

public:
    explicit key_missing_warning(QWidget *parent = nullptr);
    ~key_missing_warning();

private slots:
    void on_generateButton_clicked();

    void on_encryptionKeyButton_clicked();

private:
    Ui::key_missing_warning *ui;
};

#endif // KEY_MISSING_WARNING_H
