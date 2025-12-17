#include "database.h"

Database::Database(QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
}

Database::~Database()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool Database::initDatabase(const QString &databaseName)
{
    db.setDatabaseName(databaseName);

    if (!db.open()) {
        qDebug() << "Error opening database:" << db.lastError().text();
        return false;
    }

    return createTables();
}

bool Database::createTables()
{
    QSqlQuery query;

    // 1. Материалы
    bool success = query.exec("CREATE TABLE IF NOT EXISTS materials ("
                              "name TEXT PRIMARY KEY NOT NULL)");

    if (!success) {
        qDebug() << "Error creating materials table:" << query.lastError().text();
        return false;
    }

    // 2. Свойства материалов (обновленная структура)
    success = query.exec("CREATE TABLE IF NOT EXISTS material_properties ("
                         "property_name TEXT NOT NULL,"
                         "material_name TEXT NOT NULL,"
                         "unit TEXT NOT NULL,"
                         "value REAL NOT NULL,"
                         "FOREIGN KEY (material_name) REFERENCES materials(name) ON DELETE CASCADE,"
                         "PRIMARY KEY (property_name, material_name))");

    if (!success) {
        qDebug() << "Error creating material_properties table:" << query.lastError().text();
        return false;
    }

    // 3. Модели
    success = query.exec("CREATE TABLE IF NOT EXISTS models ("
                         "name TEXT PRIMARY KEY NOT NULL)");

    if (!success) {
        qDebug() << "Error creating models table:" << query.lastError().text();
        return false;
    }

    // 4. Виды расчетов
    success = query.exec("CREATE TABLE IF NOT EXISTS calculation_types ("
                         "name TEXT PRIMARY KEY NOT NULL,"
                         "unit TEXT NOT NULL)");

    if (!success) {
        qDebug() << "Error creating calculation_types table:" << query.lastError().text();
        return false;
    }

    // 5. Результаты расчетов
    success = query.exec("CREATE TABLE IF NOT EXISTS calculation_results ("
                         "model_name TEXT NOT NULL,"
                         "node_number TEXT NOT NULL,"
                         "calculation_type_name TEXT NOT NULL,"
                         "value REAL NOT NULL,"
                         "FOREIGN KEY (model_name) REFERENCES models(name) ON DELETE CASCADE,"
                         "FOREIGN KEY (calculation_type_name) REFERENCES calculation_types(name) ON DELETE CASCADE,"
                         "PRIMARY KEY (model_name, node_number, calculation_type_name))");

    if (!success) {
        qDebug() << "Error creating calculation_results table:" << query.lastError().text();
        return false;
    }

    // Создание индексов
    query.exec("CREATE INDEX IF NOT EXISTS idx_results_model_node ON calculation_results(model_name, node_number)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_results_calc_type ON calculation_results(calculation_type_name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_results_material ON calculation_results(material_name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_prop_values_mat ON material_properties(material_name)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_prop_values_prop ON material_properties(property_name)");

    // Добавление предопределенных типов расчетов
    query.exec("INSERT OR IGNORE INTO calculation_types (name, unit) VALUES ('Normal Stress', 'Pa')");
    query.exec("INSERT OR IGNORE INTO calculation_types (name, unit) VALUES ('Directional Deformation', 'm')");
    query.exec("INSERT OR IGNORE INTO calculation_types (name, unit) VALUES ('Shear Stress', 'Pa')");
    query.exec("INSERT OR IGNORE INTO calculation_types (name, unit) VALUES ('Total Deformation', 'm')");

    return true;
}

bool Database::addMaterial(const QString &name)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO materials (name) VALUES (:name)");
    query.bindValue(":name", name);
    return query.exec();
}

bool Database::removeMaterial(const QString &name)
{
    QSqlQuery query;
    query.prepare("DELETE FROM materials WHERE name = :name");
    query.bindValue(":name", name);
    return query.exec();
}

QList<QString> Database::getAllMaterials()
{
    QList<QString> materials;
    QSqlQuery query("SELECT name FROM materials ORDER BY name");
    while (query.next()) {
        materials.append(query.value(0).toString());
    }
    return materials;
}

bool Database::addMaterialProperty(const QString &materialName,
                                   const QString &propertyName,
                                   const QString &unit,
                                   double value)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO material_properties "
                  "(property_name, material_name, unit, value) "
                  "VALUES (:property_name, :material_name, :unit, :value)");
    query.bindValue(":property_name", propertyName);
    query.bindValue(":material_name", materialName);
    query.bindValue(":unit", unit.isEmpty() ? "dimensionless" : unit);
    query.bindValue(":value", value);

    return query.exec();
}

bool Database::updateMaterialProperty(const QString &materialName,
                                      const QString &propertyName,
                                      double value)
{
    QSqlQuery query;
    query.prepare("UPDATE material_properties SET value = :value "
                  "WHERE material_name = :material_name AND property_name = :property_name");
    query.bindValue(":value", value);
    query.bindValue(":material_name", materialName);
    query.bindValue(":property_name", propertyName);

    return query.exec();
}

bool Database::removeMaterialProperty(const QString &materialName,
                                      const QString &propertyName)
{
    QSqlQuery query;
    query.prepare("DELETE FROM material_properties "
                  "WHERE material_name = :material_name AND property_name = :property_name");
    query.bindValue(":material_name", materialName);
    query.bindValue(":property_name", propertyName);

    return query.exec();
}

QList<QPair<QString, double>> Database::getMaterialProperties(const QString &materialName)
{
    QList<QPair<QString, double>> properties;
    QSqlQuery query;

    query.prepare("SELECT property_name, value FROM material_properties "
                  "WHERE material_name = :material_name ORDER BY property_name");
    query.bindValue(":material_name", materialName);

    if (query.exec()) {
        while (query.next()) {
            properties.append(qMakePair(query.value(0).toString(),
                                        query.value(1).toDouble()));
        }
    } else {
        qDebug() << "Error getting material properties:" << query.lastError().text();
    }

    return properties;
}

QMap<QString, QPair<QString, double>> Database::getMaterialPropertiesWithUnits(const QString &materialName)
{
    QMap<QString, QPair<QString, double>> properties;
    QSqlQuery query;

    query.prepare("SELECT property_name, unit, value FROM material_properties "
                  "WHERE material_name = :material_name ORDER BY property_name");
    query.bindValue(":material_name", materialName);

    if (query.exec()) {
        while (query.next()) {
            QString propertyName = query.value(0).toString();
            QString unit = query.value(1).toString();
            double value = query.value(2).toDouble();
            properties[propertyName] = qMakePair(unit, value);
        }
    }

    return properties;
}

QMap<QString, QMap<QString, QPair<QString, double>>> Database::getAllMaterialsWithProperties()
{
    QMap<QString, QMap<QString, QPair<QString, double>>> allMaterials;

    QSqlQuery query("SELECT m.name, mp.property_name, mp.unit, mp.value "
                    "FROM materials m "
                    "LEFT JOIN material_properties mp ON m.name = mp.material_name "
                    "ORDER BY m.name, mp.property_name");

    while (query.next()) {
        QString materialName = query.value(0).toString();
        QString propertyName = query.value(1).toString();
        QString unit = query.value(2).toString();
        double value = query.value(3).toDouble();

        if (!propertyName.isEmpty()) {
            allMaterials[materialName][propertyName] = qMakePair(unit, value);
        }
    }

    return allMaterials;
}

bool Database::addModel(const QString &name)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO models (name) VALUES (:name)");
    query.bindValue(":name", name);
    return query.exec();
}

bool Database::removeModel(const QString &name)
{
    QSqlQuery query;
    query.prepare("DELETE FROM models WHERE name = :name");
    query.bindValue(":name", name);
    return query.exec();
}

QList<QString> Database::getAllModels()
{
    QList<QString> models;
    QSqlQuery query("SELECT name FROM models ORDER BY name");
    while (query.next()) {
        models.append(query.value(0).toString());
    }
    return models;
}

bool Database::addCalculationType(const QString &name, const QString &unit)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO calculation_types (name, unit) VALUES (:name, :unit)");
    query.bindValue(":name", name);
    query.bindValue(":unit", unit);
    return query.exec();
}

QList<QPair<QString, QString>> Database::getAllCalculationTypes()
{
    QList<QPair<QString, QString>> types;
    QSqlQuery query("SELECT name, unit FROM calculation_types ORDER BY name");
    while (query.next()) {
        types.append(qMakePair(query.value(0).toString(), query.value(1).toString()));
    }
    return types;
}

bool Database::addCalculationResult(const QString &modelName,
                                    const QString &nodeNumber,
                                    const QString &calculationTypeName,
                                    double value)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO calculation_results "
                  "(model_name, node_number, calculation_type_name, value) "
                  "VALUES (:model_name, :node_number, :calculation_type_name, :value)");
    query.bindValue(":model_name", modelName);
    query.bindValue(":node_number", nodeNumber);
    query.bindValue(":calculation_type_name", calculationTypeName);
    query.bindValue(":value", value);
    return query.exec();
}

QList<QVector<QVariant>> Database::getCalculationResults(const QString &modelName)
{
    QList<QVector<QVariant>> results;
    QSqlQuery query;

    if (modelName.isEmpty()) {
        query.exec("SELECT model_name, node_number, calculation_type_name, value "
                   "FROM calculation_results "
                   "ORDER BY model_name, node_number, calculation_type_name");
    } else {
        query.prepare("SELECT model_name, node_number, calculation_type_name, value "
                      "FROM calculation_results "
                      "ORDER BY node_number, calculation_type_name");
        query.bindValue(":model_name", modelName);
        query.exec();
    }

    while (query.next()) {
        QVector<QVariant> row;
        for (int i = 0; i < 5; ++i) {
            row.append(query.value(i));
        }
        results.append(row);
    }

    return results;
}

bool Database::importMaterialsFromMatML(const QList<QMap<QString, QVariant>> &materials)
{
    QSqlDatabase::database().transaction();

    try {
        QSqlQuery query(db);

        for (const QMap<QString, QVariant> &materialData : materials) {
            QString materialName = materialData["name"].toString();

            if (materialName.isEmpty()) {
                continue;
            }

            // Добавляем материал
            query.prepare("INSERT OR IGNORE INTO materials (name) VALUES (?)");
            query.bindValue(0, materialName);
            query.exec();

            // Добавляем свойства
            QMap<QString, QVariant> properties = materialData["properties"].toMap();

            for (auto it = properties.begin(); it != properties.end(); ++it) {
                QString propertyName = it.key();
                QVariant propertyData = it.value();

                if (propertyData.typeId() == QMetaType::QVariantMap) {
                    QMap<QString, QVariant> propMap = propertyData.toMap();
                    QString unit = propMap["unit"].toString();
                    double value = propMap["value"].toDouble();

                    query.prepare("INSERT OR REPLACE INTO material_properties "
                                  "(property_name, material_name, unit, value) "
                                  "VALUES (?, ?, ?, ?)");
                    query.bindValue(0, propertyName);
                    query.bindValue(1, materialName);
                    query.bindValue(2, unit.isEmpty() ? "dimensionless" : unit);
                    query.bindValue(3, value);
                    query.exec();
                } else {
                    // Если свойство - просто значение
                    double value = propertyData.toDouble();

                    query.prepare("INSERT OR REPLACE INTO material_properties "
                                  "(property_name, material_name, unit, value) "
                                  "VALUES (?, ?, ?, ?)");
                    query.bindValue(0, propertyName);
                    query.bindValue(1, materialName);
                    query.bindValue(2, "dimensionless");
                    query.bindValue(3, value);
                    query.exec();
                }
            }
        }

        QSqlDatabase::database().commit();
        return true;

    } catch (...) {
        QSqlDatabase::database().rollback();
        qDebug() << "Error importing materials, transaction rolled back";
        return false;
    }
}

bool Database::clearAllMaterials()
{
    QSqlQuery query(db);

    // Включаем каскадное удаление
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qDebug() << "Failed to enable foreign keys:" << query.lastError().text();
        return false;
    }

    // Удаляем все материалы (каскадно удалятся их свойства)
    if (!query.exec("DELETE FROM materials")) {
        qDebug() << "Failed to clear materials:" << query.lastError().text();
        return false;
    }

    return true;
}
