#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QSyntaxHighlighter>
#include <QMenu>
#include <QPlainTextEdit>

#include <QDebug>

class MyLineNumbers;
class Highlighter;

class MyTextEdit : public QPlainTextEdit
{
	Q_OBJECT
	public:
		MyTextEdit(QWidget * parent = 0);
		~MyTextEdit();

		void addContextMenuAction(QAction * action);

		void lineNumberAreaPaintEvent(QPaintEvent *event);
		int lineNumberAreaWidth();
		void addHighlightedLine(int line, const QColor & c);
		void clearHighlightedLines();
		void setTabulatorWidth(int w);

	protected:
		void contextMenuEvent(QContextMenuEvent * event);
		void resizeEvent(QResizeEvent * event);

		virtual void keyPressEvent(QKeyEvent * e);

	private:
		QMenu m_contextMenu;
		MyLineNumbers * m_lineNumbers;
		QMap<int, QColor> m_highlightedLines;
		int m_tabulatorWidth;

	public slots:
		void updateLineNumberAreaWidth(int newBlockCount);
		void highlightLines();

	private slots:
		void updateLineNumberArea(const QRect &, int);
};

class MyLineNumbers : public QWidget
{
	public:
		MyLineNumbers(MyTextEdit * t) : QWidget(t), m_textEdit(t)
		{
		}

		QSize sizeHint() const
		{
			return QSize(m_textEdit->lineNumberAreaWidth(), 0);
		}

	protected:
		void paintEvent(QPaintEvent * event)
		{
			m_textEdit->lineNumberAreaPaintEvent(event);
		}

	private:
		MyTextEdit * m_textEdit;
};

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	public:
		Highlighter(QTextDocument *parent = 0);

	protected:
		void highlightBlock(const QString &text);

	private:
		struct Rule
		{
			QRegExp pattern;
			QTextCharFormat format;
		};

		QList<Rule> m_rules;
 };

#endif // MYTEXTEDIT_H
