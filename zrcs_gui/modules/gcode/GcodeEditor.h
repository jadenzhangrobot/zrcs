#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QLabel>
#include <QPushButton>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextBlock>
#include <QFile>
#include <QPaintEvent>

class GCodeLineNumberArea;

/**
 * G-code 语法高亮器
 */
class GCodeHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit GCodeHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression pattern;
};

/**
 * G-code 编辑器
 * 支持语法高亮、行号显示、代码验证
 */
class GCodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit GCodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    QWidget *lineNumberArea;
    GCodeHighlighter *highlighter;
};

/**
 * G-code 编辑面板
 * 集成编辑器、验证、统计等功能
 */
class GCodePanel : public QWidget {
    Q_OBJECT

public:
    explicit GCodePanel(QWidget *parent = nullptr);
    ~GCodePanel();

    void loadFile(const QString &filePath);
    void saveFile();
    QStringList validateGCode();
    void updateStatistics();

signals:
    void fileLoaded(const QString &filePath);
    void fileSaved(const QString &filePath);
    void validationCompleted(const QStringList &errors);

private:
    void setupUI();

    GCodeEditor *editor;
    QPushButton *openButton;
    QPushButton *saveButton;
    QPushButton *validateButton;
    QLabel *statsLabel;
    QLabel *validationLabel;
};

