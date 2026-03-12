#include "gcode/gcodeEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QSyntaxHighlighter>
#include <QTextDocument>

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
    setStyleSheet("background-color: #1a1a1a; color: #00FF00; font-family: Courier;");
    new GCodeHighlighter(document());
}

int GCodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
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
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    editor = new GCodeEditor();
    mainLayout->addWidget(editor);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    openButton = new QPushButton("打开");
    saveButton = new QPushButton("保存");
    validateButton = new QPushButton("验证");
    
    openButton->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; padding: 8px;");
    saveButton->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; padding: 8px;");
    validateButton->setStyleSheet("background-color: #2a5a2a; color: #00FF00; border: 1px solid #00FF00; padding: 8px;");
    
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
            validationLabel->setStyleSheet("color: #00FF00; padding: 5px;");
        } else {
            validationLabel->setText("✗ 验证失败: " + errors.join("; "));
            validationLabel->setStyleSheet("color: #FF6347; padding: 5px;");
        }
    });
    
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    statsLabel = new QLabel("行数: 0 | G 指令: 0 | M 指令: 0");
    statsLabel->setStyleSheet("color: #00FF00; padding: 5px;");
    mainLayout->addWidget(statsLabel);
    
    validationLabel = new QLabel("");
    validationLabel->setStyleSheet("color: #FF6347; padding: 5px;");
    mainLayout->addWidget(validationLabel);
    
    setStyleSheet("background-color: #1a1a1a;");
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
            validationLabel->setStyleSheet("color: #00FF00; padding: 5px;");
        }
    }
}

QStringList GCodePanel::validateGCode()
{
    QStringList errors;
    QString text = editor->toPlainText();
    QStringList lines = text.split('\n');
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty() || line.startsWith(';')) continue;
        
        // 简单的验证逻辑
        if (!line.contains(QRegularExpression("^[GM]\\d+"))) {
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
