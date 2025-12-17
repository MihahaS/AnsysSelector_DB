#ifndef MATERIALIMPORTDIALOG_H
#define MATERIALIMPORTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMap>
#include "database.h"
#include "materialparser.h"

class MaterialImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialImportDialog(Database *database, QWidget *parent = nullptr);
    ~MaterialImportDialog();

private slots:
    void browseDirectory();
    void startImport();

private:
    Database *db;

    QLineEdit *directoryEdit;
    QTextEdit *logTextEdit;
    QLabel *statusLabel;
    QLabel *filesCountLabel;
    QLabel *materialsCountLabel;
    QPushButton *importButton;

    void setupUI();
    void checkDirectory(const QString &dirPath);
    ParsedMaterial parseMatML(const QString &filePath);
    bool importMaterialToDatabase(const ParsedMaterial &material);
    void logMessage(const QString &message);
};

#endif // MATERIALIMPORTDIALOG_H
