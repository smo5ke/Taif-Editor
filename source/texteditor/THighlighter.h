// #pragma once
// #include "AlifLexer.h"
// #include <QSyntaxHighlighter>
// #include "SyntaxDefinition.h"


// class SyntaxHighlighter : public QSyntaxHighlighter {
//     Q_OBJECT

// public:
//     explicit SyntaxHighlighter(QTextDocument* parent = nullptr);
//     void loadSyntaxDefinition(const QString &filePath);
// protected:
//     void highlightBlock(const QString& text) override;

// private:
//     bool isFunctionName(const QString& blockText, int idEndPos);
//     QVector<Token> tokens{};
//     SyntaxDefinition mDefinition;
// };


// THighlighter.h
#pragma once // استخدام هذا أفضل

#include <QSyntaxHighlighter>
#include "AlifLexer.h"      // تأكد من تضمين ملف Lexer
#include "SyntaxDefinition.h" // تضمين ملف التعريف

class THighlighter : public QSyntaxHighlighter // ✅ تغيير اسم الفئة
{
    Q_OBJECT

public:
    explicit THighlighter(QTextDocument* parent = nullptr);

    // دالة لتحميل التعريف (يمكن استدعاؤها لاحقًا لتغيير الثيم)

    void loadSyntaxDefinition(const QString &filePath);

protected:
    // الدالة الأساسية للتلوين
    void highlightBlock(const QString& text) override;

private:
    // دالة مساعدة لتحديد نوع الدالة
    bool isFunctionName(const QString& blockText, int idEndPos);

    SyntaxDefinition m_definition{}; // ✅ كائن لتخزين التعريف المحمل من JSON
    Lexer m_lexer{};                 // ✅ كائن الـ Lexer لتحليل الكود
};
