#include "ui/PlotDialog.h"
#include "ui_PlotDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QDateTime>
#include "components/plot/serfPlot/SerfPlot.h"

PlotDialog::PlotDialog(const QString& commandName, QWidget* parent)
	: QDialog(parent)
	  , m_commandName(commandName)
	  , m_timeRange(1.0) // 默认最小1秒
	  , m_plot(nullptr)
	  , m_ui(std::make_unique<Ui::PlotDialog>())
	  , m_updateTimer(nullptr)
	  , m_testTimer(nullptr)
	  , m_testRunning(false)
	  , m_testTime(0.0)
	  , m_testCounter(0)
{
	m_ui->setupUi(this);
	setWindowTitle(QStringLiteral("实时数据绘图 - %1").arg(commandName));
	setMinimumSize(800, 600);

	// 设置指令名称
	m_ui->commandNameLabel->setText(m_commandName);

	setupPlot();

	// 连接信号槽
	connect(m_ui->timeRangeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
	        this, &PlotDialog::setTimeRange);
	// connect(m_ui->testButton, &QPushButton::clicked, this, &PlotDialog::on_testButton_clicked);
	connect(m_ui->resetZoomButton, &QPushButton::clicked, this, &PlotDialog::resetZoom);
	connect(m_ui->clearButton, &QPushButton::clicked, this, &PlotDialog::clearData);

	// 更新定时器（每50ms更新一次）
	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, &QTimer::timeout, this, &PlotDialog::updatePlot);
	m_updateTimer->start(50);

	// 测试数据生成定时器（每秒20点 = 每50ms产生1点）
	m_testTimer = new QTimer(this);
	connect(m_testTimer, &QTimer::timeout, this, &PlotDialog::on_testTimer_timeout);
}

void PlotDialog::on_testButton_clicked()
{
	m_testRunning = !m_testRunning;

	if (m_testRunning)
	{
		// 开始测试
		m_ui->testButton->setText(QStringLiteral("测试停止"));
		m_testTime    = 0.0;
		m_testCounter = 0;
		m_testTimer->start(50); // 每秒20点 = 每50ms产生1点
	}
	else
	{
		// 停止测试
		m_ui->testButton->setText(QStringLiteral("测试开始"));
		m_testTimer->stop();
	}
}

void PlotDialog::on_testTimer_timeout()
{
	if (!m_testRunning)
	{
		return;
	}

	// 根据通道数量生成测试数据
	double baseValue = 50.0;
	for (int channel = 0; channel < m_channelNames.size(); ++channel)
	{
		// 每个通道使用不同的周期和偏移
		double period    = 10.0 + channel * 5.0;
		double offset    = channel * 20.0;
		double amplitude = 30.0 - channel * 5.0;
		double value     = baseValue + offset + amplitude * std::sin(m_testTime * 2.0 * 3.1415926 / period);

		addDataPoint(m_testTime, value, channel);
	}

	m_testTime += 0.05; // 50ms
	m_testCounter++;
}

PlotDialog::~PlotDialog()
{
	if (m_testTimer && m_testRunning)
	{
		m_testTimer->stop();
	}
	if (m_updateTimer)
	{
		m_updateTimer->stop();
	}
}

void PlotDialog::setupPlot()
{
	m_plot = new SerfPlot(m_ui->plotContainer);
	m_ui->plotLayout->addWidget(m_plot);

	// 设置坐标轴标签
	QFont font("Arial", 10);
	m_plot->setAxisXLabel(QStringLiteral("时间 (秒)"), font, QColor(34, 34, 34), 10);
	m_plot->setAxisYLabel(QStringLiteral("数值"), font, QColor(34, 34, 34), 10);

	// 设置y轴范围
	m_plot->setDisplayTime(static_cast<int>(m_timeRange));

	// 定义曲线颜色
	QList<QColor> colors = {Qt::blue, Qt::red, Qt::green, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray, Qt::darkBlue};

	// 根据通道名称添加曲线
	for (int i = 0; i < m_channelNames.size(); ++i)
	{
		QColor color = colors[i % colors.size()];
		m_plot->addGraphChannel(m_channelNames[i], color);
	}

	// 启用y轴自适应
	m_plot->setYAxisAdaptive(true);

	// 设置边距 - 增加右边距以显示AxisTag
	m_plot->enableAutoMargins(true, QMargins(30, 1, 80, 1));
}

void PlotDialog::addDataPoint(double timestamp, double value, int channel)
{
	// 更新当前值显示（显示最后一个添加的通道的值）
	m_ui->currentValueLabel->setText(QStringLiteral("当前值: %1").arg(value, 0, 'f', 3));
	m_ui->currentTimeLabel->setText(QStringLiteral("更新时间: %1")
		.arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))));

	// 添加到SerfPlot
	m_plot->appendValue(channel, QPointF(timestamp, value));
}

void PlotDialog::setChannelNames(const QStringList& channelNames)
{
	if (channelNames.isEmpty())
		return;
	m_plot->clearAllGraphs();
	auto channelCount = channelNames.size();
	// 定义曲线颜色
	QList<QColor> colors = {Qt::blue, Qt::red, Qt::green, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray, Qt::darkBlue};
	for (qsizetype idx = 0; idx < channelCount; idx++)
	{
		m_plot->addGraphChannel(channelNames[idx], colors[idx]);
	}
	m_channelNames = channelNames;
}

int PlotDialog::channelCount() const
{
	return m_channelNames.size();
}

void PlotDialog::setTimeRange(double seconds)
{
	if (seconds < 1.0)
	{
		seconds = 1.0;
	}

	m_timeRange = seconds;
	m_ui->timeRangeSpinBox->blockSignals(true);
	m_ui->timeRangeSpinBox->setValue(m_timeRange);
	m_ui->timeRangeSpinBox->blockSignals(false);

	if (m_plot)
	{
		m_plot->setDisplayTime(static_cast<int>(m_timeRange));
	}
}

double PlotDialog::timeRange() const
{
	return m_timeRange;
}

void PlotDialog::updatePlot()
{
	if (!m_plot)
	{
		return;
	}

	m_plot->repaintPlot();
}

void PlotDialog::resetZoom()
{
	if (!m_plot)
	{
		return;
	}

	m_plot->resetOperatorState();
}

void PlotDialog::clearData()
{
	m_ui->currentValueLabel->setText(QStringLiteral("当前值: --"));
	m_ui->currentTimeLabel->setText(QStringLiteral("更新时间: --"));

	if (m_plot)
	{
		m_plot->clearAllGraphsData();
	}
}
