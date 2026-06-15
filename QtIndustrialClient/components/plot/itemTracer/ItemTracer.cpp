#include "ItemTracer.h"
#include <QDebug>
#include <QGraphicsDropShadowEffect>

ItemTracer::ItemTracer(QCustomPlot* parent)
	: QCPAbstractItem(parent)
//, mTextItem{new QCPItemText(parent)}
{
	m_brush         = QBrush(QColor(255, 255, 255), Qt::SolidPattern);
	m_textAlignment = Qt::AlignLeft;
	m_font          = QFont("Arial", 9);
	m_textColor     = "#5e6d82";
	setSelectable(false);
	setSelected(false);
	m_offset  = QPoint(20, 20);
	m_decimal = 6;
	setLayerName("tracer");

	connect(mParentPlot, &QCustomPlot::mouseMove, this, &ItemTracer::handleMouseMoveEvent);
}

ItemTracer::~ItemTracer()
{
}

void ItemTracer::setLayerName(const QString& name)
{
	m_layerName = name;
	if (!mParentPlot->layer(name))
	{
		mParentPlot->addLayer(name);
	}
	setLayer(name);
}

void ItemTracer::setType(int type)
{
	m_type = type;
	if (type == 1)
	{
		setShowPopText(false);
	}
}

void ItemTracer::setShowPopText(bool b)
{
	m_showPopText = b;
}

void ItemTracer::setDecimal(int decimal)
{
	m_decimal = decimal;
}

void ItemTracer::updateTracer()
{
	// 获取鼠标在 QCustomPlot 内的坐标
	QPoint globalPos = QCursor::pos();
	QPoint plotPos   = mParentPlot->mapFromGlobal(globalPos);
	if (!mParentPlot->axisRect()->rect().contains(plotPos))
	{
		return;
	}
	m_center = plotPos;

	m_hitPointList.clear();

	for (int i = 0; i < mParentPlot->graphCount(); i++)
	{
		auto graph = mParentPlot->graph(i);
		if (!graph->visible()) continue;
		if (graph->data()->isEmpty()) continue;
		auto xAxis = graph->keyAxis();
		auto yAxis = graph->valueAxis();
		if (!xAxis || !yAxis) continue;

		// 先判断鼠标是否在轴范围内
		if (!xAxis->axisRect()->rect().contains(plotPos)) continue;

		double xCoord = xAxis->pixelToCoord(plotPos.x());
		double yCoord = yAxis->pixelToCoord(plotPos.y());

		bool foundRange = false;
		auto keyRange   = graph->data()->keyRange(foundRange);
		if (xCoord < keyRange.lower || xCoord > keyRange.upper) continue;

		int idx = findXCoord(graph, xCoord, yCoord, false);
		if (idx >= graph->data()->size()) continue;

		HitPoint hp;
		hp.lineName   = graph->name();
		hp.lineType   = graph->property("lineType").toString();
		hp.xCoord     = graph->data()->at(idx)->mainKey();
		hp.yCoodValue = graph->data()->at(idx)->mainValue();
		hp.xAxis      = xAxis;
		hp.yAxis      = yAxis;
		hp.nXIndex    = idx;
		m_hitPointList.push_back(hp);
	}

	//更新x轴坐标
	if (!m_hitPointList.empty())
	{
		const auto hitPointFirst = m_hitPointList.first();
		m_xCoodValue             = hitPointFirst.xCoord;
	}
}

void ItemTracer::handleMouseMoveEvent(QMouseEvent* event)
{
	if (!visible())
	{
		return;
	}
	if (0 == m_type)
	{
		updateTracer();
	}
	else if (1 == m_type)
	{
		handleType_1(event);
	}
	QCPLayer* layer = parentPlot()->layer(m_layerName);
	if (layer)
	{
		layer->replot();
	}
}

void ItemTracer::handleMousePressEvent(QMouseEvent* event)
{
}

void ItemTracer::handleMouseReleaseEvent(QMouseEvent* event)
{
}

double ItemTracer::selectTest(const QPointF& pos, bool onlySelectable, QVariant* details) const
{
	return -1;
}

void ItemTracer::draw(QCPPainter* painter)
{
	if (m_type != 0)
	{
		return;
	}

	// 检查鼠标是否在绘图区域内
	QPoint cursorPos = mParentPlot->mapFromGlobal(QCursor::pos());
	if (!mParentPlot->axisRect()->rect().contains(cursorPos))
	{
		return;
	}

	// 绘制命中点
	for (const auto& hitPoint : m_hitPointList)
	{
		QPointF point;
		point.setX(hitPoint.xAxis->coordToPixel(m_xCoodValue));
		point.setY(hitPoint.yAxis->coordToPixel(hitPoint.yCoodValue));
		QPen pen = painter->pen();
		painter->setPen(pen);
		painter->setBrush(Qt::white);
		pen.setColor(Qt::white);
		pen.setColor(QColor(35, 84, 164));
		painter->setPen(pen);
		QPainterPath path;
		path.addEllipse(point, 5, 5);
		path.addEllipse(point, 3, 3);
		painter->drawPath(path);
	}
	if (!m_hitPointList.isEmpty())
	{
		// 绘制虚线
		QPen pen = painter->pen();
		pen.setColor(Qt::black);
		pen.setStyle(Qt::PenStyle::DashLine);
		painter->setPen(pen);
		int bottomOffset = 0;
		painter->drawLine(QLineF(m_center.x(), clipRect().top(), m_center.x(), clipRect().bottom() - bottomOffset));
#if 1
		if (m_showPopText)
		{
			painter->save();

			// 创建文本字符串
			QString     strText = QString("x: %1\n").arg(m_xCoodValue, 0, 'f', m_decimal);
			QStringList lines;
			lines.append(strText);

			for (const auto& hitPoint : m_hitPointList)
			{
				if (!hitPoint.lineName.isEmpty())
				{
					lines.append(QString("%1: %2").arg(hitPoint.lineName).arg(hitPoint.yCoodValue, 0, 'f', 6));
				}
			}

			// 获取每行文本的矩形大小，并计算总的高度
			const QFontMetrics fontMetrics(m_font);
			int                totalHeight = 0;
			int                maxWidth    = 0;
			for (const QString& line : lines)
			{
				QRect lineRect = fontMetrics.boundingRect(QRect(0, 0, 1000, 1000), Qt::TextDontClip | Qt::AlignLeft,
				                                          line);
				totalHeight += lineRect.height();
				if (lineRect.width() > maxWidth)
				{
					maxWidth = lineRect.width();
				}
			}

			// 考虑1.5倍行间距
			const int adjustedHeight = totalHeight + (lines.count() - 1) * 0.5 * fontMetrics.height();

			// 调整文本框大小以包裹文本，并添加一定的边距
			constexpr int padding     = 5;
			constexpr int margin      = 10; // 距离边缘的最小边距
			auto          textBoxRect = QRect(0, 0, maxWidth + padding * 2, adjustedHeight);

			// 计算初始位置
			int translateX = m_center.x() + m_offset.x();
			int translateY = m_center.y() + m_offset.y();

			// 获取绘图区域的边界
			QRect plotRect = mParentPlot->rect();

			// 调整水平位置，防止超出左右边界
			if (translateX + textBoxRect.width() > plotRect.right() - margin)
			{
				// 尝试放在鼠标左侧
				translateX = m_center.x() - m_offset.x() - textBoxRect.width();
				// 如果还是超出左边界，就紧贴左边界
				if (translateX < margin)
				{
					translateX = margin;
				}
			}
			// 防止超出左边界
			else if (translateX < margin)
			{
				translateX = margin;
			}

			// 调整垂直位置，防止超出上下边界
			if (translateY + textBoxRect.height() > plotRect.bottom() - margin)
			{
				// 尝试放在鼠标上方
				translateY = m_center.y() - m_offset.y() - textBoxRect.height();
				// 如果还是超出上边界，就紧贴上边界
				if (translateY < margin)
				{
					translateY = margin;
				}
			}
			// 防止超出上边界
			else if (translateY < margin)
			{
				translateY = margin;
			}

			// 应用变换
			painter->translate(translateX, translateY);

			// 绘制带阴影的圆角文本框
			const QColor shadowColor(QColor::fromRgb(50, 50, 50, 30));
			painter->setPen(shadowColor);
			painter->setBrush(Qt::white);
			painter->drawRoundedRect(textBoxRect, 3, 3);

			// 绘制文本
			painter->setPen(m_textColor);
			painter->setFont(m_font);
			int yPos = textBoxRect.y() + padding + fontMetrics.ascent() + 2;
			for (QString line : lines)
			{
				painter->drawText(textBoxRect.x() + padding, yPos, line.replace("nan", "N/A"));
				yPos += fontMetrics.height() * 1.5;
			}

			painter->restore();
		}
#endif
	}
}


int ItemTracer::findXCoord(QCPGraph* graph, double xValue, double yValue, bool isDecreasing)
{
	if (!graph) return -1;

	auto data = graph->data();
	if (data->size() == 0) return -1;

	int low  = 0;
	int high = data->size() - 1;

	// 如果逆序，需要翻转逻辑
	auto getKey = [&](int idx) -> double
	{
		return isDecreasing ? data->at(high - idx)->mainKey() : data->at(idx)->mainKey();
	};

	// 二分查找最接近 xValue 的索引
	while (low < high)
	{
		int    mid    = (low + high) / 2;
		double midKey = getKey(mid);

		if (midKey < xValue)
			low = mid + 1;
		else
			high = mid;
	}

	// low 可能是最接近的点，也可能是前一个点
	int idx = low;
	if (idx > 0)
	{
		double distCurr = std::abs(getKey(idx) - xValue);
		double distPrev = std::abs(getKey(idx - 1) - xValue);
		if (distPrev < distCurr)
			idx = idx - 1;
	}

	// 如果是逆序，需要转换回原始索引
	return isDecreasing ? high - idx : idx;
}

void ItemTracer::handleType_1(QMouseEvent* event)
{
	int nXIndex = -1;

	m_center           = event->pos();
	double      xCoord = mParentPlot->xAxis->pixelToCoord(event->pos().x());
	double      yCoord = mParentPlot->yAxis->pixelToCoord(event->pos().y());
	QVariantMap tipData;
	tipData.insert("x", xCoord);
	tipData.insert("y", yCoord);
	emit tipUpdate(tipData);
}
