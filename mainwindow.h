#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QProgressDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include "database.h"
#include "fileparser.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Вкладка "Результаты расчетов"
    void loadResultsFile();
    void updateResultsTable();
    void filterByModel();
    void filterByCalculationType();
    void filterByMaterial();
    void exportResults();

    // Вкладка "Материалы"
    void importMatMLMaterials();
    void refreshMaterialsList();
    void showMaterialDetails(const QString &materialName);
    void onMaterialSelected(int row);
    void searchMaterials(const QString &searchText);
    void editMaterialProperty();
    void deleteMaterial();
    void exportMaterialData();
    void showMaterialStatistics();

    // Контекстное меню материалов
    void showMaterialContextMenu(const QPoint &pos);

private:
    void setupUI();
    void setupConnections();
    void setupResultsTab(QWidget *parent);
    void setupMaterialsTab(QWidget *parent);

    void loadModels();
    void loadCalculationTypes();
    void showResults(const QList<QVector<QVariant>>& results);

    // Методы для работы с материалами
    void displayMaterialProperties(const QString &materialName);
    void displayAllMaterials();
    void updateMaterialStats();

    Database *db;
    FileParser *parser;

    // UI элементы для вкладки "Результаты расчетов"
    QTabWidget *mainTabWidget;

    // Вкладка "Результаты"
    QTableWidget *resultsTable;
    QComboBox *modelComboBox;
    QComboBox *calcTypeComboBox;
    QPushButton *loadFileButton;
    QPushButton *exportButton;

    // Вкладка "Материалы"
    QListWidget *materialsListWidget;
    QTableWidget *materialPropertiesTable;
    QTextEdit *materialDescriptionEdit;
    QLineEdit *materialSearchEdit;
    QPushButton *importMaterialsButton;
    QPushButton *refreshMaterialsButton;
    QPushButton *deleteMaterialButton;
    QPushButton *exportMaterialButton;
    QPushButton *statsButton;

    // Статистика
    QLabel *statsLabel;
};

#endif // MAINWINDOW_H
