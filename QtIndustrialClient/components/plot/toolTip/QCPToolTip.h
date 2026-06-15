#pragma once

#include "../qcustomplot/qcustomplot.h"

class QCPToolTip : public QCPLayerable
{
	Q_OBJECT

public:
	explicit QCPToolTip(QCustomPlot* parentPlot);

	void setText(const QString& text);
	void setAnchor(const QPointF& pos); // 图坐标
	void setVisible(bool on);

	void setText(const QPoint& pos, const QString& text);

protected:
	void applyDefaultAntialiasingHint(QCPPainter* painter) const
	{
		painter->setAntialiasing(true);
	}

	void draw(QCPPainter* painter) override;

private:
	QString m_text;
	QPoint  mAnchor;
	QRectF  mBounding;

	Qt::Alignment m_textAlignment;
	QFont         m_font;
	QColor        m_textColor;
	QPen          m_borderPen;
	QBrush        m_brush;

	QPointF      m_radius;
	Qt::SizeMode m_sizeMode;

	QPointF  m_offset; // 偏移鼠标的距离
	QMargins m_padding;

	QCPGraph* m_highlightGraph;
	QPointF   m_graphDataPos;
};
