#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_key_missing_warning.h"

#include <QMainWindow>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QtSql>
#include <QStandardItemModel>
#include <QTreeView>

#include <openssl/evp.h>

extern const char* KEY; // Declaration

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateCategoryBox();

    void loadCategoriesAndElements();

    void on_addCategoryButton_clicked();

    void on_addItemButton_clicked();

    void on_tableView_clicked(const QModelIndex &index);

    void on_deleteElementButton_clicked();

    void deleteCategory(int categoryId);

    void deleteElement(int elementId);

    void showAboutKeyDialog();

    void on_generateButton_clicked();

    void on_auditPasswordButton_clicked();

    void on_encryptionKeyButton_clicked();

    void on_learnMoreKeyMissingButton_clicked();

    void loadEncryptDecryptKey();

private:
    Ui::MainWindow *ui;

    QSqlDatabase db;

    QSqlQuery *query;

    QSqlTableModel *model;

    QStandardItemModel *treeModel;

    QComboBox *categoryComboBox;

    QStandardItemModel *categoryModel;

    QSqlTableModel *elementsModel;

    QModelIndex selectedIndex;

    // Encryption and Decryption functions
    QByteArray encryptData(const QByteArray &data);
    QByteArray decryptData(const QByteArray &ciphertext);

    QByteArray encryptionKey;

    void initializeDatabase();

    void updateCategoryTreeView();

    int selectedCategoryId;

    int row;
};
#endif // MAINWINDOW_H
