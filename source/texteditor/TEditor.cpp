#include "TEditor.h"
#include "THighlighter.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMimeData>
#include <QSettings>
#include <QPainterPath>
#include <QStack>


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

    updateLineNumberAreaWidth();
    highlightCurrentLine();

    // load saved font size
    QSettings settingsVal("Alif", "Taif");
    int savedSize = settingsVal.value("editorFontSize").toInt();
    updateFontSize(savedSize);

    highlighter = new THighlighter(this->document());
    // highlighter->loadSyntaxDefinition(":/syntax/alif.json");
    // Handle special key events
    installEventFilter(this); // for SHIFT + ENTER it's make line without number
}

void TEditor::updateFontSize(int size) {
    if (size < 10) {
        size = 16;
    }

    QFont font = this->font(); // Get current font
    font.setPointSize(size);
    this->setFont(font);

    QFont fontNums = lineNumberArea->font();
    fontNums.setPointSize(size);
    lineNumberArea->setFont(fontNums);
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

void TEditor::updateLineNumberAreaWidth() {
    int width = lineNumberAreaWidth();
    // Set viewport margins to create space for line number area on the Left
    setViewportMargins(0, 0, width + 10, 0);
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

void TEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    int areaWidth = lineNumberAreaWidth();
    // Position line number area on the Left
    lineNumberArea->setGeometry(QRect(
        cr.right() - areaWidth,
        cr.top(),
        areaWidth,
        cr.height()
    ));
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

    // 🔹 حفظ حالة الطي الحالية
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

void TEditor::dragMoveEvent(QDragMoveEvent* event) { // ضروري لمنع ظهور سلوك غريب بعد الإفلات
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
