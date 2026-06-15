#include "QCPToolTip.h"

QCPToolTip::QCPToolTip(QCustomPlot* parentPlot)
	: QCPLayerable(parentPlot), m_text(""), mAnchor(0, 0)
{
	setLayer("overlay"); // 放在最顶层
	setVisible(false);

	m_borderPen = QPen(Qt::NoPen);
	m_brush     = QColor(87, 98, 93, 180);
	m_offset    = QPoint(20, 20);
	m_padding   = QMargins(6, 6, 6, 6);
	m_textColor = QColor(Qt::white);
	m_font.setPixelSize(12);
	m_font.setFamilies({QString::fromUtf8("Microsoft YaHei")});
}

void QCPToolTip::setText(const QString& text)
{
	m_text = text;
	layer()->replot();
}

void QCPToolTip::setAnchor(const QPointF& pos)
{
	mAnchor = QPoint(mParentPlot->xAxis->coordToPixel(mAnchor.x()),
	                 mParentPlot->yAxis->coordToPixel(mAnchor.y()));
	layer()->replot();
}

void QCPToolTip::setVisible(bool on)
{
	if (visible() == on)return;
	QCPLayerable::setVisible(on);
	layer()->replot();
}

void QCPToolTip::setText(const QPoint& pos, const QString& text)
{
	QCPLayerable::setVisible(true);
	m_text  = text;
	mAnchor = pos;
	layer()->replot();
}

void QCPToolTip::draw(QCPPainter* painter)
{
	if (!mVisible) return;

	QPointF pos = mAnchor + m_offset;
	painter->translate(pos); // 移动painter的绘制原点位置

	QFontMetrics fontMetrics(m_font);
	QRect        textRect = fontMetrics.boundingRect(0, 0, 0, 0, Qt::TextDontClip | m_textAlignment, m_text);
	textRect.moveTopLeft(QPoint(m_padding.left(), m_padding.top()));

	QRect textBoxRect = textRect.adjusted(-m_padding.left(), -m_padding.top(), m_padding.right(), m_padding.bottom());
	textBoxRect.moveTopLeft(QPoint());

	// 限制ToolTip不超过QCustomPlot的范围
	if (pos.x() + textBoxRect.width() >= mParentPlot->viewport().right())
		painter->translate(-m_offset.x() * 2 - textBoxRect.width(), 0);
	if (pos.y() + textBoxRect.height() * 2 >= mParentPlot->viewport().bottom())
		painter->translate(0, -m_offset.y() * 2 - textBoxRect.height());

	// 绘制背景和边框
	if ((m_brush != Qt::NoBrush && m_brush.color().alpha() != 0) ||
		(m_borderPen != Qt::NoPen && m_borderPen.color().alpha() != 0))
	{
		double clipPad      = m_borderPen.widthF();
		QRect  boundingRect = textBoxRect.adjusted(-clipPad, -clipPad, clipPad, clipPad);

		painter->setPen(m_borderPen);
		painter->setBrush(m_brush);
		painter->drawRoundedRect(boundingRect, m_radius.x(), m_radius.y(), m_sizeMode);
	}

	// 绘制文字
	painter->setFont(m_font);
	painter->setPen(m_textColor);
	painter->setBrush(Qt::NoBrush);
	painter->drawText(textRect, Qt::TextDontClip | m_textAlignment, m_text);
}
