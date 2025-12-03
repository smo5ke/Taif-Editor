#pragma once

#include <QTimer>
#include <QScrollBar>

#include "THighlighter.h"
#include "AlifComplete.h"


class LineNumberArea;
class TEditor;


class TMinimap : public QWidget {
    Q_OBJECT
public:
    explicit TMinimap(TEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    TEditor *editor;
    void scrollEditorTo(const QPoint &pos);
};


class TEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    TEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;
    QString filePath{};

    QString getCurrentLineIndentation(const QTextCursor &cursor) const;
    void curserIndentation();


    void startAutoSave();
    void stopAutoSave();
    void removeBackupFile();

public slots:
    void updateFontSize(int);
    void toggleComment();
    void duplicateLine();
    void moveLineUp();
    void moveLineDown();
    void performAutoSave();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    THighlighter* highlighter{};
    AutoComplete* autoComplete{};
    LineNumberArea* lineNumberArea{};

    struct FoldRegion {
        int startBlockNumber;
        int endBlockNumber;
        bool folded = false;
    };
    QVector<FoldRegion> foldRegions;

    void updateFoldRegions();
    void toggleFold(int blockNum);


    QTimer *autoSaveTimer;

    friend class TMinimap;
    friend class LineNumberArea;
    TMinimap *minimap;

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    inline void updateLineNumberArea(const QRect &rect, int dy);
    void updateMinimap();
signals:
    void openRequest(QString filePath);
};


class LineNumberArea : public QWidget {
public:
    LineNumberArea(TEditor* editor) : QWidget(editor), tEditor(editor) {
        this->setStyleSheet(
            "QWidget {"
            "   border-left: 1px solid #10a8f4;"
            "   border-top-left-radius: 9px;"        // Rounded top-left corner
            "   border-bottom-left-radius: 9px;"     // Rounded bottom-left corner
            "}"
        );
    }

    QSize sizeHint() const override {
        return QSize(tEditor->lineNumberAreaWidth(), 0);
    }

    void mousePressEvent(QMouseEvent* event) override {
        int y = event->position().y();
        QTextBlock block = tEditor->firstVisibleBlock();
        int top = qRound(tEditor->blockBoundingGeometry(block).translated(tEditor->contentOffset()).top());
        int height = qRound(tEditor->blockBoundingRect(block).height());

        while (block.isValid() && top <= y) {
            if (y >= top && y < top + height) {
                int blockNum = block.blockNumber();
                tEditor->toggleFold(blockNum);
                return;
            }
            block = block.next();
            top += height;
            height = qRound(tEditor->blockBoundingRect(block).height());
        }
    }


protected:
    void paintEvent(QPaintEvent* event) override {
        tEditor->lineNumberAreaPaintEvent(event);
    }


private:
    TEditor* tEditor{};
};
