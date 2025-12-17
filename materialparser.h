#ifndef MATERIALPARSER_H
#define MATERIALPARSER_H

#include <QObject>
#include <QFile>
#include <QXmlStreamReader>
#include <QDirIterator>
#include <QMap>
#include <QDebug>

struct PropertyMeta {
    QString name;
    QString unit;
};

struct ParsedMaterial {
    QString name;
    QMap<QString, PropertyMeta> meta;   // id → meta
    QMap<QString, double> values;       // id → value
    bool isotropic = false;

    bool isEmpty() const {
        return name.isEmpty() && values.isEmpty();
    }
};

class MaterialParser : public QObject
{
    Q_OBJECT

public:
    explicit MaterialParser(QObject *parent = nullptr);

    ParsedMaterial parseMatML(const QString &path);
    QList<ParsedMaterial> parseDirectory(const QString &directoryPath);

signals:
    void progressChanged(int current, int total);
    void logMessage(const QString &message);

private:
    void processElement(QXmlStreamReader &xml, ParsedMaterial &material,
                        QString &currentPropId, QString &currentMetaId);
};

#endif // MATERIALPARSER_H
