#include "SerfPlot.h"
#include <QDateTime>
#include <QtMath>

SerfPlot::SerfPlot(QWidget* parent)
	: QWidget(parent)
	  , m_bXAdaptive(true)
	  , m_bYAdaptive(true)
	  , m_displayPoints(10 * 10000)
	  , m_xAxisDisplayType(XAxisDisplayType::Line)
	  , m_xAxisDisplayArea(20)
{
	m_layout = new QVBoxLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);

	m_customPlot = new QCustomPlot(this);
	m_layout->addWidget(m_customPlot);

	initialize();
	initConnect();
	setYAxisAdaptive(true);
}

void SerfPlot::initialize()
{
	m_customPlot->setBackground(Qt::white);
	m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

	QFont qfLable(QFont("Arial"));
	qfLable.setPixelSize(12);
	//设置基础属性
	m_customPlot->xAxis->setTickLengthOut(5);
	m_customPlot->xAxis->ticker()->setTickCount(5);
	m_customPlot->xAxis->setTickLabelFont(qfLable);
	m_customPlot->xAxis->setTickLabelColor(QColor(34, 34, 34));
	m_customPlot->xAxis->setSubTicks(false);
	m_customPlot->xAxis2->setVisible(true);
	m_customPlot->xAxis2->setTickLabels(false);
	m_customPlot->xAxis2->setTicks(false);

	m_customPlot->yAxis->setTickLengthOut(5);
	//yAxis->setSingleZoom(false);
	m_customPlot->yAxis->ticker()->setTickCount(5);
	m_customPlot->yAxis->setTickLabelFont(qfLable);
	m_customPlot->yAxis->setTickLabelColor(QColor(34, 34, 34));
	m_customPlot->yAxis->setSubTicks(false);
	m_customPlot->yAxis2->setVisible(true);
	m_customPlot->yAxis2->setTickLabels(false);
	m_customPlot->yAxis2->setTicks(false);

	m_customPlot->yAxis->setNumberFormat("g");
	m_customPlot->yAxis->setNumberPrecision(2);

	m_customPlot->axisRect()->setMargins(QMargins(0, 0, 0, 0));

	qint64 now = QDateTime::currentSecsSinceEpoch();
	m_customPlot->yAxis->setRange(0, 5);
	m_customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

	//添加鼠标悬浮事件
	m_itemTracer = new ItemTracer(m_customPlot);
	m_itemTracer->setShowPopText(true);
	//隐藏图例
	m_customPlot->legend->setVisible(false);
	//右键鼠标属性
	m_customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
}

void SerfPlot::addGraphChannel(const QString& name, const QColor& color, bool drawCircle)
{
	int indexKey = m_customPlot->graphCount();
	if (m_graphMap.contains(indexKey))
		return;

	QCPGraph* g = m_customPlot->addGraph();
	g->setPen(QPen(color, 1));
	g->setName(name);
	m_graphMap[indexKey] = g;
	if (drawCircle)
	{
		g->setScatterStyle(QCPScatterStyle(
			QCPScatterStyle::ssCircle, // 点形状，可选：ssDot, ssDisc, ssCross, ssStar 等
			color,                     // 点颜色
			4                          // 点大小（像素）
		));
	}

	// 为每个通道创建AxisTag
	AxisTag* tag = new AxisTag(m_customPlot->yAxis);
	tag->setPen(QPen(color));
	tag->setText(name);
	m_axisTagMap[indexKey] = tag;
}

void SerfPlot::appendValue(int channel, QPointF point)
{
	auto graph = getGraph(channel);
	if (!graph)
		return;
	graph->addData(point.x(), point.y());
	removeOutDisplayData(graph);

	// 更新AxisTag位置和值
	if (m_axisTagMap.contains(channel))
	{
		AxisTag* tag = m_axisTagMap[channel];
		tag->updatePosition(point.y());
		tag->setText(QString::number(point.y(), 'f', 3));
	}
	//repaintPlot();
}

void SerfPlot::appendValue(int channel, const QVector<double>& xData, const QVector<double>& yData)
{
	auto graph = getGraph(channel);
	if (!graph || xData.isEmpty())
		return;

	graph->addData(xData, yData);
	removeOutDisplayData(graph);
	//repaintPlot();
}

void SerfPlot::setDisplayPoints(int seconds)
{
	m_displayPoints = seconds;
}

void SerfPlot::clearAllGraphs()
{
	// 清理AxisTags
	for (auto tag : m_axisTagMap)
	{
		delete tag;
	}
	m_axisTagMap.clear();

	m_graphMap.clear();
	m_customPlot->clearGraphs();
	if (m_customPlot->legend->visible())
		m_customPlot->legend->setVisible(false);
	setYAxisAdaptive(true);
	repaintPlot();
}

void SerfPlot::clearAllGraphsData()
{
	for (auto g : m_graphMap)
		g->data()->clear();
	setYAxisAdaptive(true);
	repaintPlot();
}

void SerfPlot::startRefresh(int intervalMs)
{
	if (!m_timer)
	{
		m_timer = new QTimer(this);
		connect(m_timer, &QTimer::timeout, this, &SerfPlot::repaintPlot);
	}
	if (m_timer->isActive())
		return;
	m_timer->start(intervalMs);
	repaintPlot();
}

void SerfPlot::stopRefresh() const
{
	if (m_timer)
	{
		m_timer->stop();
	}
}

bool SerfPlot::isRefresh() const
{
	if (!m_timer)
		return false;
	return m_timer->isActive();
}

void SerfPlot::setChannelColor(const int& channel, const QColor& color)
{
	if (!m_graphMap.contains(channel)) return;
	auto graph = m_graphMap.value(channel);
	graph->setPen(QColor(color));
}

QCPGraph* SerfPlot::getGraph(const int graphChannel) const
{
	if (!m_graphMap.contains(graphChannel))
		return nullptr;
	return m_graphMap.value(graphChannel);
}

void SerfPlot::setAxisXLabel(QString strText, QFont font, QColor color, int Padding)
{
	m_customPlot->xAxis->setLabel(strText);
	m_customPlot->xAxis->setLabelFont(font);
	m_customPlot->xAxis->setLabelColor(color);
	m_customPlot->xAxis->setLabelPadding(Padding);
}

void SerfPlot::setAxisYLabel(QString strText, QFont font, QColor color, int Padding)
{
	m_customPlot->yAxis->setLabel(strText);
	m_customPlot->yAxis->setLabelFont(font);
	m_customPlot->yAxis->setLabelColor(color);
	m_customPlot->xAxis->setLabelPadding(Padding);
}

void SerfPlot::setXAxisAdaptive(bool enable)
{
	m_bXAdaptive = enable;
	if (m_bXAdaptive)
		repaintPlot();
}

void SerfPlot::setYAxisAdaptive(bool enable)
{
	m_bYAdaptive = enable;
	if (m_bYAdaptive)
		repaintPlot();
}

bool SerfPlot::getYAxisIsAdaptive() const
{
	return m_bYAdaptive;
}

void SerfPlot::setDecimal(int decimal)
{
	if (m_itemTracer)
		m_itemTracer->setDecimal(decimal);
}

void SerfPlot::setDisplayType(XAxisDisplayType type)
{
	m_xAxisDisplayType = type;
}

void SerfPlot::setDisplayTime(int time)
{
	m_xAxisDisplayArea = time;
	repaintPlot();
}

#if 0
void SerfPlot::mouseMoveEvent(QMouseEvent* event)
{
	QCustomPlot::mouseMoveEvent(event); // 保留默认行为

	// 将鼠标像素坐标映射为图表坐标
	double x = xAxis->pixelToCoord(event->pos().x());
	double y = yAxis->pixelToCoord(event->pos().y());

	QString tooltipText;
	bool    found = false;

	for (auto it = m_graphMap.begin(); it != m_graphMap.end(); ++it)
	{
		const QString& name  = it.key();
		QCPGraph*      graph = it.value();

		// 遍历 graph 数据，找距离 x 最近的点
		const QVector<double>& xs = m_dataMap[name].x;
		const QVector<double>& ys = m_dataMap[name].y;

		for (int i = 0; i < xs.size(); ++i)
		{
			if (qAbs(xs[i] - x) < 0.5) // 距离阈值（单位秒）
			{
				QDateTime time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(xs[i]));
				tooltipText    += QString("%1 val:%3")
				                  .arg(name)
				                  .arg(ys[i], 0, 'f', 2);
				found = true;
				if (it != m_graphMap.end() - 1) tooltipText.append('\n');
				break;
			}
		}
	}
}
#endif

void SerfPlot::repaintPlot()
{
	if (!m_customPlot->isVisible())
		return;
	if (m_bXAdaptive && !m_graphMap.isEmpty())
	{
		auto minL = std::numeric_limits<double>::max();
		auto maxR = std::numeric_limits<double>::lowest();

		bool bFindMinMax = false;
		for (auto graph : m_graphMap)
		{
			if (!graph->visible())continue;
			if (graph->data()->isEmpty())continue;
			minL        = qMin(minL, (graph->data()->constBegin())->key);
			maxR        = qMax(maxR, (graph->data()->constEnd() - 1)->key);
			bFindMinMax = true;
		}
		if (bFindMinMax)
		{
			m_customPlot->xAxis->setRange(minL, maxR);
		}
	}

	if (getYAxisIsAdaptive())
		updateScaleYAxis();

	m_itemTracer->updateTracer();
	m_customPlot->replot(QCustomPlot::rpQueuedReplot); // 刷新图像，已在 appendValue 设置了数据
}

void SerfPlot::resetOperatorState()
{
	setXAxisAdaptive(true);
	setYAxisAdaptive(true);
}

void SerfPlot::enableAutoMargins(bool enable, const QMargins& margins)
{
	//边距设置
	if (!enable)
		m_customPlot->axisRect()->setAutoMargins(QCP::msNone);
	else
		m_customPlot->axisRect()->setAutoMargins(QCP::msLeft | QCP::msTop | QCP::msBottom); // 不启用右边距自动调整，为AxisTag留出空间
	m_customPlot->axisRect()->setMargins(margins);
}

void SerfPlot::updateScaleYAxis()
{
	auto minY    = std::numeric_limits<double>::max();
	auto maxY    = std::numeric_limits<double>::lowest();
	bool hasData = false;
	for (auto graph : m_graphMap)
	{
		if (!graph->visible() || graph->data()->isEmpty())
			continue;

		double min = 0, max = 0;
		findVisibleMinMax(graph, min, max);
		minY    = qMin(minY, min);
		maxY    = qMax(maxY, max);
		hasData = true;
	}
	if (!hasData)
		return;

	double diff = maxY - minY;
	double yTip = 0.0;
	if (qFuzzyIsNull(diff))
		yTip = 5.0;
	else
		yTip = diff / 5;

	QCPAxis::ScaleType type = m_customPlot->yAxis->scaleType();
	if (type == QCPAxis::stLinear)
		m_customPlot->yAxis->setRange(minY - yTip, maxY + yTip);
	else if (type == QCPAxis::stLogarithmic)
		m_customPlot->yAxis->setRange(minY * 0.95, maxY * 1.05);
}

void SerfPlot::findVisibleMinMax(QCPGraph* graph, double& minValue, double& maxValue)
{
	QCPRange xRange = graph->keyAxis()->range();
	auto     data   = graph->data();

	auto begin = data->findBegin(xRange.lower);
	auto end   = data->findEnd(xRange.upper);

	minValue = std::numeric_limits<double>::max();
	maxValue = std::numeric_limits<double>::lowest();

	for (auto it = begin; it != end; ++it)
	{
		minValue = qMin(minValue, it->value);
		maxValue = qMax(maxValue, it->value);
	}
}

void SerfPlot::findXVisibleMinMax(QCPGraph* graph, double& minX, double& maxX)
{
	if (!graph || graph->dataCount() == 0)
		return;
	auto container = graph->data();
	if (container->isEmpty())
		return;

	bool     foundRange = false;
	QCPRange xRange     = graph->data()->keyRange(foundRange);
	minX                = container->constBegin()->key;
	maxX                = (container->constEnd() - 1)->key;
}

void SerfPlot::initConnect()
{
	connect(m_customPlot, &QCustomPlot::mousePress, this, [this](QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton &&
			m_customPlot->interactions().testFlag(QCP::iRangeDrag)
			&& m_customPlot->axisRect()->rect().contains(event->pos()))
		{
			m_bPress = true;
		}
	});

	connect(m_customPlot, &QCustomPlot::mouseRelease, this, [this](QMouseEvent* event)
	{
		m_bPress = false;
		if (event->button() == Qt::MouseButton::RightButton)
		{
			setXAxisAdaptive(true);
			setYAxisAdaptive(true);
			repaintPlot();
		}
	});

	connect(m_customPlot, &QCustomPlot::mouseMove, this, [this](QMouseEvent* event)
	{
		if (getYAxisIsAdaptive() && m_bPress)
		{
			setXAxisAdaptive(false);
			setYAxisAdaptive(false);
			repaintPlot();
		}
	});

	connect(m_customPlot, &QCustomPlot::mouseWheel, this, [this](QWheelEvent* event)
	{
		setXAxisAdaptive(false);
		setYAxisAdaptive(false);
		repaintPlot();
	});

	connect(m_customPlot, &QCustomPlot::mouseDoubleClick, this, [this](QMouseEvent* event)
	{
		if (!getYAxisIsAdaptive())
		{
			setXAxisAdaptive(true);
			setYAxisAdaptive(true);
			repaintPlot();
		}
	});
}

void SerfPlot::removeOutDisplayData(const QCPGraph* graph) const
{
	auto data = graph->data();
	if (data->isEmpty())
		return;

	if (m_xAxisDisplayType == XAxisDisplayType::Time)
	{
		double right = (data->constEnd() - 1)->key;
		double left  = right - m_xAxisDisplayArea;
		data->removeBefore(left);
	}
	else
	{
		if (data->size() > m_displayPoints)
		{
			auto it = data->constEnd() - m_displayPoints;
			data->removeBefore(it->key);
		}
	}
}

AxisTag* SerfPlot::axisTag(int channel) const
{
	if (m_axisTagMap.contains(channel))
	{
		return m_axisTagMap[channel];
	}
	return nullptr;
}
