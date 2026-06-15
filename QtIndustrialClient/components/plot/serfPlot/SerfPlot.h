#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "../qcustomplot/qcustomplot.h"
#include "../itemTracer/ItemTracer.h"
#include "../axistag/axistag.h"

class SerfPlot : public QWidget
{
	Q_OBJECT

public:
	enum class XAxisDisplayType :uint8_t
	{
		Time,
		Line
	};

	explicit SerfPlot(QWidget* parent = nullptr);
	/**
	 * @brief 增加曲线
	 * @param name
	 * @param color
	 */
	void      addGraphChannel(const QString& name, const QColor& color = Qt::blue, bool drawCircle = false);
	void      appendValue(int channel, QPointF point);
	void      appendValue(int channel, const QVector<double>& xData, const QVector<double>& yData);
	void      setDisplayPoints(int seconds);
	void      clearAllGraphs();
	void      clearAllGraphsData();
	void      startRefresh(int intervalMs = 1000);
	void      stopRefresh() const;
	bool      isRefresh() const;
	void      setChannelColor(const int& channel, const QColor& color);
	QCPGraph* getGraph(const int graphChannel) const;
	void      setAxisXLabel(QString strText, QFont font, QColor color, int Padding = 10);
	void      setAxisYLabel(QString strText, QFont font, QColor color, int Padding = 10);
	void      setXAxisAdaptive(bool enable);
	void      setYAxisAdaptive(bool enable);
	bool      getYAxisIsAdaptive() const;
	void      setDecimal(int decimal);
	void      setDisplayType(XAxisDisplayType type);
	void      setDisplayTime(int time);
	void      repaintPlot();
	void      resetOperatorState();
	void      enableAutoMargins(bool enable, const QMargins& margins = {30, 1, 1, 1});

	/**
	 * @brief 获取内部的QCustomPlot指针
	 * @return QCustomPlot指针
	 */
	QCustomPlot* customPlot() const { return m_customPlot; }

	/**
	 * @brief 获取指定通道的AxisTag
	 * @param channel 通道索引
	 * @return AxisTag指针
	 */
	AxisTag* axisTag(int channel) const;

#if 0
protected:
	void mouseMoveEvent(QMouseEvent* event) override;
#endif

private:
	void initialize();
	void initConnect();
	void updateScaleYAxis();
	void findVisibleMinMax(QCPGraph* graph, double& minY, double& maxY);
	void findXVisibleMinMax(QCPGraph* graph, double& minX, double& maxX);
	void removeOutDisplayData(const QCPGraph* graph) const;

private:
	QCustomPlot*         m_customPlot;       // 内部的QCustomPlot实例
	QVBoxLayout*         m_layout;           // 布局
	QMap<int, QCPGraph*> m_graphMap;         // 名称 -> 曲线指针
	QMap<int, AxisTag*>  m_axisTagMap;       // 通道 -> AxisTag指针
	int                  m_displayPoints;    // 显示的点数
	std::atomic_bool     m_bYAdaptive;       // y轴自适应
	std::atomic_bool     m_bXAdaptive;       // x轴自适应
	std::atomic_bool     m_bPress;           // 鼠标按下
	QTimer*              m_timer = nullptr;  // 定时器
	ItemTracer*          m_itemTracer;       // 鼠标悬浮数值显示
	XAxisDisplayType     m_xAxisDisplayType; // x轴显示类型
	std::atomic_int      m_xAxisDisplayArea; // x轴的时间显示区域
};
