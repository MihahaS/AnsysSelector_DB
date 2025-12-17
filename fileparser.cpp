#include "fileparser.h"

FileParser::FileParser(QObject *parent) : QObject(parent)
{
}

ParsedData FileParser::parseFile(const QString &filePath, QString &error)
{
    ParsedData data;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = "Ошибка открытия файла: " + filePath;
        return data;
    }

    QTextStream in(&file);
    QString line;
    bool headerProcessed = false;
    int lineNumber = 0;

    // Определяем тип расчета из имени файла
    QString fileName = QFileInfo(filePath).fileName();
    data.calculationType = detectCalculationType(fileName);

    while (!in.atEnd()) {
        lineNumber++;
        line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith("#") || line.startsWith("//")) {
            continue;
        }

        if (!headerProcessed) {
            // Парсим заголовок для получения единиц измерения
            data.unit = extractUnitFromHeader(line);
            headerProcessed = true;
            continue;
        }

        // Парсим строки с данными
        // Обрабатываем запятые в качестве десятичных разделителей
        line.replace(',', '.');

        // Разделяем по табуляции или пробелам
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() >= 2) {
            QString nodeNumber = parts[0];
            QString valueStr = parts[1];

            // Проверяем, если значение заканчивается на запятую (ошибка в исходном файле)
            if (valueStr.endsWith('.')) {
                valueStr = valueStr.left(valueStr.length() - 1);
            }

            // Проверяем специальные случаи
            if (valueStr == "0" || valueStr.isEmpty()) {
                // Ноль или пустая строка - записываем как 0
                data.nodeValues[nodeNumber] = 0.0;
            } else {
                double value = parseNumber(valueStr);
                data.nodeValues[nodeNumber] = value;
            }
        } else if (parts.size() == 1) {
            // Если только номер узла, значение по умолчанию 0
            QString nodeNumber = parts[0];
            data.nodeValues[nodeNumber] = 0.0;
        }
    }

    file.close();

    if (data.nodeValues.isEmpty()) {
        error = "Недопустимые данные";
    }

    return data;
}

QString FileParser::detectCalculationType(const QString &fileName)
{
    QString name = fileName.toLower();

    if (name.contains("normal") && name.contains("stress")) {
        return "Normal Stress";
    } else if (name.contains("directional") && name.contains("deformation")) {
        // Определяем компоненту деформации из имени файла
        if (name.contains("x") || fileName.contains(" X ")) {
            return "Directional Deformation X";
        } else if (name.contains("y") || fileName.contains(" Y ")) {
            return "Directional Deformation Y";
        } else if (name.contains("z") || fileName.contains(" Z ")) {
            return "Directional Deformation Z";
        } else {
            return "Directional Deformation";
        }
    } else if (name.contains("shear") && name.contains("stress")) {
        return "Shear Stress";
    } else if (name.contains("total") && name.contains("deformation")) {
        return "Total Deformation";
    } else if (name.contains("stress")) {
        return "Stress";
    } else if (name.contains("deformation") || name.contains("displacement")) {
        return "Deformation";
    } else if (name.contains("strain")) {
        return "Strain";
    } else if (name.contains("force")) {
        return "Force";
    }

    // Извлекаем название из имени файла
    QString baseName = QFileInfo(fileName).baseName();
    baseName.replace('_', ' ');
    baseName.replace('-', ' ');
    return baseName;
}

QString FileParser::extractUnitFromHeader(const QString &header)
{
    // Ищем единицы измерения в скобках
    QRegularExpression re("\\(([^)]+)\\)");
    QRegularExpressionMatch match = re.match(header);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Если не нашли в скобках, пробуем определить по содержимому
    QString headerLower = header.toLower();

    if (headerLower.contains("pa") || headerLower.contains("pascal")) {
        return "Pa";
    } else if (headerLower.contains("m") || headerLower.contains("meter")) {
        return "m";
    } else if (headerLower.contains("n") || headerLower.contains("newton")) {
        return "N";
    } else if (headerLower.contains("m/m")) {
        return "m/m";
    } else if (headerLower.contains("mm")) {
        return "mm";
    }

    return "";
}

double FileParser::parseNumber(const QString &numberStr)
{
    QString str = numberStr.trimmed();

    // Удаляем возможные пробелы вокруг 'e'
    str.replace(" e", "e");
    str.replace("e ", "e");
    str.replace("E", "e");

    // Проверяем пустую строку или строку только с запятой
    if (str.isEmpty() || str == "." || str == ",") {
        return 0.0;
    }

    // Проверяем специальные случаи
    if (str == "0" || str == "0." || str == "0," || str == "0.0" || str == "0,0") {
        return 0.0;
    }

    // Обработка научной нотации
    if (str.contains('e')) {
        QStringList parts = str.split('e', Qt::SkipEmptyParts);
        if (parts.size() == 2) {
            bool ok1, ok2;
            double mantissa = parts[0].toDouble(&ok1);
            int exponent = parts[1].toInt(&ok2);

            if (ok1 && ok2) {
                return mantissa * qPow(10, exponent);
            }
        }
    }

    // Стандартное преобразование
    bool ok;
    double value = str.toDouble(&ok);

    if (!ok) {
        qDebug() << "Ошибка: Ошибка парсинга значиения:" << str << "изменено на 0";
        return 0.0;
    }

    return value;
}
