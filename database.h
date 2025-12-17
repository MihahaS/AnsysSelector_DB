#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDebug>
#include <QMap>

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    bool initDatabase(const QString &databaseName = "materials.db");
    bool createTables();
    QSqlDatabase getDatabase() const { return db; }

    // Методы для работы с материалами
    bool addMaterial(const QString &name);
    bool removeMaterial(const QString &name);
    QList<QString> getAllMaterials();

    // Методы для работы со свойствами материалов (новая структура)
    bool addMaterialProperty(const QString &materialName,
                             const QString &propertyName,
                             const QString &unit,
                             double value);
    bool updateMaterialProperty(const QString &materialName,
                                const QString &propertyName,
                                double value);
    bool removeMaterialProperty(const QString &materialName,
                                const QString &propertyName);
    QList<QPair<QString, double>> getMaterialProperties(const QString &materialName);
    QMap<QString, QPair<QString, double>> getMaterialPropertiesWithUnits(const QString &materialName);

    // Получение всех свойств всех материалов
    QMap<QString, QMap<QString, QPair<QString, double>>> getAllMaterialsWithProperties();

    // Методы для моделей
    bool addModel(const QString &name);
    bool removeModel(const QString &name);
    QList<QString> getAllModels();

    // Методы для типов расчетов
    bool addCalculationType(const QString &name, const QString &unit);
    QList<QPair<QString, QString>> getAllCalculationTypes();

    // Методы для результатов расчетов
    bool addCalculationResult(const QString &modelName,
                              const QString &nodeNumber,
                              const QString &calculationTypeName,
                              double value);
    QList<QVector<QVariant>> getCalculationResults(const QString &modelName = "");

    // Импорт материалов из MatML
    bool importMaterialsFromMatML(const QList<QMap<QString, QVariant>> &materials);
    bool clearAllMaterials();

private:
    QSqlDatabase db;
};

#endif // DATABASE_H
