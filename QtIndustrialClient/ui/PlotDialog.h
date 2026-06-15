#pragma once

#include <QDialog>
#include <QDateTime>
#include <QTimer>
#include <QVector>
#include <memory>

class SerfPlot;
class QLabel;
class QDoubleSpinBox;
class QPushButton;

namespace Ui
{
	class PlotDialog;
}

/**
 * @class PlotDialog
 * @brief 实时数据绘图对话框
 *
 * 功能：
 * - 显示指令名称
 * - 使用SerfPlot绘制实时数据
 * - 支持设置显示时间范围（最小1秒）
 * - 右侧箭头指示器显示最新数据
 */
class PlotDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlotDialog(const QString& commandName, QWidget* parent = nullptr);
	~PlotDialog() override;

	/**
	 * @brief 添加新的数据点
	 * @param timestamp 时间戳（秒）
	 * @param value 数据值
	 * @param channel 通道索引
	 */
	void addDataPoint(double timestamp, double value, int channel = 0);

	/**
	 * @brief 设置显示的时间范围
	 * @param seconds 时间范围（秒）
	 */
	void setTimeRange(double seconds);

	/**
	 * @brief 获取当前时间范围
	 * @return 时间范围（秒）
	 */
	double timeRange() const;

	/**
	 * @brief 设置曲线通道配置
	 * @param channelNames 曲线名称列表（legend显示）
	 */
	void setChannelNames(const QStringList& channelNames);

	/**
	 * @brief 获取当前曲线通道数量
	 * @return 通道数量
	 */
	int channelCount() const;

private slots:
	/**
	 * @brief 更新绘图显示
	 */
	void updatePlot();

	/**
	 * @brief 重置缩放
	 */
	void resetZoom();

	/**
	 * @brief 清除所有数据
	 */
	void clearData();

	/**
	 * @brief 测试按钮点击事件（切换测试状态）
	 */
	void on_testButton_clicked();

	/**
	 * @brief 测试数据生成定时器槽函数
	 */
	void on_testTimer_timeout();

private:
	void setupPlot();

	QString                         m_commandName;
	double                          m_timeRange;    // 显示的时间范围（秒）
	QStringList                     m_channelNames; // 曲线名称列表
	SerfPlot*                       m_plot;         // 使用重构后的SerfPlot
	std::unique_ptr<Ui::PlotDialog> m_ui;           // UI指针
	QTimer*                         m_updateTimer;
	QTimer*                         m_testTimer;   // 测试数据生成定时器
	bool                            m_testRunning; // 测试运行状态
	double                          m_testTime;    // 测试已运行时间
	int                             m_testCounter; // 测试数据计数器
};
