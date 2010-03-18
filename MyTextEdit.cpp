#include "MyTextEdit.h"

#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>
#include <QDomDocument>

MyTextEdit::MyTextEdit(QWidget *parent) :
	QPlainTextEdit(parent),
	m_contextMenu(this),
	m_tabulatorWidth(8)
{
	setLineWrapMode(QPlainTextEdit::NoWrap);

	setTabStopWidth(40);

	m_lineNumbers = new MyLineNumbers(this);

	connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
	connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightLines()));

	updateLineNumberAreaWidth(0);
	highlightLines();
//	highlightCurrentLine();
}

MyTextEdit::~MyTextEdit()
{
	delete m_lineNumbers;
}

void MyTextEdit::keyPressEvent(QKeyEvent * e)
{
	if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
	{
		QString text = textCursor().block().text();
		int i = 0;
		while(text[i].isSpace() && i < text.size())
			++i;

		QPlainTextEdit::keyPressEvent(e);
		textCursor().insertText(text.left(i));
	}
	else
		QPlainTextEdit::keyPressEvent(e);
}

void MyTextEdit::contextMenuEvent(QContextMenuEvent * event)
{
	m_contextMenu.exec(event->globalPos());
}

void MyTextEdit::addContextMenuAction(QAction * action)
{
	if(action == NULL)
		m_contextMenu.addSeparator();
	else
		m_contextMenu.addAction(action);
}

int MyTextEdit::lineNumberAreaWidth()
{
	int digits = 1;
	int max = qMax(1, blockCount());
	while (max >= 10)
	{
		max /= 10;
		++digits;
	}

	int space = 10 + fontMetrics().width(QLatin1Char('9')) * digits;

	return space;
 }

void MyTextEdit::updateLineNumberAreaWidth(int newBlockCount)
{
	Q_UNUSED(newBlockCount);

	setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void MyTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
	if (dy)
		m_lineNumbers->scroll(0, dy);
	else
		m_lineNumbers->update(0, rect.y(), m_lineNumbers->width(), rect.height());

	if (rect.contains(viewport()->rect()))
		updateLineNumberAreaWidth(0);
}

void MyTextEdit::resizeEvent(QResizeEvent *e)
{
	QPlainTextEdit::resizeEvent(e);

	QRect cr = contentsRect();
	m_lineNumbers->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void MyTextEdit::addHighlightedLine(int line, const QColor & c)
{
	m_highlightedLines.insert(line, c);
}

void MyTextEdit::clearHighlightedLines()
{
	m_highlightedLines.clear();
}

void MyTextEdit::highlightLines()
 {
	 QList<QTextEdit::ExtraSelection> extraSelections;

	 if (!isReadOnly())
	 {
		 QTextEdit::ExtraSelection selection;

		 for(QMap<int, QColor>::const_iterator it = m_highlightedLines.begin(); it != m_highlightedLines.end(); ++it)
		 {
			 selection.format.setBackground(it.value());
			 selection.format.setProperty(QTextFormat::FullWidthSelection, true);
			 selection.cursor = QTextCursor(document());
			 selection.cursor.setPosition(document()->findBlockByLineNumber(it.key()).position());
			 extraSelections.append(selection);
		 }

		 selection.format.setBackground(QColor(Qt::yellow).lighter(170));
		 selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		 selection.cursor = textCursor();
		 selection.cursor.clearSelection();
		 extraSelections.append(selection);
	 }

	 setExtraSelections(extraSelections);
 }

void MyTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
	QPainter painter(m_lineNumbers);
	painter.fillRect(event->rect(), Qt::lightGray);
	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
	int bottom = top + (int) blockBoundingRect(block).height();

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			QString number = QString::number(blockNumber + 1);
			painter.setPen(Qt::black);
			painter.drawText(-2, top, m_lineNumbers->width(), fontMetrics().height(), Qt::AlignRight, number);
		}

		block = block.next();
		top = bottom;
		bottom = top + (int) blockBoundingRect(block).height();
		++blockNumber;
	}
}

Highlighter::Highlighter(QTextDocument * parent) : QSyntaxHighlighter(parent)
{
        QFile f(QDir::current().filePath("HighlighterRules.xml"));
	if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
                qWarning() << "Cannot open XML " << f.fileName() << ". Higlighting won't work.";
		return;
	}

	QDomDocument domDoc;
	QString errorMsg;
	int errorLine, errorColumn;
	domDoc.setContent(&f, &errorMsg, &errorLine, &errorColumn);

	QDomElement root = domDoc.documentElement();
	if(root.tagName() != "highlighterRules")
	{
                qWarning() << f.fileName() << " is not valid Highlighter XML. Higlighting won't work.";
		return;
	}

	QDomElement rule = root.firstChildElement("rule");
	while(!rule.isNull())
	{
		Rule r;
		QTextCharFormat format;
		format.setFontItalic(rule.attribute("italic", "false") == "true");
		format.setFontWeight(rule.attribute("bold", "false") == "true" ? QFont::Black : QFont::Normal);
		format.setForeground(QBrush(QColor(rule.attribute("color", "#000000"))));
		r.format = format;

		QDomElement pattern = rule.firstChildElement("pattern");
		while(!pattern.isNull())
		{
			r.pattern = QRegExp(pattern.text(), Qt::CaseInsensitive);
			m_rules.push_back(r);
			pattern = pattern.nextSiblingElement("pattern");
		}
		rule = rule.nextSiblingElement("rule");
	}

	f.close();
}

void Highlighter::highlightBlock(const QString &text)
{
	foreach (const Rule & rule, m_rules)
	{
		int index = rule.pattern.indexIn(text);
		while (index >= 0)
		{
			int length = rule.pattern.matchedLength();
			setFormat(index, length, rule.format);
			index = rule.pattern.indexIn(text, index + length);
		}
	}
}
