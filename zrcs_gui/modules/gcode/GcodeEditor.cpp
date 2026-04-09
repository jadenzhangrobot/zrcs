#include "gcode/GcodeEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QPainter>
#include <QTextBlock>
#include <QStyle>
#include "ui_gcode_panel.h"

class GCodeLineNumberArea : public QWidget {
public:
    explicit GCodeLineNumberArea(GCodeEditor *editor)
        : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    GCodeEditor *codeEditor;
};

// ============================================================================
// GCodeHighlighter 实现
// ============================================================================

GCodeHighlighter::GCodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    pattern = QRegularExpression("\\b[GM]\\d+\\b");
}

void GCodeHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat format;
    format.setForeground(Qt::green);
    
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), format);
    }
}

// ============================================================================
// GCodeEditor 实现
// ============================================================================

GCodeEditor::GCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setObjectName("gcodeEditor");
    lineNumberArea = new GCodeLineNumberArea(this);
    highlighter = new GCodeHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged, this, &GCodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &GCodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &GCodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int GCodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void GCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#12161e"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#6f7f96"));
            painter.drawText(0, top, lineNumberArea->width() - 6, fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void GCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void GCodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void GCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void GCodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor("#1f2a3a"));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
    setExtraSelections(extraSelections);
}

// ============================================================================
// GCodePanel 实现
// ============================================================================

GCodePanel::GCodePanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

GCodePanel::~GCodePanel()
{
}

void GCodePanel::setupUI()
{
    Ui::GCodePanelUi ui;
    ui.setupUi(this);
    editor = findChild<GCodeEditor*>("editor");
    openButton = findChild<QPushButton*>("openButton");
    saveButton = findChild<QPushButton*>("saveButton");
    validateButton = findChild<QPushButton*>("validateButton");
    statsLabel = findChild<QLabel*>("statsLabel");
    validationLabel = findChild<QLabel*>("validationLabel");
    if (!editor || !openButton || !saveButton || !validateButton || !statsLabel || !validationLabel) {
        return;
    }
    
    openButton->setProperty("kind", "accent");
    saveButton->setProperty("kind", "accent");
    validateButton->setProperty("kind", "accent");
    
    connect(openButton, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "打开 G-code 文件", "", "G-code 文件 (*.gcode *.nc)");
        if (!filePath.isEmpty()) {
            loadFile(filePath);
        }
    });
    
    connect(saveButton, &QPushButton::clicked, this, &GCodePanel::saveFile);
    connect(validateButton, &QPushButton::clicked, this, [this]() {
        QStringList errors = validateGCode();
        if (errors.isEmpty()) {
            validationLabel->setText("✓ 验证通过");
            validationLabel->setProperty("state", "ok");
        } else {
            validationLabel->setText("✗ 验证失败: " + errors.join("; "));
            validationLabel->setProperty("state", "error");
        }
        style()->unpolish(validationLabel);
        style()->polish(validationLabel);
    });
    
    statsLabel->setText("行数: 0 | G 指令: 0 | M 指令: 0");
    statsLabel->setObjectName("gcodeStatsLabel");
    
    validationLabel->setText("");
    validationLabel->setObjectName("gcodeValidationLabel");
}


void GCodePanel::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        editor->setPlainText(file.readAll());
        file.close();
        updateStatistics();
    }
}

void GCodePanel::saveFile()
{
    QString filePath = QFileDialog::getSaveFileName(this, "保存 G-code 文件", "", "G-code 文件 (*.gcode)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(editor->toPlainText().toUtf8());
            file.close();
            validationLabel->setText("✓ 文件已保存");
            validationLabel->setProperty("state", "ok");
            style()->unpolish(validationLabel);
            style()->polish(validationLabel);
        }
    }
}

QStringList GCodePanel::validateGCode()
{
    QStringList errors;
    QString text = editor->toPlainText();
    QStringList lines = text.split('\n');
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        int semicolonPos = line.indexOf(';');
        if (semicolonPos >= 0) {
            line = line.left(semicolonPos);
        }
        line.remove(QRegularExpression("\\([^\\)]*\\)"));
        line = line.trimmed();
        if (line.isEmpty()) continue;
        
        // 简单的验证逻辑
        if (!line.contains(QRegularExpression("^[GM]\\d+", QRegularExpression::CaseInsensitiveOption))) {
            errors.append(QString("第 %1 行: 无效的 G-code 指令").arg(i + 1));
        }
    }
    
    return errors;
}

void GCodePanel::updateStatistics()
{
    QString text = editor->toPlainText();
    QStringList lines = text.split('\n');
    
    int gCount = 0, mCount = 0;
    for (const QString &line : lines) {
        if (line.contains(QRegularExpression("\\bG\\d+"))) gCount++;
        if (line.contains(QRegularExpression("\\bM\\d+"))) mCount++;
    }
    
    statsLabel->setText(QString("行数: %1 | G 指令: %2 | M 指令: %3 | 预计时间: 0 分钟")
        .arg(lines.size()).arg(gCount).arg(mCount));
}
