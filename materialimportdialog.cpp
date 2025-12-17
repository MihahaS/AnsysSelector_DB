#include "materialimportdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDirIterator>
#include <QXmlStreamReader>
#include <QTextStream>
#include <QApplication>
#include <QProgressDialog>
#include <QDateTime>
#include <QDebug>

MaterialImportDialog::MaterialImportDialog(Database *database, QWidget *parent)
    : QDialog(parent), db(database)
{
    setWindowTitle("Импорт материалов из MatML");
    resize(700, 500);
    setModal(true);

    setupUI();
}

MaterialImportDialog::~MaterialImportDialog()
{
}

void MaterialImportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Заголовок
    QLabel *titleLabel = new QLabel("Импорт материалов из файлов MatML", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Панель выбора директории
    QGroupBox *dirGroup = new QGroupBox("Выбор директории с файлами", this);
    QVBoxLayout *dirLayout = new QVBoxLayout(dirGroup);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(new QLabel("Директория:", this));

    directoryEdit = new QLineEdit(this);
    directoryEdit->setPlaceholderText("Выберите папку с файлами .xml или .matml");
    pathLayout->addWidget(directoryEdit, 1);

    QPushButton *browseButton = new QPushButton("Обзор...", this);
    connect(browseButton, &QPushButton::clicked, this, &MaterialImportDialog::browseDirectory);
    pathLayout->addWidget(browseButton);

    dirLayout->addLayout(pathLayout);

    // Информация о поддерживаемых форматах
    QLabel *infoLabel = new QLabel("Поддерживаемые форматы: .xml, .matml\n"
                                   "Файлы должны соответствовать стандарту MatML", this);
    infoLabel->setStyleSheet("color: gray; font-style: italic; padding: 5px;");
    dirLayout->addWidget(infoLabel);

    mainLayout->addWidget(dirGroup);

    // Лог импорта
    QGroupBox *logGroup = new QGroupBox("Лог импорта", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);
    logTextEdit->setFont(QFont("Courier", 9));
    logLayout->addWidget(logTextEdit);

    mainLayout->addWidget(logGroup, 1);

    // Статистика
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->addWidget(new QLabel("Статус:", this));

    statusLabel = new QLabel("Ожидание начала импорта", this);
    statusLabel->setStyleSheet("font-weight: bold;");
    statsLayout->addWidget(statusLabel, 1);

    filesCountLabel = new QLabel("Файлов: 0", this);
    statsLayout->addWidget(filesCountLabel);

    materialsCountLabel = new QLabel("Материалов: 0", this);
    statsLayout->addWidget(materialsCountLabel);

    mainLayout->addLayout(statsLayout);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *clearLogButton = new QPushButton("Очистить лог", this);
    connect(clearLogButton, &QPushButton::clicked, [this]() {
        logTextEdit->clear();
    });

    buttonLayout->addWidget(clearLogButton);
    buttonLayout->addStretch();

    importButton = new QPushButton("Начать импорт", this);
    importButton->setDefault(true);
    importButton->setStyleSheet("padding: 8px; font-weight: bold;");
    connect(importButton, &QPushButton::clicked, this, &MaterialImportDialog::startImport);

    QPushButton *cancelButton = new QPushButton("Закрыть", this);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addWidget(importButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void MaterialImportDialog::browseDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    "Выберите папку с файлами MatML",
                                                    directoryEdit->text(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        directoryEdit->setText(QDir::toNativeSeparators(dir));

        // Автоматически проверяем файлы в директории
        checkDirectory(dir);
    }
}

void MaterialImportDialog::checkDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    QStringList filters = {"*.xml", "*.matml"};
    QStringList files = dir.entryList(filters, QDir::Files);

    filesCountLabel->setText(QString("Файлов: %1").arg(files.size()));

    if (files.isEmpty()) {
        statusLabel->setText("Файлы не найдены");
        statusLabel->setStyleSheet("color: orange; font-weight: bold;");
        importButton->setEnabled(false);
    } else {
        statusLabel->setText("Готово к импорту");
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
        importButton->setEnabled(true);
    }
}

void MaterialImportDialog::startImport()
{
    QString dirPath = directoryEdit->text().trimmed();

    if (dirPath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите директорию с файлами");
        return;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "Ошибка", "Директория не существует");
        return;
    }

    // Получаем список файлов
    QStringList filters = {"*.xml", "*.matml"};
    QStringList files = dir.entryList(filters, QDir::Files);

    if (files.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "В директории не найдены файлы .xml или .matml");
        return;
    }

    // Создаем прогресс-диалог
    QProgressDialog progress("Импорт материалов...", "Отмена", 0, files.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setWindowTitle("Импорт материалов");

    // Очищаем лог
    logTextEdit->clear();
    logMessage("=== Начало импорта ===");
    logMessage(QString("Директория: %1").arg(dirPath));
    logMessage(QString("Найдено файлов: %1").arg(files.size()));
    logMessage("---");

    // Отключаем кнопки
    importButton->setEnabled(false);

    // Счетчики
    int totalFiles = files.size();
    int processedFiles = 0;
    int importedMaterials = 0;
    int skippedFiles = 0;

    // Парсим каждый файл
    for (int i = 0; i < files.size(); ++i) {
        if (progress.wasCanceled()) {
            logMessage("\nИмпорт отменен пользователем");
            break;
        }

        QString fileName = files[i];
        QString filePath = dir.absoluteFilePath(fileName);

        progress.setLabelText(QString("Обработка файла %1 из %2: %3")
                                  .arg(i + 1).arg(totalFiles).arg(fileName));
        progress.setValue(i);
        QApplication::processEvents();

        // Парсим файл
        logMessage(QString("Обработка: %1").arg(fileName));

        ParsedMaterial material = parseMatML(filePath);

        if (material.isEmpty()) {
            logMessage(QString("  ✗ Не удалось распознать материал"));
            skippedFiles++;
        } else {
            logMessage(QString("  ✓ Материал: %1").arg(material.name));
            logMessage(QString("    Свойств: %1, Isotropic: %2")
                           .arg(material.values.size())
                           .arg(material.isotropic ? "да" : "нет"));

            // Импортируем в базу данных
            if (importMaterialToDatabase(material)) {
                importedMaterials++;
                logMessage(QString("    ✓ Успешно импортирован"));
            } else {
                logMessage(QString("    ✗ Ошибка импорта в БД"));
                skippedFiles++;
            }
        }

        processedFiles++;
        materialsCountLabel->setText(QString("Материалов: %1").arg(importedMaterials));
    }

    progress.setValue(totalFiles);

    // Итоговая статистика
    logMessage("\n=== Итог импорта ===");
    logMessage(QString("Обработано файлов: %1").arg(processedFiles));
    logMessage(QString("Импортировано материалов: %1").arg(importedMaterials));
    logMessage(QString("Пропущено файлов: %1").arg(skippedFiles));

    if (progress.wasCanceled()) {
        logMessage("Импорт был отменен");
        statusLabel->setText("Импорт отменен");
        statusLabel->setStyleSheet("color: orange; font-weight: bold;");
    } else {
        logMessage("Импорт завершен");
        statusLabel->setText(QString("Импортировано: %1 материалов").arg(importedMaterials));
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
    }

    // Включаем кнопки
    importButton->setEnabled(true);

    // Показываем итоговое сообщение
    if (!progress.wasCanceled()) {
        if (importedMaterials > 0) {
            QMessageBox::information(this, "Импорт завершен",
                                     QString("Успешно импортировано %1 материалов из %2 файлов")
                                         .arg(importedMaterials).arg(processedFiles));
        } else {
            QMessageBox::warning(this, "Импорт завершен",
                                 "Не удалось импортировать ни одного материала");
        }
    }
}

ParsedMaterial MaterialImportDialog::parseMatML(const QString &filePath)
{
    ParsedMaterial material;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return material;
    }

    QXmlStreamReader xml(&file);
    QString currentPropId;
    QString currentMetaId;
    bool inPropertyDetails = false;
    bool inPropertyData = false;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "Material") {
                // Начало описания материала
            }
            else if (xml.name() == "Name" && material.name.isEmpty()) {
                // Имя материала
                material.name = xml.readElementText().trimmed();
            }
            else if (xml.name() == "PropertyDetails") {
                inPropertyDetails = true;
                currentMetaId = xml.attributes().value("id").toString();
                material.meta[currentMetaId] = PropertyMeta();
            }
            else if (xml.name() == "PropertyData") {
                inPropertyData = true;
                currentPropId = xml.attributes().value("property").toString();
            }
            else if (xml.name() == "Data" && inPropertyData && !currentPropId.isEmpty()) {
                QString text = xml.readElementText().trimmed();

                // Преобразуем значение
                bool ok;
                double value = text.toDouble(&ok);
                if (ok) {
                    material.values[currentPropId] = value;
                }

                // Проверяем на Isotropic
                if (text.contains("Isotropic", Qt::CaseInsensitive)) {
                    material.isotropic = true;
                }
            }
            else if (xml.name() == "Name" && inPropertyDetails && !currentMetaId.isEmpty()) {
                QString propertyName = xml.readElementText().trimmed();
                if (!propertyName.isEmpty()) {
                    material.meta[currentMetaId].name = propertyName;
                }
            }
            else if (xml.name() == "Unit" && inPropertyDetails && !currentMetaId.isEmpty()) {
                xml.readNextStartElement(); // Name внутри Unit
                if (xml.name() == "Name") {
                    QString unit = xml.readElementText().trimmed();
                    material.meta[currentMetaId].unit = unit;
                }
            }
        }
        else if (token == QXmlStreamReader::EndElement) {
            if (xml.name() == "PropertyDetails") {
                inPropertyDetails = false;
                currentMetaId.clear();
            }
            else if (xml.name() == "PropertyData") {
                inPropertyData = false;
                currentPropId.clear();
            }
        }
    }

    if (xml.hasError()) {
        qDebug() << "XML parsing error:" << xml.errorString() << "in file:" << filePath;
    }

    file.close();

    // Переносим данные из meta в values, если они соответствуют
    for (auto it = material.meta.begin(); it != material.meta.end(); ++it) {
        QString metaId = it.key();
        if (!material.values.contains(metaId)) {
            // Пробуем найти значение по имени свойства
            QString propertyName = it.value().name;
            for (auto valueIt = material.values.begin(); valueIt != material.values.end(); ++valueIt) {
                // Если ID значения соответствует или имя свойства совпадает
                if (valueIt.key().contains(metaId) ||
                    material.meta.contains(valueIt.key())) {
                    // Значение уже связано, пропускаем
                }
            }
        }
    }

    return material;
}

bool MaterialImportDialog::importMaterialToDatabase(const ParsedMaterial &material)
{
    if (material.name.isEmpty()) {
        return false;
    }

    // Добавляем материал
    if (!db->addMaterial(material.name)) {
        return false;
    }

    // Добавляем свойства
    for (auto it = material.meta.begin(); it != material.meta.end(); ++it) {
        QString propertyId = it.key();
        PropertyMeta meta = it.value();

        if (meta.name.isEmpty()) {
            continue;
        }

        // Получаем значение
        double value = 0.0;
        if (material.values.contains(propertyId)) {
            value = material.values[propertyId];
        } else {
            // Пробуем найти значение по имени
            for (auto valueIt = material.values.begin(); valueIt != material.values.end(); ++valueIt) {
                QString valueKey = valueIt.key();
                if (valueKey.contains(meta.name) || meta.name.contains(valueKey)) {
                    value = valueIt.value();
                    break;
                }
            }
        }

        // Добавляем свойство с значением
        if (!db->addMaterialProperty(material.name, meta.name, meta.unit, value)) {
            qDebug() << "Failed to add property:" << meta.name << "for material:" << material.name;
        }
    }

    // Добавляем свойство Isotropic
    if (material.isotropic) {
        db->addMaterialProperty(material.name, "Isotropic", "dimensionless", 1.0);
    }

    return true;
}

void MaterialImportDialog::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    logTextEdit->append(QString("[%1] %2").arg(timestamp).arg(message));

    // Автоскроллинг
    QTextCursor cursor = logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    logTextEdit->setTextCursor(cursor);
}
