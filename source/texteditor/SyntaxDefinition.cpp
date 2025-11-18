#include "SyntaxDefinition.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFont>
#include <QJsonParseError>

bool SyntaxDefinition::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "لا يمكن فتح ملف تعريف الصيغة:" << filePath << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "JSON غير صالح في ملف تعريف الصيغة:" << filePath;
        qWarning() << "خطأ التحليل:" << parseError.errorString() << "عند الموضع:" << parseError.offset;
        return false;
    }

    QJsonObject root = doc.object();

    // 🔹 DEBUG: اطبع مفاتيح الـ JSON للتحقق من الهيكل
    qDebug() << "مفاتيح JSON الجذرية:" << root.keys();

    // 🔹 حاول العثور على كائن الأنماط في أماكن مختلفة محتملة
    QJsonObject stylesObject;

    if (root.contains("styles") && root["styles"].isObject()) {
        stylesObject = root["styles"].toObject();
    }
    else if (root.contains("highlighting") && root["highlighting"].isObject()) {
        QJsonObject highlighting = root["highlighting"].toObject();
        if (highlighting.contains("styles") && highlighting["styles"].isObject()) {
            stylesObject = highlighting["styles"].toObject();
        }
    }
    else if (root.contains("language") && root["language"].isObject()) {
        QJsonObject language = root["language"].toObject();
        // ابحث بشكل متكرر عن الأنماط
        stylesObject = findStylesObject(language);
    }

    if (stylesObject.isEmpty()) {
        qWarning() << "لم يتم العثور على كائن الأنماط في ملف JSON";
        qDebug() << "هيكل JSON الكامل:" << root;
        return false;
    }

    if (!parseStyles(stylesObject)) {
        return false;
    }

    qDebug() << "تم تحميل" << m_styles.size() << "نمط بنجاح";
    return true;
}

QTextCharFormat SyntaxDefinition::createFormatFromKateObject(const QJsonObject &styleObj)
{
    QTextCharFormat format;

    qDebug() << "معالجة كائن النمط، المفاتيح:" << styleObj.keys();

    // 🔹 الألوان في تنسيق Kate
    if (styleObj.contains("color") && styleObj["color"].isString()) {
        QColor color(styleObj["color"].toString());
        if (color.isValid()) {
            format.setForeground(color);
            qDebug() << "  - لون النص:" << color.name();
        }
    }

    if (styleObj.contains("selColor") && styleObj["selColor"].isString()) {
        QColor color(styleObj["selColor"].toString());
        if (color.isValid()) {
            format.setBackground(color);
            qDebug() << "  - لون الخلفية:" << color.name();
        }
    }

    // 🔹 خصائص الخط
    if (styleObj.contains("bold") && styleObj["bold"].isBool()) {
        format.setFontWeight(styleObj["bold"].toBool() ? QFont::Bold : QFont::Normal);
        qDebug() << "  - غامق:" << styleObj["bold"].toBool();
    }

    if (styleObj.contains("italic") && styleObj["italic"].isBool()) {
        format.setFontItalic(styleObj["italic"].toBool());
        qDebug() << "  - مائل:" << styleObj["italic"].toBool();
    }

    if (styleObj.contains("underline") && styleObj["underline"].isBool()) {
        format.setFontUnderline(styleObj["underline"].toBool());
        qDebug() << "  - خط سفلي:" << styleObj["underline"].toBool();
    }

    // 🔹 إذا لم يكن هناك تنسيق، عدّ غير صالح
    if (format.isEmpty()) {
        qDebug() << "  - النمط فارغ";
        return QTextCharFormat();
    }

    return format;
}

// دالة مساعدة للبحث المتكرر عن كائن الأنماط
QJsonObject SyntaxDefinition::findStylesObject(const QJsonObject &obj)
{
    if (obj.contains("styles") && obj["styles"].isObject()) {
        return obj["styles"].toObject();
    }

    // ابحث في جميع الكائنات الفرعية
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (it.value().isObject()) {
            QJsonObject result = findStylesObject(it.value().toObject());
            if (!result.isEmpty()) {
                return result;
            }
        }
    }

    return QJsonObject();
}

bool SyntaxDefinition::parseKateStyles(const QJsonObject &stylesObject)
{
    m_styles.clear();

    for (auto it = stylesObject.constBegin(); it != stylesObject.constEnd(); ++it) {
        QString styleName = it.key();

        if (!it.value().isObject()) {
            qDebug() << "النمط" << styleName << "ليس كائن، تخطي";
            continue;
        }

        QJsonObject styleObj = it.value().toObject();
        QTextCharFormat format = createFormatFromKateObject(styleObj);

        if (format.isValid()) {
            m_styles.insert(styleName, format);
            qDebug() << "تم إنشاء النمط:" << styleName;
        } else {
            qDebug() << "فشل إنشاء النمط:" << styleName;
        }
    }

    if (m_styles.isEmpty()) {
        qWarning() << "لم يتم إنشاء أي أنماط من الكائن";
        return false;
    }

    qDebug() << "تم تحميل" << m_styles.size() << "نمط من تنسيق Kate";
    return true;
}

bool SyntaxDefinition::loadFromJson(const QJsonObject &root)
{
    qDebug() << "=== تحليل ملف الصيغة ===";

    // 🔹 ابحث في الهيكل المحدد language -> highlighting -> itemDatas
    if (root.contains("language") && root["language"].isObject()) {
        QJsonObject language = root["language"].toObject();

        if (language.contains("highlighting") && language["highlighting"].isObject()) {
            QJsonObject highlighting = language["highlighting"].toObject();
            qDebug() << "مفاتيح highlighting:" << highlighting.keys();

            if (highlighting.contains("itemDatas") && highlighting["itemDatas"].isObject()) {
                QJsonObject itemDatas = highlighting["itemDatas"].toObject();
                qDebug() << "تم العثور على itemDatas!";

                return parseItemDatas(itemDatas);
            } else {
                qWarning() << "لم يتم العثور على itemDatas في highlighting";
            }
        }
    }

    qWarning() << "فشل في العثور على الهيكل المتوقع";
    return false;
}

bool SyntaxDefinition::parseStyles(const QJsonObject &stylesObject)
{
    m_styles.clear();
    for (auto it = stylesObject.constBegin(); it != stylesObject.constEnd(); ++it) {
        if (!it.value().isObject()) {
            qWarning() << "Invalid style format for:" << it.key();
            continue;
        }
        m_styles.insert(it.key(), createFormat(it.value().toObject()));
    }
    return !m_styles.isEmpty();
}

QTextCharFormat SyntaxDefinition::createFormat(const QJsonObject &styleObject)
{
    QTextCharFormat format;
    if (styleObject.contains("foreground") && styleObject["foreground"].isString()) {
        format.setForeground(QColor(styleObject["foreground"].toString()));
    }
    if (styleObject.contains("background") && styleObject["background"].isString()) {
        format.setBackground(QColor(styleObject["background"].toString()));
    }
    if (styleObject.contains("bold") && styleObject["bold"].isBool() && styleObject["bold"].toBool()) {
        format.setFontWeight(QFont::Bold);
    }
    if (styleObject.contains("italic") && styleObject["italic"].isBool() && styleObject["italic"].toBool()) {
        format.setFontItalic(true);
    }
    if (styleObject.contains("underline") && styleObject["underline"].isBool() && styleObject["underline"].toBool()) {
        format.setFontUnderline(true);
    }
    // أضف المزيد من خصائص التنسيق حسب الحاجة
    return format;
}

bool SyntaxDefinition::parseItemDatas(const QJsonObject &itemDatas)
{
    m_styles.clear();

    if (itemDatas.contains("itemData") && itemDatas["itemData"].isArray()) {
        QJsonArray itemDataArray = itemDatas["itemData"].toArray();
        qDebug() << "عدد عناصر itemData:" << itemDataArray.size();

        for (const QJsonValue &itemValue : itemDataArray) {
            if (!itemValue.isObject()) continue;

            QJsonObject itemObj = itemValue.toObject();

            if (itemObj.contains("@attributes") && itemObj["@attributes"].isObject()) {
                QJsonObject attributes = itemObj["@attributes"].toObject();

                if (attributes.contains("name") && attributes.contains("defStyleNum")) {
                    QString styleName = attributes["name"].toString();
                    QString defStyleNum = attributes["defStyleNum"].toString();

                    QTextCharFormat format = createFormatFromStyleNum(defStyleNum);
                    if (format.isValid()) {
                        m_styles.insert(styleName, format);
                        qDebug() << "تم إنشاء النمط:" << styleName << "->" << defStyleNum;
                    }
                }
            }
        }
    } else {
        qWarning() << "itemData ليس مصفوفة أو غير موجود";
        return false;
    }

    qDebug() << "تم تحميل" << m_styles.size() << "نمط بنجاح";
    return !m_styles.isEmpty();
}

QTextCharFormat SyntaxDefinition::createFormatFromStyleNum(const QString &defStyleNum)
{
    QTextCharFormat format;

    // تعيين الألوان بناءً على defStyleNum
    if (defStyleNum == "dsNormal") {
        // النص العادي - لون افتراضي
        format.setForeground(QColor("#cccccc"));
    }
    else if (defStyleNum == "dsKeyword" || defStyleNum == "dsControlFlow") {
        // الكلمات المفتاحية - أزرق
        format.setForeground(QColor("#569cd6"));
        format.setFontWeight(QFont::Bold);
    }
    else if (defStyleNum == "dsOperator") {
        // العوامل - رمادي فاتح
        format.setForeground(QColor("#d4d4d4"));
    }
    else if (defStyleNum == "dsBuiltIn") {
        // الدوال المدمجة - أصفر
        format.setForeground(QColor("#dcdcaa"));
    }
    else if (defStyleNum == "dsVariable" || defStyleNum == "dsFunction") {
        // المتغيرات والدوال - أزرق فاتح
        format.setForeground(QColor("#9cdcfe"));
    }
    else if (defStyleNum == "dsString" || defStyleNum == "dsVerbatimString" || defStyleNum == "dsSpecialString") {
        // النصوص - برتقالي
        format.setForeground(QColor("#ce9178"));
    }
    else if (defStyleNum == "dsComment") {
        // التعليقات - أخضر
        format.setForeground(QColor("#6a9955"));
        format.setFontItalic(true);
    }
    else if (defStyleNum == "dsFloat" || defStyleNum == "dsDecVal" || defStyleNum == "dsBaseN") {
        // الأرقام - أخضر فاتح
        format.setForeground(QColor("#b5cea8"));
    }
    else if (defStyleNum == "dsImport" || defStyleNum == "dsPreprocessor") {
        // الاستيراد والمعالجات - وردي
        format.setForeground(QColor("#c586c0"));
    }
    else if (defStyleNum == "dsError") {
        // الأخطاء - أحمر
        format.setForeground(QColor("#f44747"));
        format.setFontUnderline(true);
    }
    else if (defStyleNum == "dsAttribute") {
        // السمات - أزرق
        format.setForeground(QColor("#9cdcfe"));
        format.setFontItalic(true);
    }
    else if (defStyleNum == "dsChar") {
        // المحارف - برتقالي
        format.setForeground(QColor("#ce9178"));
    }
    else if (defStyleNum == "dsSpecialChar") {
        // المحارف الخاصة - وردي
        format.setForeground(QColor("#d7ba7d"));
    }
    else {
        // نمط افتراضي
        format.setForeground(QColor("#cccccc"));
        qDebug() << "نمط غير معروف:" << defStyleNum << "-> استخدام الافتراضي";
    }

    return format;
}

bool SyntaxDefinition::parseRules(const QJsonArray &rulesArray)
{
    m_rules.clear();
    for (const QJsonValue &value : rulesArray) {
        if (!value.isObject()) continue;
        QJsonObject ruleObject = value.toObject();

        if (!ruleObject.contains("match") || !ruleObject["match"].isString()) continue;

        HighlightingRule rule;
        rule.pattern = QRegularExpression(ruleObject["match"].toString());
        if (!rule.pattern.isValid()) {
            qWarning() << "Invalid regex pattern:" << ruleObject["match"].toString() << rule.pattern.errorString();
            continue;
        }

        if (ruleObject.contains("style") && ruleObject["style"].isString()) {
            QString styleName = ruleObject["style"].toString();
            if (m_styles.contains(styleName)) {
                // تطبيق النمط الافتراضي على كامل المطابقة (المجموعة 0)
                rule.formats.insert(0, m_styles[styleName]);
            } else {
                qWarning() << "Undefined style name:" << styleName;
            }
        }

        if (ruleObject.contains("captures") && ruleObject["captures"].isObject()) {
            QJsonObject captures = ruleObject["captures"].toObject();
            for (auto it = captures.constBegin(); it != captures.constEnd(); ++it) {
                bool ok;
                int captureIndex = it.key().toInt(&ok);
                if (!ok || captureIndex <= 0) continue; // يجب أن تكون أرقامًا أكبر من 0

                if (it.value().isObject()) {
                    QJsonObject captureStyleObj = it.value().toObject();
                    if (captureStyleObj.contains("style") && captureStyleObj["style"].isString()) {
                        QString styleName = captureStyleObj["style"].toString();
                        if (m_styles.contains(styleName)) {
                            rule.formats.insert(captureIndex, m_styles[styleName]);
                        } else {
                            qWarning() << "Undefined style name in capture:" << styleName;
                        }
                    }
                }
            }
        }

        if (!rule.formats.isEmpty()) { // أضف القاعدة فقط إذا كان لها تنسيق
            m_rules.append(rule);
        }
    }
    return !m_rules.isEmpty();
}

const QVector<HighlightingRule>& SyntaxDefinition::rules() const
{
    return m_rules;
}

QTextCharFormat SyntaxDefinition::getStyleFormat(const QString &styleName) const
{
    // Look up the style name, return the format if found,
    // otherwise return a default (empty) format.
    return m_styles.value(styleName, QTextCharFormat());
}
