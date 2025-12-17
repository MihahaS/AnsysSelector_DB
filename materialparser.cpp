#include "materialparser.h"

MaterialParser::MaterialParser(QObject *parent) : QObject(parent)
{
}

ParsedMaterial MaterialParser::parseMatML(const QString &path)
{
    ParsedMaterial material;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit logMessage(QString("Cannot open file: %1").arg(path));
        return material;
    }

    QXmlStreamReader xml(&file);
    QString currentPropId;
    QString currentMetaId;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            processElement(xml, material, currentPropId, currentMetaId);
        }

        if (xml.isEndElement()) {
            if (xml.name() == "PropertyDetails") {
                currentMetaId.clear();
            }
            if (xml.name() == "PropertyData") {
                currentPropId.clear();
            }
        }
    }

    if (xml.hasError()) {
        emit logMessage(QString("XML error in %1: %2").arg(path).arg(xml.errorString()));
    }

    file.close();

    if (!material.isEmpty()) {
        emit logMessage(QString("Successfully parsed material: %1 from %2").arg(material.name).arg(path));
    }

    return material;
}

void MaterialParser::processElement(QXmlStreamReader &xml, ParsedMaterial &material,
                                    QString &currentPropId, QString &currentMetaId)
{
    if (xml.name() == "Name" && material.name.isEmpty()) {
        material.name = xml.readElementText().trimmed();
    }

    if (xml.name() == "PropertyDetails") {
        currentMetaId = xml.attributes().value("id").toString();
    }

    if (xml.name() == "PropertyData") {
        currentPropId = xml.attributes().value("property").toString();
    }

    if (xml.name() == "Data" && !currentPropId.isEmpty()) {
        const QString txt = xml.readElementText().trimmed();
        bool ok;
        double value = txt.toDouble(&ok);

        if (ok) {
            material.values[currentPropId] = value;
        }

        if (txt.contains("Isotropic", Qt::CaseInsensitive)) {
            material.isotropic = true;
        }
    }

    if (xml.name() == "Name" && !currentMetaId.isEmpty()) {
        material.meta[currentMetaId].name = xml.readElementText().trimmed();
    }

    if (xml.name() == "Unit") {
        xml.readNextStartElement(); // Name
        if (!xml.isEndElement() && xml.name() == "Name") {
            material.meta[currentMetaId].unit = xml.readElementText().trimmed();
        }
    }
}

QList<ParsedMaterial> MaterialParser::parseDirectory(const QString &directoryPath)
{
    QList<ParsedMaterial> materials;

    QStringList filters = {"*.xml", "*.matml"};
    QDirIterator it(directoryPath, filters, QDir::Files);
    QList<QString> files;

    while (it.hasNext()) {
        files.append(it.next());
    }

    int total = files.size();
    int current = 0;

    for (const QString &filePath : files) {
        current++;
        emit progressChanged(current, total);

        ParsedMaterial material = parseMatML(filePath);
        if (!material.isEmpty()) {
            materials.append(material);
        }
    }

    return materials;
}
