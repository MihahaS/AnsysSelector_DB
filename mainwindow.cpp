#include "mainwindow.h"
#include "materialimportdialog.h"
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , db(new Database(this))
    , parser(new FileParser(this))
{
    // Инициализация базы данных
    if (!db->initDatabase()) {
        QMessageBox::critical(this, "Database Error",
                              "Failed to initialize database!");
        exit(1);
    }

    setupUI();
    setupConnections();

    // Загрузка начальных данных
    loadModels();
    loadCalculationTypes();
    updateResultsTable();
    refreshMaterialsList();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // Создание центрального виджета и layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Создаем вкладки
    mainTabWidget = new QTabWidget(this);

    // Вкладка 1: Результаты расчетов
    QWidget *resultsTab = new QWidget();
    setupResultsTab(resultsTab);
    mainTabWidget->addTab(resultsTab, "Результаты расчетов");

    // Вкладка 2: Материалы
    QWidget *materialsTab = new QWidget();
    setupMaterialsTab(materialsTab);
    mainTabWidget->addTab(materialsTab, "Материалы");

    mainLayout->addWidget(mainTabWidget);
    setCentralWidget(centralWidget);

    resize(1200, 800);
    setWindowTitle("База данных материалов и результатов расчетов");
}

void MainWindow::setupResultsTab(QWidget *parent)
{
    QVBoxLayout *layout = new QVBoxLayout(parent);

    // Панель управления
    QHBoxLayout *controlLayout = new QHBoxLayout();

    loadFileButton = new QPushButton("📁 Загрузить файл результатов", parent);
    loadFileButton->setIconSize(QSize(20, 20));
    exportButton = new QPushButton("📤 Экспорт результатов", parent);
    exportButton->setIconSize(QSize(20, 20));

    controlLayout->addWidget(loadFileButton);
    controlLayout->addWidget(exportButton);
    controlLayout->addStretch();

    layout->addLayout(controlLayout);

    // Панель фильтров
    QGroupBox *filterGroup = new QGroupBox("Фильтры", parent);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    filterLayout->addWidget(new QLabel("Модель:", parent));
    modelComboBox = new QComboBox(parent);
    modelComboBox->addItem("Все модели", "");
    modelComboBox->setMinimumWidth(200);
    filterLayout->addWidget(modelComboBox);

    filterLayout->addWidget(new QLabel("Вид расчета:", parent));
    calcTypeComboBox = new QComboBox(parent);
    calcTypeComboBox->addItem("Все типы", "");
    calcTypeComboBox->setMinimumWidth(200);
    filterLayout->addWidget(calcTypeComboBox);

    filterLayout->addStretch();

    layout->addWidget(filterGroup);

    // Таблица результатов
    resultsTable = new QTableWidget(parent);
    resultsTable->setColumnCount(4);
    QStringList headers = {"Модель", "Номер узла", "Вид расчета", "Значение"};
    resultsTable->setHorizontalHeaderLabels(headers);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable->setSortingEnabled(true);
    resultsTable->setAlternatingRowColors(true);

    // Настройка ширины колонок
    resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    resultsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    resultsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    resultsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    layout->addWidget(resultsTable, 1);
}

void MainWindow::setupMaterialsTab(QWidget *parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(parent);

    // Левая панель: Список материалов
    QWidget *leftPanel = new QWidget(parent);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    // Поиск материалов
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Поиск:", parent));
    materialSearchEdit = new QLineEdit(parent);
    materialSearchEdit->setPlaceholderText("Введите название материала...");
    searchLayout->addWidget(materialSearchEdit);

    leftLayout->addLayout(searchLayout);

    // Список материалов
    materialsListWidget = new QListWidget(parent);
    materialsListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    materialsListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(materialsListWidget, 1);

    // Панель кнопок для материалов
    QHBoxLayout *materialButtonsLayout = new QHBoxLayout();

    importMaterialsButton = new QPushButton("📥 Импорт MatML", parent);
    refreshMaterialsButton = new QPushButton("🔄 Обновить", parent);
    deleteMaterialButton = new QPushButton("🗑️ Удалить", parent);
    exportMaterialButton = new QPushButton("📤 Экспорт", parent);
    statsButton = new QPushButton("📊 Статистика", parent);

    deleteMaterialButton->setEnabled(false);
    exportMaterialButton->setEnabled(false);

    materialButtonsLayout->addWidget(importMaterialsButton);
    materialButtonsLayout->addWidget(refreshMaterialsButton);
    materialButtonsLayout->addWidget(deleteMaterialButton);
    materialButtonsLayout->addWidget(exportMaterialButton);
    materialButtonsLayout->addWidget(statsButton);
    materialButtonsLayout->addStretch();

    leftLayout->addLayout(materialButtonsLayout);

    // Статистика
    statsLabel = new QLabel(parent);
    statsLabel->setStyleSheet("color: gray; font-style: italic;");
    leftLayout->addWidget(statsLabel);

    // Правая панель: Свойства материала
    QWidget *rightPanel = new QWidget(parent);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    // Заголовок с названием материала
    QLabel *materialTitleLabel = new QLabel("Свойства материала", parent);
    materialTitleLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;");
    materialTitleLabel->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(materialTitleLabel);

    // Таблица свойств
    materialPropertiesTable = new QTableWidget(parent);
    materialPropertiesTable->setColumnCount(3);
    QStringList propHeaders = {"Свойство", "Значение", "Единица измерения"};
    materialPropertiesTable->setHorizontalHeaderLabels(propHeaders);
    materialPropertiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    materialPropertiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    materialPropertiesTable->setAlternatingRowColors(true);

    materialPropertiesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    materialPropertiesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    materialPropertiesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    rightLayout->addWidget(materialPropertiesTable, 1);

    // Описание/примечания
    QLabel *descLabel = new QLabel("Примечания:", parent);
    rightLayout->addWidget(descLabel);

    materialDescriptionEdit = new QTextEdit(parent);
    materialDescriptionEdit->setMaximumHeight(100);
    materialDescriptionEdit->setPlaceholderText("Добавьте примечания к материалу...");
    rightLayout->addWidget(materialDescriptionEdit);

    // Распределяем пространство
    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(rightPanel, 2);
}

void MainWindow::setupConnections()
{
    // Вкладка "Результаты расчетов"
    connect(loadFileButton, &QPushButton::clicked, this, &MainWindow::loadResultsFile);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportResults);

    connect(modelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterByModel);
    connect(calcTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterByCalculationType);

    // Вкладка "Материалы"
    connect(importMaterialsButton, &QPushButton::clicked,
            this, &MainWindow::importMatMLMaterials);
    connect(refreshMaterialsButton, &QPushButton::clicked,
            this, &MainWindow::refreshMaterialsList);
    connect(deleteMaterialButton, &QPushButton::clicked,
            this, &MainWindow::deleteMaterial);
    connect(exportMaterialButton, &QPushButton::clicked,
            this, &MainWindow::exportMaterialData);
    connect(statsButton, &QPushButton::clicked,
            this, &MainWindow::showMaterialStatistics);

    connect(materialSearchEdit, &QLineEdit::textChanged,
            this, &MainWindow::searchMaterials);

    connect(materialsListWidget, &QListWidget::currentRowChanged,
            this, &MainWindow::onMaterialSelected);
    connect(materialsListWidget, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showMaterialContextMenu);

    connect(materialPropertiesTable, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::editMaterialProperty);
}

void MainWindow::loadResultsFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Open Results File",
                                                    "",
                                                    "Text Files (*.txt *.csv);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    // Запрос имени модели
    QString modelName = QInputDialog::getText(this,
                                              "Model Name",
                                              "Enter model name:",
                                              QLineEdit::Normal,
                                              QFileInfo(fileName).baseName());

    if (modelName.isEmpty()) {
        return;
    }

    // Добавляем модель в базу данных
    db->addModel(modelName);

    // Парсим файл
    QString error;
    ParsedData data = parser->parseFile(fileName, error);

    if (!error.isEmpty()) {
        QMessageBox::warning(this, "Parse Error", error);
    }

    if (data.nodeValues.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No valid data found in file or file is empty");
        return;
    }

    // Добавляем тип расчета, если его нет
    db->addCalculationType(data.calculationType, data.unit);

    // Сохраняем результаты в базу данных
    int count = 0;
    int errorCount = 0;

    for (auto it = data.nodeValues.begin(); it != data.nodeValues.end(); ++it) {
        if (db->addCalculationResult(modelName, it.key(), data.calculationType, it.value())) {
            count++;
        } else {
            errorCount++;
            qDebug() << "Failed to add node:" << it.key() << "value:" << it.value();
        }
    }

    // Обновляем UI
    loadModels();
    loadCalculationTypes();
    updateResultsTable();

    QString message = QString("Loaded %1 nodes from file").arg(count);
    if (errorCount > 0) {
        message += QString("\nFailed to load %1 nodes").arg(errorCount);
    }

    QMessageBox::information(this, "Success", message);
}

void MainWindow::updateResultsTable()
{
    QString modelFilter = modelComboBox->currentData().toString();
    QString calcTypeFilter = calcTypeComboBox->currentData().toString();

    // Получаем все результаты
    auto allResults = db->getCalculationResults(modelFilter);

    // Фильтруем результаты
    QList<QVector<QVariant>> filteredResults;

    for (const auto &row : allResults) {
        if (!calcTypeFilter.isEmpty() && row[2].toString() != calcTypeFilter) {
            continue;
        }
        filteredResults.append(row);
    }

    // Показываем отфильтрованные результаты
    showResults(filteredResults);
}

void MainWindow::showResults(const QList<QVector<QVariant>>& results)
{
    resultsTable->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        const QVector<QVariant>& row = results[i];

        for (int j = 0; j < row.size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(row[j].toString());
            resultsTable->setItem(i, j, item);
        }
    }

    resultsTable->resizeColumnsToContents();
}

void MainWindow::filterByModel()
{
    updateResultsTable();
}

void MainWindow::filterByCalculationType()
{
    updateResultsTable();
}

void MainWindow::filterByMaterial()
{
    updateResultsTable();
}

void MainWindow::exportResults()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Export Results",
                                                    "",
                                                    "CSV Files (*.csv);;Text Files (*.txt)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open file for writing");
        return;
    }

    QTextStream out(&file);

    // Заголовок
    out << "Model,Node Number,Calculation Type,Value\n";

    // Данные
    auto results = db->getCalculationResults();
    for (const auto &row : results) {
        out << row[0].toString() << ","
            << row[1].toString() << ","
            << row[2].toString() << ","
            << row[3].toString() << ","
            << row[4].toString() << "\n";
    }

    file.close();
    QMessageBox::information(this, "Success", "Results exported successfully");
}

void MainWindow::importMatMLMaterials()
{
    MaterialImportDialog dialog(db, this);

    if (dialog.exec() == QDialog::Accepted) {
        refreshMaterialsList();
        QMessageBox::information(this, "Успех", "Материалы успешно импортированы");
    }
}

void MainWindow::refreshMaterialsList()
{
    materialsListWidget->clear();

    QStringList materials = db->getAllMaterials();
    for (const QString &material : materials) {
        QListWidgetItem *item = new QListWidgetItem(material);
        materialsListWidget->addItem(item);
    }

    updateMaterialStats();
}

void MainWindow::onMaterialSelected(int row)
{
    if (row < 0) {
        materialPropertiesTable->setRowCount(0);
        materialDescriptionEdit->clear();
        deleteMaterialButton->setEnabled(false);
        exportMaterialButton->setEnabled(false);
        return;
    }

    QListWidgetItem *item = materialsListWidget->item(row);
    if (!item) return;

    QString materialName = item->text();
    showMaterialDetails(materialName);

    deleteMaterialButton->setEnabled(true);
    exportMaterialButton->setEnabled(true);
}

void MainWindow::showMaterialDetails(const QString &materialName)
{
    materialPropertiesTable->setRowCount(0);

    // Получаем свойства материала с единицами измерения
    auto properties = db->getMaterialPropertiesWithUnits(materialName);

    int row = 0;
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        QString propertyName = it.key();
        QString unit = it.value().first;
        double value = it.value().second;

        materialPropertiesTable->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(propertyName);
        QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(value, 'g', 6));
        QTableWidgetItem *unitItem = new QTableWidgetItem(unit);

        materialPropertiesTable->setItem(row, 0, nameItem);
        materialPropertiesTable->setItem(row, 1, valueItem);
        materialPropertiesTable->setItem(row, 2, unitItem);

        row++;
    }

    materialPropertiesTable->resizeRowsToContents();
}

void MainWindow::searchMaterials(const QString &searchText)
{
    QStringList materials = db->getAllMaterials();
    materialsListWidget->clear();

    for (const QString &material : materials) {
        if (searchText.isEmpty() || material.contains(searchText, Qt::CaseInsensitive)) {
            materialsListWidget->addItem(material);
        }
    }

    updateMaterialStats();
}

void MainWindow::editMaterialProperty()
{
    int row = materialPropertiesTable->currentRow();
    if (row < 0) return;

    QListWidgetItem *materialItem = materialsListWidget->currentItem();
    if (!materialItem) return;

    QString materialName = materialItem->text();
    QString propertyName = materialPropertiesTable->item(row, 0)->text();
    QString currentValueStr = materialPropertiesTable->item(row, 1)->text();
    QString unit = materialPropertiesTable->item(row, 2)->text();

    bool ok;
    double currentValue = currentValueStr.toDouble(&ok);
    if (!ok) currentValue = 0.0;

    double newValue = QInputDialog::getDouble(this,
                                              "Редактирование свойства",
                                              QString("Новое значение для '%1' (%2):")
                                                  .arg(propertyName).arg(unit),
                                              currentValue,
                                              -1e9, 1e9, 6, &ok);

    if (ok && db->updateMaterialProperty(materialName, propertyName, newValue)) {
        // Обновляем в таблице
        materialPropertiesTable->item(row, 1)->setText(QString::number(newValue, 'g', 6));
        QMessageBox::information(this, "Успех", "Свойство обновлено");
    } else if (ok) {
        QMessageBox::warning(this, "Ошибка", "Не удалось обновить свойство");
    }
}

void MainWindow::deleteMaterial()
{
    QListWidgetItem *currentItem = materialsListWidget->currentItem();
    if (!currentItem) return;

    QString materialName = currentItem->text();

    int reply = QMessageBox::question(this,
                                      "Подтверждение удаления",
                                      QString("Удалить материал '%1'?\nВсе связанные свойства также будут удалены.")
                                          .arg(materialName),
                                      QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (db->removeMaterial(materialName)) {
            delete materialsListWidget->takeItem(materialsListWidget->currentRow());
            materialPropertiesTable->setRowCount(0);
            materialDescriptionEdit->clear();

            updateMaterialStats();

            QMessageBox::information(this, "Успех", "Материал удален");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить материал");
        }
    }
}

void MainWindow::exportMaterialData()
{
    QListWidgetItem *currentItem = materialsListWidget->currentItem();
    if (!currentItem) return;

    QString materialName = currentItem->text();
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Экспорт материала",
                                                    QString("%1.csv").arg(materialName),
                                                    "CSV Files (*.csv);;Text Files (*.txt)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для записи");
        return;
    }

    QTextStream out(&file);

    // Заголовок
    out << "Материал: " << materialName << "\n";
    out << "Свойство;Значение;Единица измерения\n";

    // Данные
    auto properties = db->getMaterialProperties(materialName);
    for (const auto &property : properties) {
        out << property.first << ";"
            << QString::number(property.second, 'g', 6) << ";"
            << "\n";
    }

    file.close();
    QMessageBox::information(this, "Успех", "Данные материала экспортированы");
}

void MainWindow::showMaterialStatistics()
{
    QStringList materials = db->getAllMaterials();
    int totalMaterials = materials.size();

    // Получаем все материалы со свойствами
    auto allMaterials = db->getAllMaterialsWithProperties();
    int totalProperties = 0;

    // Считаем общее количество свойств
    for (auto it = allMaterials.begin(); it != allMaterials.end(); ++it) {
        totalProperties += it->size();
    }

    // Считаем уникальные названия свойств
    QSet<QString> uniqueProperties;
    for (auto it = allMaterials.begin(); it != allMaterials.end(); ++it) {
        for (auto propIt = it->begin(); propIt != it->end(); ++propIt) {
            uniqueProperties.insert(propIt.key());
        }
    }

    QString stats = QString("Статистика материалов:\n"
                            "Всего материалов: %1\n"
                            "Всего свойств: %2\n"
                            "Уникальных свойств: %3\n"
                            "Среднее свойств на материал: %4")
                        .arg(totalMaterials)
                        .arg(totalProperties)
                        .arg(uniqueProperties.size())
                        .arg(totalMaterials > 0 ?
                                 QString::number((double)totalProperties / totalMaterials, 'f', 2) : "0");

    // Если есть материалы, показываем детальную статистику
    if (totalMaterials > 0) {
        stats += "\n\nМатериалы со свойствами:";

        // Ограничим вывод, если материалов много
        int maxDisplay = qMin(10, totalMaterials);
        int count = 0;

        for (auto it = allMaterials.begin(); it != allMaterials.end() && count < maxDisplay; ++it) {
            stats += QString("\n  • %1: %2 свойств")
                         .arg(it.key())
                         .arg(it->size());
            count++;
        }

        if (totalMaterials > maxDisplay) {
            stats += QString("\n  ... и еще %1 материалов").arg(totalMaterials - maxDisplay);
        }
    }

    QMessageBox::information(this, "Статистика материалов", stats);
}

void MainWindow::showMaterialContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = materialsListWidget->itemAt(pos);
    if (!item) return;

    QMenu contextMenu(this);

    QAction *showDetailsAction = contextMenu.addAction("📋 Показать свойства");
    QAction *deleteAction = contextMenu.addAction("🗑️ Удалить");
    QAction *exportAction = contextMenu.addAction("📤 Экспорт");

    QAction *selectedAction = contextMenu.exec(materialsListWidget->mapToGlobal(pos));

    if (selectedAction == showDetailsAction) {
        showMaterialDetails(item->text());
    } else if (selectedAction == deleteAction) {
        deleteMaterial();
    } else if (selectedAction == exportAction) {
        exportMaterialData();
    }
}

void MainWindow::updateMaterialStats()
{
    int count = materialsListWidget->count();
    statsLabel->setText(QString("Найдено материалов: %1").arg(count));
}

void MainWindow::loadModels()
{
    modelComboBox->clear();
    modelComboBox->addItem("Все модели", "");

    QStringList models = db->getAllModels();
    for (const QString &model : models) {
        modelComboBox->addItem(model, model);
    }
}

void MainWindow::loadCalculationTypes()
{
    calcTypeComboBox->clear();
    calcTypeComboBox->addItem("Все типы", "");

    auto types = db->getAllCalculationTypes();
    for (const auto &type : types) {
        calcTypeComboBox->addItem(type.first + " (" + type.second + ")", type.first);
    }
}

