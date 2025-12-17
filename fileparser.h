#ifndef FILEPARSER_H
#define FILEPARSER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>
#include <QDebug>

struct ParsedData {
    QString calculationType;
    QString unit;
    QMap<QString, double> nodeValues; // Key: node number, Value: calculation value
};

class FileParser : public QObject
{
    Q_OBJECT

public:
    explicit FileParser(QObject *parent = nullptr);

    ParsedData parseFile(const QString &filePath, QString &error);
    QString detectCalculationType(const QString &fileName);

private:
    QString extractUnitFromHeader(const QString &header);
    double parseNumber(const QString &numberStr);
};

#endif // FILEPARSER_H
