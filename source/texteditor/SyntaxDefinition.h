#ifndef SYNTAXDEFINITION_H
#define SYNTAXDEFINITION_H

#include <QString>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include <QMap>
#include <QColor>
#include <QJsonObject>  // ✅ أضف هذا السطر

// هيكل لتخزين القاعدة
struct HighlightingRule {
    QRegularExpression pattern;
    QMap<int, QTextCharFormat> formats; // لتخزين تنسيق كل مجموعة التقاط (capture group)
};

// فئة قارئ التعريف
class SyntaxDefinition
{
public:
    QTextCharFormat createFormatFromStyleNum(const QString &defStyleNum);
    bool parseItemDatas(const QJsonObject &itemDatas);
    bool parseKateStyles(const QJsonObject &itemsObject);
    QJsonObject findStylesObject(const QJsonObject &obj);
    bool load(const QString &filePath); // تحميل التعريف من ملف JSON
    QTextCharFormat getStyleFormat(const QString &styleName) const;
    const QVector<HighlightingRule>& rules() const; // للحصول على القواعد المحملة
    bool loadFromJson(const QJsonObject &root);
    QTextCharFormat createFormatFromKateObject(const QJsonObject &styleObj);


private:
    QVector<HighlightingRule> m_rules;
    QMap<QString, QTextCharFormat> m_styles; // لتخزين الأنماط المحملة

    bool parseStyles(const QJsonObject &stylesObject);
    bool parseRules(const QJsonArray &rulesArray);
    QTextCharFormat createFormat(const QJsonObject &styleObject);
};

#endif // SYNTAXDEFINITION_H
