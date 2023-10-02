#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSqlRecord>
#include <QSqlError>
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTreeView>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>

#include <openssl/evp.h>

const char* KEY; // Define and initialize the KEY variable

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Load the encryption key from key.dat file
    QFile keyFile("key.dat");
    if (keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&keyFile);
        KEY = in.readAll().toUtf8().constData(); // Read the key and convert to const char*
        keyFile.close();

        ui->encryptionKeyEdit->setText(KEY);
        qDebug() << "Encryption key loaded from key.dat";
    } else {
        qDebug() << "Failed to open key.dat: " << keyFile.errorString();
    }

    initializeDatabase();

    ui->treeView->setModel(model);

    // Loading data from the database
    loadCategoriesAndElements();
    updateCategoryBox();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete query;
    delete elementsModel;
}

void MainWindow::initializeDatabase()
{
    // Initialize the SQLite database
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("./fileDB.db");

    if (db.open()) {
        qDebug() << "Database opened successfully";
    } else {
        qDebug() << "Failed to open database";
        // Handle the error, perhaps show a message to the user
    }

    query = new QSqlQuery(db);

    // Create tables if they do not exist
    query->exec("CREATE TABLE IF NOT EXISTS Categories ("
                "category_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "category_name TEXT)");

    query->exec("CREATE TABLE IF NOT EXISTS Elements ("
                "element_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "element_name TEXT,"
                "element_login TEXT,"
                "element_password TEXT,"
                "category_id INTEGER)");

    // Create and configure the model for displaying data
    model = new QSqlTableModel(this);
    model->setTable("Categories"); // Display categories in QTreeView
    model->select();

    ui->treeView->setModel(model);

    // Load data from the database
    loadCategoriesAndElements();
    updateCategoryBox();
}

QByteArray encryptData(const QByteArray &data);

QByteArray decryptData(const QByteArray &ciphertext);

void MainWindow::updateCategoryBox() {
    ui->categoryBox->clear(); // Clearing the combobox before update

    QSqlQueryModel *categoryModel = new QSqlQueryModel(this);
    categoryModel->setQuery("SELECT category_name FROM Categories", db); // Select categories from the database

    // Clear the combobox before updating
    ui->categoryBox->setModel(categoryModel);
}

void MainWindow::loadCategoriesAndElements()
{
    QStandardItemModel* treeModel = new QStandardItemModel();

    // Set the number of columns for treeModel (3)
    treeModel->setColumnCount(3);
    treeModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Login" << "Password");

    ui->treeView->setColumnWidth(0, 140);
    ui->treeView->setColumnWidth(1, 140);
    ui->treeView->setColumnWidth(2, 140);

    // Get all records from the Categories model
    if (model->select()) {
        while (model->canFetchMore()) {
            model->fetchMore();
        }

        // Iterate through all category records
        for (int i = 0; i < model->rowCount(); ++i) {
            int category_id = model->record(i).value("category_id").toInt();
            QString categoryName = model->record(i).value("category_name").toString();

            qDebug() << "Loading category:" << categoryName; // Add this message for debugging

            // Create a category item
            QStandardItem* categoryItem = new QStandardItem(categoryName); // Set the category text
            treeModel->appendRow(categoryItem);

            // Load elements belonging to this category
            QSqlTableModel* elementsModel = new QSqlTableModel(this);
            elementsModel->setTable("Elements");
            elementsModel->setFilter(QString("category_id = %1").arg(category_id));
            elementsModel->select();

            // Iterate through elements and add them to the category
            for (int j = 0; j < elementsModel->rowCount(); ++j) {
                QByteArray encryptedName = elementsModel->record(j).value("element_name").toByteArray();
                QByteArray encryptedLogin = elementsModel->record(j).value("element_login").toByteArray();
                QByteArray encryptedPassword = elementsModel->record(j).value("element_password").toByteArray();

                // Decrypt name, login, and password before displaying
                QString elementName = QString::fromUtf8(decryptData(encryptedName));
                QString elementLogin = QString::fromUtf8(decryptData(encryptedLogin));
                QString elementPassword = QString::fromUtf8(decryptData(encryptedPassword));

                // Create an element item
                QList<QStandardItem*> elementRow;
                elementRow << new QStandardItem(elementName)
                           << new QStandardItem(elementLogin)
                           << new QStandardItem(elementPassword);

                categoryItem->appendRow(elementRow);
            }
        }
    } else {
        qDebug() << "Failed to select categories from the database";
    }

    // Set the model for QTreeView
    ui->treeView->setModel(treeModel);
}

void MainWindow::on_addCategoryButton_clicked()
{
    QString categoryName = ui->categoryEdit->text();

    if (!categoryName.isEmpty()) {
        QSqlRecord record = model->record();
        record.setValue("category_name", categoryName);

        if (model->insertRecord(-1, record)) { // Insert a new record into the model
            if (model->submitAll()) { // Save changes to the database
                ui->categoryEdit->clear(); // Clear the input field
                loadCategoriesAndElements(); // Update the display in QTreeView
                updateCategoryBox();
            } else {
                qDebug() << "Failed to submit changes to the database";
            }
        } else {
            qDebug() << "Failed to insert a new category into the model";
        }
    } else {
        qDebug() << "Category name cannot be empty";
    }

    model->select();
}

void MainWindow::on_addItemButton_clicked()
{
    // Get the selected category from the combo box
    QString selectedCategory = ui->categoryBox->currentText();

    // Get values from the input fields
    QString elementName = ui->nameEdit->text();
    QString elementLogin = ui->loginEdit->text();
    QString elementPassword = ui->passEdit->text();

    // Check that category and values are not empty
    if (!selectedCategory.isEmpty() && !elementName.isEmpty() && !elementLogin.isEmpty() && !elementPassword.isEmpty()) {
        // Find the category ID by name
        QSqlQuery query(db);
        query.prepare("SELECT category_id FROM Categories WHERE category_name = :categoryName");
        query.bindValue(":categoryName", selectedCategory);

        if (query.exec() && query.first()) {
            int categoryId = query.value(0).toInt();

            // Encrypt elements before saving
            QByteArray encryptedName = encryptData(elementName.toUtf8());
            QByteArray encryptedLogin = encryptData(elementLogin.toUtf8());
            QByteArray encryptedPassword = encryptData(elementPassword.toUtf8());

            // Create and initialize elementsModel
            elementsModel = new QSqlTableModel(this);
            elementsModel->setTable("Elements");
            elementsModel->select();

            // Insert a new element into the Elements table
            QSqlRecord record = elementsModel->record();
            record.setValue("element_name", encryptedName);
            record.setValue("element_login", encryptedLogin);
            record.setValue("element_password", encryptedPassword);
            record.setValue("category_id", categoryId);

            if (elementsModel->insertRecord(-1, record) && elementsModel->submitAll()) {
                // Clear input fields after successful insertion
                ui->nameEdit->clear();
                ui->loginEdit->clear();
                ui->passEdit->clear();

                // Reload categories and elements to update the display
                loadCategoriesAndElements();
            } else {
                qDebug() << "Failed to insert a new element into the model";
            }
        } else {
            qDebug() << "Failed to find category ID";
        }
    } else {
        qDebug() << "Category and input fields cannot be empty";
    }
}

void MainWindow::on_tableView_clicked(const QModelIndex &index)
{
    QModelIndex categoryIndex = index.parent(); // Get the parent category index
    if (categoryIndex.isValid()) {
        // This is an element inside a category, set visibility for "Login" and "Password"
        ui->treeView->setColumnHidden(1, false); // Login
        ui->treeView->setColumnHidden(2, false); // Password
    } else {
        // This is a category, hide "Login" and "Password"
        ui->treeView->setColumnHidden(1, true); // Login
        ui->treeView->setColumnHidden(2, true); // Password
    }
    row = index.row();

    // Add code to handle changes in cells
    QModelIndex modelIndex = model->index(row, index.column(), categoryIndex);
    if (modelIndex.isValid()) {
        // This is a valid cell in the data model
        int categoryId = model->record(row).value("category_id").toInt();
        QString columnName = model->headerData(index.column(), Qt::Horizontal).toString();

        // Get the old value from the model
        QString oldValue = model->data(modelIndex, Qt::DisplayRole).toString();

        // Open a dialog to make changes
        QString newValue = QInputDialog::getText(this, "Edit Value", "Edit " + columnName, QLineEdit::Normal, oldValue);

        if (!newValue.isEmpty() && newValue != oldValue) {
            // Update the data model
            model->setData(modelIndex, newValue);

            // Encrypt the new value before saving it to the database
            QByteArray encryptedValue = encryptData(newValue.toUtf8());

            // Update the record in the database
            QSqlQuery updateQuery(db);
            updateQuery.prepare("UPDATE Elements SET " + columnName + " = :value WHERE category_id = :categoryId AND element_name = :elementName");
            updateQuery.bindValue(":value", encryptedValue);
            updateQuery.bindValue(":categoryId", categoryId);
            updateQuery.bindValue(":elementName", oldValue);

            if (updateQuery.exec()) {
                qDebug() << columnName << " updated successfully";
            } else {
                qDebug() << "Failed to update " << columnName;
            }
        }
    }
}

void MainWindow::on_deleteElementButton_clicked()
{
    // Get a list of selected indexes from QTreeView
    QModelIndexList selectedIndexes = ui->treeView->selectionModel()->selectedIndexes();

    // Create lists to store unique categories and elements to delete
    QList<int> categoriesToDelete;
    QList<int> elementsToDelete;

    foreach (QModelIndex index, selectedIndexes) {
        if (index.column() == 0) { // Check if the column with the name is selected
            int row = index.row();
            QModelIndex parentIndex = index.parent();

            if (!parentIndex.isValid()) {
                // This is a category
                int categoryId = model->record(row).value("category_id").toInt();
                categoriesToDelete.append(categoryId);
            } else {
                // This is an element
                int elementId = elementsModel->record(row).value("element_id").toInt();
                elementsToDelete.append(elementId);
            }
        }
    }

    // Delete selected categories and elements from the database
    foreach (int categoryId, categoriesToDelete) {
        deleteCategory(categoryId);
    }

    foreach (int elementId, elementsToDelete) {
        deleteElement(elementId);
    }

    // Update the display in QTreeView after deletion
    loadCategoriesAndElements();
    updateCategoryBox();
}

void MainWindow::deleteCategory(int categoryId)
{
    // Delete the category from the database
    QSqlQuery deleteCategoryQuery(db);
    deleteCategoryQuery.prepare("DELETE FROM Categories WHERE category_id = :categoryId");
    deleteCategoryQuery.bindValue(":categoryId", categoryId);
    if (deleteCategoryQuery.exec()) {
        qDebug() << "Category deleted successfully";
    } else {
        qDebug() << "Failed to delete category";
    }
}

void MainWindow::deleteElement(int elementId)
{
    // Delete the element from the database
    QSqlQuery deleteElementQuery(db);
    deleteElementQuery.prepare("DELETE FROM Elements WHERE element_id = :elementId");
    deleteElementQuery.bindValue(":elementId", elementId);
    if (deleteElementQuery.exec()) {
        qDebug() << "Element deleted successfully";
    } else {
        qDebug() << "Failed to delete element";
    }
}

void MainWindow::on_generateButton_clicked()
{
    // Get password generation settings
    bool includeLetters = ui->includeLettersBox->isChecked();
    bool includeNumbers = ui->includeNumbersBox->isChecked();
    bool includeSpecialChars = ui->includeSpecialCharactersBox->isChecked();
    bool includeUpperCase = ui->includingUpperCaseLettersBox->isChecked();
    QString excludedChars = ui->excludeSpecificCharactersEdit->text();

    int passwordLength = ui->lengthPasswordEdit->text().toInt();

    if (!includeLetters && !includeNumbers && !includeSpecialChars && !includeUpperCase)
    {
        ui->generatePasswordEdit->setText("???");
        return;
    }

    // Define available characters for generation
    QString availableChars = "";
    if (includeLetters)
    {
        if (includeUpperCase)
            availableChars += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        availableChars += "abcdefghijklmnopqrstuvwxyz";
    }
    if (includeNumbers)
        availableChars += "0123456789";
    if (includeSpecialChars)
        availableChars += "!@#$%^&*()_-+=<>?";

    // Remove excluded characters from available characters
    for (const QChar& excludedChar : excludedChars)
    {
        availableChars.remove(excludedChar);
    }

    // Generate the password
    QString generatedPassword;

    for (int i = 0; i < passwordLength; ++i)
    {
        int randomIndex = QRandomGenerator::global()->bounded(availableChars.length());
        generatedPassword += availableChars.at(randomIndex);
    }

    // Display the generated password
    ui->generatePasswordEdit->setText(generatedPassword);
}

int calculatePasswordStrength(const QString& password)
{
    int length = password.length();
    int uppercaseCount = 0;
    int lowercaseCount = 0;
    int digitCount = 0;
    int specialCharCount = 0;

    for (QChar c : password)
    {
        if (c.isUpper())
            uppercaseCount++;
        else if (c.isLower())
            lowercaseCount++;
        else if (c.isDigit())
            digitCount++;
        else if (!c.isLetterOrNumber())
            specialCharCount++;
    }

    int strength = 0;
    strength += qMin(2, length) * 10;
    strength += qMin(2, uppercaseCount) * 10;
    strength += qMin(2, lowercaseCount) * 10;
    strength += qMin(2, digitCount) * 10;
    strength += qMin(2, specialCharCount) * 10;

    return strength;
}

void MainWindow::on_auditPasswordButton_clicked()
{
    QString password = ui->passwordAuditEdit->text();
    int strength = calculatePasswordStrength(password);

    ui->auditPasswordBar->setValue(strength);
}

void MainWindow::on_encryptionKeyButton_clicked()
{
    QString keyData = ui->encryptionKeyEdit->text();
    QFile file("key.dat");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << keyData;
        file.close();
        qDebug() << "Key data saved to key.dat";
    } else {
        qDebug() << "Failed to create or open key.dat: " << file.errorString();
    }
}


