#include "TEditor.h"
#include "THighlighter.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMimeData>
#include <QSettings>
#include <QPainterPath>
#include <QStack>
#include <QMenu>
#include <QAction>
#include <QFile>


TMinimap::TMinimap(TEditor *editor) : QWidget(editor), editor(editor)
{
    setStyleSheet("background-color: #1e1e1e; border-left: 1px solid #333;");
    setFixedWidth(100);
}

QSize TMinimap::sizeHint() const {
    return QSize(100, 0);
}

// void TMinimap::paintEvent(QPaintEvent *event)
// {
//     Q_UNUSED(event);
//     QPainter painter(this);
//     // painter.fillRect(rect(), QColor("#1e1e1e")); // خلفية الخريطة

//     if (editor) {
//         // 1. إجبار الرسام على العمل بنظام إحداثيات LTR لضمان الرسم الصحيح
//         painter.setLayoutDirection(Qt::LayoutDirectionAuto);

//         qreal scale = 0.15;
//         painter.save();
//         painter.setClipRect(rect());

//         painter.scale(scale, scale);
//         painter.translate(0, 0);

//         // رسم محتوى المستند
//         // ملاحظة: drawContents ترسم كل شيء، بما في ذلك الخلفية إذا لم نكن حذرين
//         // لكن QPlainTextEdit عادة ما يكون شفافاً عند الرسم بهذه الطريقة
//         editor->document()->drawContents(&painter);

//         painter.restore();

//         // 2. رسم المستطيل المضيء
//         // ... (بقية كود المستطيل المضيء كما هو، فهو يعتمد على الأرقام فقط) ...
//         int scrollMax = editor->verticalScrollBar()->maximum();
//         int scrollVal = editor->verticalScrollBar()->value();
//         int pageStep  = editor->verticalScrollBar()->pageStep();

//         long long totalScrollableHeight = scrollMax + pageStep;
//         if (totalScrollableHeight == 0) totalScrollableHeight = 1;

//         double viewRatio = (double)pageStep / totalScrollableHeight;
//         double posRatio = (double)scrollVal / totalScrollableHeight;

//         int mapHeight = height();
//         int highlightH = mapHeight * viewRatio;
//         int highlightY = mapHeight * posRatio;

//         if (highlightH < 20) highlightH = 20;

//         painter.setPen(Qt::NoPen);
//         painter.setBrush(QColor(255, 255, 255, 10));
//         painter.drawRect(0, highlightY, width(), highlightH);
//     }
// }
void TMinimap::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // 1. رسم الخلفية (مهم جداً للتباين)
    painter.fillRect(rect(), QColor("#1e1e1e"));

    if (!editor) return;

    // إعدادات الخط والرسم
    // نستخدم خطاً صغيراً جداً لرسم النصوص
    QFont font = editor->font();
    // لا نعتمد على Scale للرسام، بل نصغر الخط نفسه ليكون أوضح في الرسم اليدوي
    // أو نستخدم Scale ونرسم بخط عادي. لنجرب الـ Scale لأنه أسرع:

    qreal scale = 0.15; // نسبة التصغير
    painter.save();
    painter.scale(scale, scale);

    // ✅ إجبار الرسم من اليسار لليمين داخل الخريطة لضمان ظهور النص في المكان المتوقع
    painter.setLayoutDirection(Qt::LeftToRight);
    painter.setPen(QColor("#a0a0a0")); // لون رمادي فاتح للنص

    // 2. ✅✅✅ الرسم اليدوي (Manual Loop) ✅✅✅
    // نمر على كل الكتل (الأسطر) ونرسمها يدوياً
    QTextBlock block = editor->document()->firstBlock();
    qreal currentY = 0;

    while (block.isValid()) {
        QString text = block.text();

        // إذا كان السطر غير فارغ، ارسمه
        if (!text.isEmpty()) {
            // نرسم النص عند الإحداثي (0, currentY)
            // نستخدم drawText لرسم النص الخام
            // (يمكنك لاحقاً تحسينها لرسم مربعات ملونة مثل VSCode بدلاً من النص)
            painter.drawText(QPointF(0, currentY + block.layout()->boundingRect().height()), text);
        }

        // نزيد الإحداثي Y بمقدار ارتفاع السطر الحالي
        currentY += block.layout()->boundingRect().height();

        block = block.next();
    }

    painter.restore(); // نعود للحجم الطبيعي لرسم المستطيل المضيء

    // 3. رسم المستطيل المضيء (نفس الكود السابق)
    int scrollMax = editor->verticalScrollBar()->maximum();
    int scrollVal = editor->verticalScrollBar()->value();
    int pageStep  = editor->verticalScrollBar()->pageStep();

    long long totalScrollableHeight = scrollMax + pageStep;
    if (totalScrollableHeight <= 0) totalScrollableHeight = 1;

    double viewRatio = (double)pageStep / totalScrollableHeight;
    double posRatio = (double)scrollVal / totalScrollableHeight;

    if (scrollMax == 0) { viewRatio = 1.0; posRatio = 0.0; }

    int mapHeight = height();
    int highlightY = mapHeight * posRatio;
    int highlightH = mapHeight * viewRatio;

    if (highlightH < 15) highlightH = 15;
    if (highlightY + highlightH > mapHeight) highlightY = mapHeight - highlightH;

    // رسم المستطيل
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 30));
    painter.drawRect(0, highlightY, width(), highlightH);
}

void TMinimap::mousePressEvent(QMouseEvent *event) {
    scrollEditorTo(event->pos());
}

void TMinimap::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        scrollEditorTo(event->pos());
    }
}

void TMinimap::scrollEditorTo(const QPoint &pos) {
    // حساب نسبة النقر في الخريطة وتطبيقها على سكرول المحرر
    double ratio = (double)pos.y() / height();
    int maxVal = editor->verticalScrollBar()->maximum();
    editor->verticalScrollBar()->setValue(maxVal * ratio);
}



TEditor::TEditor(QWidget* parent) {
    setAcceptDrops(true);
    this->setStyleSheet("QPlainTextEdit { background-color: #141520; color: #cccccc; }");
    this->setTabStopDistance(32);

    // set "force" cursor and text direction from right to left
    QTextDocument* editorDocument = this->document();
    QTextOption option = editorDocument->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    editorDocument->setDefaultTextOption(option);


    highlighter = new THighlighter(editorDocument);
    autoComplete = new AutoComplete(this, parent);
    lineNumberArea = new LineNumberArea(this);

    connect(this, &TEditor::blockCountChanged, this, &TEditor::updateLineNumberAreaWidth);
    connect(this, &TEditor::updateRequest, this, &TEditor::updateLineNumberArea);
    connect(this, &TEditor::cursorPositionChanged, this, &TEditor::highlightCurrentLine);
    connect(this->document(), &QTextDocument::contentsChanged, this, &TEditor::updateFoldRegions);

    minimap = new TMinimap(this);

    connect(this->document(), &QTextDocument::contentsChanged, this, &TEditor::updateMinimap);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, &TEditor::updateMinimap);

    updateLineNumberAreaWidth();
    highlightCurrentLine();

    QSettings settingsVal("Alif", "Taif");
    int savedSize = settingsVal.value("editorFontSize").toInt();
    updateFontSize(savedSize);

    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setInterval(30000);
    connect(autoSaveTimer, &QTimer::timeout, this, &TEditor::performAutoSave);

    connect(this->document(), &QTextDocument::contentsChanged, this, &TEditor::startAutoSave);

    highlighter = new THighlighter(this->document());
    installEventFilter(this);
}

void TEditor::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta == 0) return;

        QFont font = this->font();
        qreal currentSize = font.pointSizeF();

        qreal step = 0.5;

        if (delta > 0) {
            currentSize += step;
        } else {
            currentSize -= step;
        }

        if (currentSize < 5.0) currentSize = 5.0;
        if (currentSize > 50) currentSize = 50;

        font.setPointSizeF(currentSize);
        this->setFont(font);

        if (lineNumberArea) {
            QFont lineFont = lineNumberArea->font();
            lineFont.setPointSizeF(currentSize);
            lineNumberArea->setFont(lineFont);
        }

        updateLineNumberAreaWidth();

        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void TEditor::updateMinimap() {
    if (minimap) minimap->update();
}

void TEditor::updateFontSize(int size) {
    if (size < 10) {
        size = 16;
    }

    QFont font = this->font();
    font.setPointSize(size);
    this->setFont(font);

    QFont fontNums = lineNumberArea->font();
    fontNums.setPointSize(size);
    lineNumberArea->setFont(fontNums);
}


// 1. دالة تعليق/إلغاء تعليق الأكواد
void TEditor::toggleComment()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock(); // لبدء عملية تراجع (Undo) واحدة

    int startPos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();

    // تحديد بداية ونهاية الأسطر المحددة
    cursor.setPosition(startPos);
    int startBlock = cursor.blockNumber();
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    int endBlock = cursor.blockNumber();

    // إذا كان التحديد ينتهي في بداية سطر جديد، لا نحسب السطر الأخير
    if (cursor.atBlockStart() && endBlock > startBlock) {
        endBlock--;
    }

    // التحقق: هل سنقوم بالتعليق أم إزالة التعليق؟
    bool shouldComment = false;

    // نفحص السطر الأول لنقرر (إذا لم يكن معلقاً، سنعلق الجميع)
    QTextBlock block = document()->findBlockByNumber(startBlock);
    if (!block.text().trimmed().startsWith("#")) {
        shouldComment = true;
    }

    // تنفيذ العملية على كل الأسطر المحددة
    for (int i = startBlock; i <= endBlock; ++i) {
        block = document()->findBlockByNumber(i);
        QTextCursor lineCursor(block);

        if (shouldComment) {
            // إضافة تعليق
            lineCursor.movePosition(QTextCursor::StartOfBlock);
            lineCursor.insertText("# ");
        } else {
            // إزالة تعليق
            QString text = block.text();
            if (text.startsWith("# ")) {
                lineCursor.movePosition(QTextCursor::StartOfBlock);
                lineCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                lineCursor.removeSelectedText();
            } else if (text.startsWith("#")) {
                lineCursor.movePosition(QTextCursor::StartOfBlock);
                lineCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                lineCursor.removeSelectedText();
            }
        }
    }

    cursor.endEditBlock();
}

// 2. دالة تكرار السطر (Duplicate)
void TEditor::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    // احفظ النص الحالي للسطر
    QString lineText = cursor.block().text();

    // انتقل لنهاية السطر
    cursor.movePosition(QTextCursor::EndOfBlock);

    // أضف سطراً جديداً والنص المنسوخ
    cursor.insertText("\n" + lineText);

    cursor.endEditBlock();
}

// 3. دالة تحريك السطر للأعلى
void TEditor::moveLineUp()
{
    QTextCursor cursor = textCursor();
    QTextBlock currentBlock = cursor.block();
    QTextBlock prevBlock = currentBlock.previous();

    if (!prevBlock.isValid()) return; // نحن في السطر الأول

    cursor.beginEditBlock();

    // حفظ نص السطر الحالي وحذفه
    QString currentText = currentBlock.text();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.deletePreviousChar(); // حذف السطر الفارغ

    // الانتقال للسطر السابق وإدراج النص قبله
    cursor.movePosition(QTextCursor::StartOfBlock); // بداية السطر السابق
    cursor.insertText(currentText + "\n");

    // إعادة المؤشر للسطر الذي تم تحريكه
    cursor.movePosition(QTextCursor::Up);
    setTextCursor(cursor);

    cursor.endEditBlock();
}

// 4. دالة تحريك السطر للأسفل
void TEditor::moveLineDown()
{
    QTextCursor cursor = textCursor();
    QTextBlock currentBlock = cursor.block();
    QTextBlock nextBlock = currentBlock.next();

    if (!nextBlock.isValid()) return; // نحن في السطر الأخير

    cursor.beginEditBlock();

    // حفظ نص السطر الحالي وحذفه
    QString currentText = currentBlock.text();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    if (cursor.atBlockStart()) cursor.deleteChar(); // حذف السطر الفارغ (السطر التالي سيصعد)

    // الانتقال للسطر التالي (الذي أصبح مكان الحالي) ثم نهايته
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + currentText);

    setTextCursor(cursor);
    cursor.endEditBlock();
}

bool TEditor::eventFilter(QObject* obj, QEvent* event) {
    if (obj == this and event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (autoComplete->isPopupVisible()) {
            if (keyEvent->key() == Qt::Key_Return
                or keyEvent->key() == Qt::Key_Enter) {
                return false;
            }
        }
        // Handle Shift+Return or Shift+Enter
        if (keyEvent->key() == Qt::Key_Return
             or keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return true; // Event handled
            }
            curserIndentation();
            return true;
        }
    }
    return QPlainTextEdit::eventFilter(obj, event);
}

void TEditor::contextMenuEvent(QContextMenuEvent *event)
{
    // 1. إنشاء القائمة
    QMenu *menu = createStandardContextMenu(); // نبدأ بالقائمة القياسية للنظام

    // 2. تخصيص القائمة (إضافة خياراتنا)
    menu->addSeparator(); // خط فاصل

    // --- خيار تعليق/إلغاء تعليق ---
    QAction *commentAction = new QAction("تعليق/إلغاء تعليق", this);
    commentAction->setShortcut(QKeySequence("Ctrl+/"));
    connect(commentAction, &QAction::triggered, this, &TEditor::toggleComment);
    menu->addAction(commentAction);

    // --- خيار تكرار السطر ---
    QAction *duplicateAction = new QAction("تكرار السطر", this);
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(duplicateAction, &QAction::triggered, this, &TEditor::duplicateLine);
    menu->addAction(duplicateAction);

    // 3. تطبيق التصميم الداكن (لضمان ظهورها بشكل صحيح)
    // (يفضل وضع هذا في Taif.cpp، لكن سأضعه هنا مؤقتاً لضمان عمله فوراً)
    menu->setStyleSheet(
        "QMenu { background-color: #252526; color: #cccccc; border: 1px solid #454545; }"
        "QMenu::item { padding: 5px 20px; background-color: transparent; }"
        "QMenu::item:selected { background-color: #094771; color: #ffffff; }"
        "QMenu::separator { height: 1px; background: #454545; margin: 5px 0; }"
        );

    // 4. عرض القائمة في مكان الماوس
    menu->exec(event->globalPos());

    // 5. تنظيف الذاكرة
    delete menu;
}

int TEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // Increased width to accommodate line numbers
    int space = 30 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

// void TEditor::updateLineNumberAreaWidth() {
//     int width = lineNumberAreaWidth();
//     // Set viewport margins to create space for line number area on the Left
//     setViewportMargins(0, 0, width + 10, 0);

// }

void TEditor::updateLineNumberAreaWidth() {
    // عرض منطقة الأرقام
    int numsWidth = lineNumberAreaWidth();

    // عرض الخريطة
    int mapWidth = 0;
    if (minimap && minimap->isVisible()) {
        mapWidth = minimap->width();
    }

    // ✅ التصحيح:
    // المتغير الأول (يسار) -> للخريطة
    // المتغير الثالث (يمين) -> للأرقام
    setViewportMargins(mapWidth, 0, numsWidth, 0);
}

inline void TEditor::updateLineNumberArea(const QRect &rect, int dy) {
    // Trigger a repaint of the line number area
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

// void TEditor::resizeEvent(QResizeEvent* event) {
//     QPlainTextEdit::resizeEvent(event);

//     QRect cr = contentsRect();
//     int areaWidth = lineNumberAreaWidth();
//     // Position line number area on the Left
//     lineNumberArea->setGeometry(QRect(
//         cr.right() - areaWidth,
//         cr.top(),
//         areaWidth,
//         cr.height()
//     ));

//     // lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));

//     // 2. ضبط منطقة الخريطة المصغرة (يمين)
//     // if (minimap) {
//         int mapWidth = minimap->width();
//         minimap->setGeometry(QRect(cr.left() - mapWidth, cr.top(), mapWidth, cr.height()));
//     // }
// }

void TEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    int numsWidth = lineNumberAreaWidth();

    // ✅ 1. وضع أرقام الأسطر في أقصى اليمين
    // نستخدم this->width() للحصول على العرض الكلي للمحرر
    lineNumberArea->setGeometry(this->width() - numsWidth, cr.top(), numsWidth, cr.height());

    // ✅ 2. وضع الخريطة في أقصى اليسار (عند الإحداثي 0)
    if (minimap) {
        int mapWidth = minimap->width();
        minimap->setGeometry(25, cr.top(), mapWidth, cr.height());
    }
}

void TEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::transparent);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            painter.setPen(QColor(200, 200, 200));

            painter.drawText(12, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::AlignRight | Qt::AlignVCenter, number);

            // 🔹 رسم رمز الطي
            bool hasFold = false;
            for (const auto& region : foldRegions) {
                if (region.startBlockNumber == blockNumber) {
                    hasFold = true;
                    bool folded = region.folded;

                    // شكل السهم ▸ أو ▾
                    QPolygon arrow;
                    int midY = top + fontMetrics().height() / 2;
                    if (folded) {
                        arrow << QPoint(lineNumberArea->width() - 10, midY - 4)
                        << QPoint(lineNumberArea->width() - 2, midY)
                        << QPoint(lineNumberArea->width() - 10, midY + 4);
                    } else {
                        arrow << QPoint(lineNumberArea->width() - 10, midY - 4)
                        << QPoint(lineNumberArea->width() - 2, midY - 4)
                        << QPoint(lineNumberArea->width() - 6, midY + 4);
                    }

                    painter.setBrush(QColor("#10a8f4"));
                    painter.setPen(Qt::NoPen);
                    painter.drawPolygon(arrow);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void TEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(23, 24, 36, 240);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void TEditor::updateFoldRegions() {

    // حفظ حالة الطي الحالية
    QHash<int, bool> previousFoldStates;
    for (const FoldRegion& region : foldRegions) {
        previousFoldStates[region.startBlockNumber] = region.folded;
    }

    foldRegions.clear();
    QStack<int> braceStack;

    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        QString text = block.text();

        // ---- 2) معالجة بايثون: def / class (بسيط عن طريق المسافة البادئة)
        QString trimmed = text.trimmed();
        if (trimmed.startsWith("دالة ") || trimmed.startsWith("صنف ")) {
            int start = block.blockNumber();

            // حساب مستوى البادئة (indentation)
            int startIndent = 0;
            for (QChar c : text) {
                if (c == '\t') startIndent += 4;
                else if (c == ' ') startIndent += 1;
                else break;
            }

            QTextBlock next = block.next();
            int end = start;

            while (next.isValid()) {
                QString nextText = next.text();
                QString nextTrim = nextText.trimmed();

                // تخطي الأسطر الفارغة
                if (nextTrim.isEmpty()) {
                    next = next.next();
                    continue;
                }

                // احسب البادئة للسطر التالي
                int nextIndent = 0;
                for (QChar c : nextText) {
                    if (c == '\t') nextIndent += 4;
                    else if (c == ' ') nextIndent += 1;
                    else break;
                }

                // لو بدأنا بدالة أو صنف جديد بنفس أو أقل مستوى
                if (nextTrim.startsWith("دالة ") || nextTrim.startsWith("صنف ")) {
                    if (nextIndent <= startIndent)
                        break;
                }

                // إذا عاد للمستوى نفسه أو أقل، اعتبره نهاية الكتلة
                if (nextIndent <= startIndent)
                    break;

                end = next.blockNumber();
                next = next.next();
            }

            if (end > start) {
                FoldRegion region{};
                region.startBlockNumber = start;
                region.endBlockNumber = end;
                region.folded = false;
                if (previousFoldStates.contains(region.startBlockNumber))
                    region.folded = previousFoldStates[region.startBlockNumber];
                foldRegions.append(region);
            }
        }
        block = block.next();
    }

    // تحدِّث عمود الأرقام ليعيد الرسم
    if (lineNumberArea)
        lineNumberArea->update();

    for (const FoldRegion& region : foldRegions) {
        QTextBlock block = document()->findBlockByNumber(region.startBlockNumber + 1);
        while (block.isValid() && block.blockNumber() <= region.endBlockNumber) {
            block.setVisible(!region.folded);
            block = block.next();
        }
    }
    document()->markContentsDirty(0, document()->characterCount());
    viewport()->update();
}

void TEditor::toggleFold(int blockNumber) {
    for (FoldRegion &region : foldRegions) {
        if (region.startBlockNumber == blockNumber) {
            region.folded = !region.folded;

            QTextBlock block = document()->findBlockByNumber(region.startBlockNumber + 1);
            while (block.isValid() && block.blockNumber() <= region.endBlockNumber) {
                block.setVisible(!region.folded);
                block = block.next();
            }

            // ✅ التحديث الأهم: تحديث جميع المناطق الفرعية ضمن هذه المنطقة
            if (!region.folded) { // أي عند الفتح
                for (FoldRegion &subRegion : foldRegions) {
                    if (subRegion.startBlockNumber > region.startBlockNumber &&
                        subRegion.endBlockNumber <= region.endBlockNumber) {
                        // عدل حالة السهم إذا لزم
                        QTextBlock subBlock = document()->findBlockByNumber(subRegion.startBlockNumber + 1);
                        bool allVisible = true;
                        while (subBlock.isValid() && subBlock.blockNumber() <= subRegion.endBlockNumber) {
                            if (!subBlock.isVisible()) {
                                allVisible = false;
                                break;
                            }
                            subBlock = subBlock.next();
                        }
                        // إذا كانت الدالة مفتوحة فعلاً، عدل حالتها
                        subRegion.folded = !allVisible;
                    }
                }
            }

            document()->markContentsDirty(0, document()->characterCount());
            viewport()->update();
            break;
        }
    }
}


/* ---------------------------------- Drag and Drop ---------------------------------- */

void TEditor::dragEnterEvent(QDragEnterEvent* event) {
    // Check if the dragged data contains URLs (files)
    if (event->mimeData()->hasUrls()) {
        // Check if any of the URLs have a .alif ... extension
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.fileName().endsWith(".alif", Qt::CaseInsensitive) or
                url.fileName().endsWith(".aliflib", Qt::CaseInsensitive) or
                url.fileName().endsWith(".txt", Qt::CaseInsensitive)) {
                event->acceptProposedAction(); // Accept the drag event
                return;
            }
        }
    }

    // Mouse Text Drag
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        return;
    }
    event->ignore(); // Ignore if not a .alif ... file
}

void TEditor::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void TEditor::dropEvent(QDropEvent* event) {
    // Check if the dropped data contains URLs (files)
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.fileName().endsWith(".alif", Qt::CaseInsensitive) or
                url.fileName().endsWith(".aliflib", Qt::CaseInsensitive) or
                url.fileName().endsWith(".txt", Qt::CaseInsensitive)) {

                QString filePath = url.toLocalFile();
                emit openRequest(filePath);

                event->acceptProposedAction();
                return;
            }
        }
    }

    // Mouse Text Drop
    if (event->mimeData()->hasText()) {
        QTextCursor dropCursor = cursorForPosition(event->position().toPoint());
        int dropPosition = dropCursor.position();

        // The text is being moved, not just dropped from an external source.
        // So we handle it completely.

        // If the drop is within the selection, do nothing.
        if (dropPosition >= textCursor().selectionStart()
            and dropPosition <= textCursor().selectionEnd()) {
            event->ignore();
            return;
        }

        QString droppedText = event->mimeData()->text();
        QTextCursor originalCursor = textCursor();

        // Remove the original selected text FIRST.
        originalCursor.removeSelectedText();

        // Adjust the drop position if the removal occurred before it.
        if (originalCursor.position() < dropPosition) {
            dropPosition -= droppedText.length();
        }

        // Insert the text at the correct, adjusted position.
        dropCursor.setPosition(dropPosition);
        dropCursor.insertText(droppedText);

        event->acceptProposedAction();
        return;
    }

    event->ignore(); // Ignore if not a .alif ... file
}

void TEditor::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}


/* ---------------------------------- Indentation ---------------------------------- */

void TEditor::curserIndentation() {
    QTextCursor cursor = textCursor();
    QString lineText = cursor.block().text();
    int cursorPosInLine = cursor.positionInBlock();
    QString currentIndentation = getCurrentLineIndentation(cursor);

    // Check if the cursor is not at the very beginning of the line
    if (cursorPosInLine > 0) {
        int checkPos = cursorPosInLine - 1;
        while (checkPos >= 0 and lineText.at(checkPos).isSpace()) {
            checkPos--;
        }

        if (checkPos >= 0 and lineText.at(checkPos) == ':') {
            currentIndentation += "\t";
        }
    }

    cursor.beginEditBlock();
    cursor.insertText("\n" + currentIndentation);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

QString TEditor::getCurrentLineIndentation(const QTextCursor &cursor) const {
    QTextBlock block = cursor.block();
    if (!block.isValid()) {
        return QString();
    }

    QString lineText = block.text();
    QString indentation;
    for (const QChar &ch : lineText) {
        if (ch == ' ' or ch == '\t') {
            indentation += ch;
        } else {
            break;
        }
    }
    return indentation;
}




void TEditor::startAutoSave() {
    if (!autoSaveTimer->isActive()) {
        autoSaveTimer->start();
    }
}

void TEditor::stopAutoSave() {
    autoSaveTimer->stop();
}

void TEditor::performAutoSave() {
    // 1. تحقق هل يوجد مسار للملف وهل هو معدل؟
    QString filePath = this->property("filePath").toString();
    if (filePath.isEmpty() || !this->document()->isModified()) return;

    // 2. أنشئ اسم ملف النسخة الاحتياطية (مثلاً: file.alif.~)
    QString backupPath = filePath + ".~";

    // 3. احفظ المحتوى فيه
    QFile file(backupPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << this->toPlainText();
        file.close();
        // (اختياري) طباعة في الكونسول للمراقبة
        // qDebug() << "Auto-saved to:" << backupPath;
    }
}

void TEditor::removeBackupFile() {
    QString filePath = this->property("filePath").toString();
    if (filePath.isEmpty()) return;

    QString backupPath = filePath + ".~";
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath); // احذف النسخة الاحتياطية لأننا حفظنا الأصلي
    }
    stopAutoSave(); // أوقف المؤقت حتى يحدث تعديل جديد
}
