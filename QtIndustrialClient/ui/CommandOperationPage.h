/**
 * @file CommandOperationPage.h
 * @brief 命令操作页面类，用于参数的读取和设置
 * @author Claude Code
 * @date 2026-04-10
 */

#pragma once

#include <functional>
#include <memory>

#include <QMap>
#include <QVector>
#include <QWidget>
#include <QPointer>
#include "models/ParameterRecord.h"

class QTabWidget;
class QTableWidget;
class ComboBoxDelegate;
class QStyledItemDelegate;
class PlotDialog;

namespace Ui {
class CommandOperationPage;
}

/**
 * @class CommandOperationPage
 * @brief 命令操作页面类，提供参数读取和设置功能
 *
 * 负责：
 * - 显示参数表格（按协议类型和分组组织）
 * - 处理参数的读取和写入操作
 * - 解码接收到的参数值
 * - 更新表格显示
 */
class CommandOperationPage : public QWidget {
public:
    /**
     * @brief 构造一个新的命令操作页面
     * @param parent 父部件
     */
    explicit CommandOperationPage(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CommandOperationPage() override;

    /**
     * @brief 设置参数记录数据
     * @param records 参数记录向量
     */
    void setRecords(const QVector<ParameterRecord>& records);

    /**
     * @brief 设置操作处理函数
     * @param handler 接受ParameterRecord和ParameterAction的回调函数
     */
    void setActionHandler(std::function<void(const ParameterRecord&, ParameterAction)> handler);

private:
    /**
     * @struct ValueBinding
     * @brief 值绑定结构，关联参数记录和UI表格位置
     */
    struct ValueBinding {
        QTableWidget* table = nullptr;  ///< 关联的表格控件
        int row = -1;                   ///< 所在表格行
        ParameterRecord record;         ///< 参数记录
    };

    /**
     * @brief 判断是否是A3协议
     * @param protocolType 协议类型
     * @return true如果是A3协议
     */
    static bool isA3Protocol(quint8 protocolType);

public slots:
    /**
     * @brief 接收新数据并更新绘图
     * @param protocolType 协议类型
     * @param groupId 分组ID
     * @param commandId 命令ID
     * @param value 数据值
     */
    void onNewDataReceived(quint8 protocolType, int groupId, int commandId, double value);

    /**
     * @brief 生成命令键，用于映射参数记录
     * @param protocolType 协议类型 (0xA0 或 0xA3)
     * @param groupId 分组ID
     * @param commandId 命令ID
     * @return 32位无符号整数作为键
     */
    static quint32 makeCommandKey(quint8 protocolType, int groupId, int commandId);

    /**
     * @brief 创建参数表格
     * @param records 参数记录向量
     * @return 新创建的QTableWidget指针
     */
    QTableWidget* createTable(const QVector<ParameterRecord>& records);

    /**
     * @brief 创建操作按钮单元格
     * @param table 所属表格
     * @param row 所在行
     * @param record 参数记录
     * @return 包含操作按钮的QWidget指针
     */
    QWidget* createActionCell(QTableWidget* table, int row, const ParameterRecord& record);

    /**
     * @brief 触发参数操作
     * @param record 参数记录
     * @param action 操作类型（读取或写入）
     */
    void emitAction(const ParameterRecord& record, ParameterAction action);

    /**
     * @brief 解码参数值
     * @param record 参数记录
     * @param payload 负载数据
     * @param offset 偏移量
     * @return 解码后的QString值
     */
    static QString decodeValue(const ParameterRecord& record, const QByteArray& payload, int offset);

public:
    /**
     * @brief 更新参数值显示
     * @param protocolType 协议类型
     * @param groupId 分组ID
     * @param commandId 命令ID
     * @param payload 包含新值的负载数据
     */
    void updateValues(quint8 protocolType, int groupId, int commandId, const QByteArray& payload);

    /**
     * @brief 获取参数记录
     * @return 参数记录向量
     */
    QVector<ParameterRecord> records() const
    {
        return m_records;
    }

    QTabWidget* m_groupTabs = nullptr;  ///< 分组标签页控件
    QVector<ParameterRecord> m_records;  ///< 参数记录存储
    QMap<quint32, QVector<ValueBinding>> m_valueBindingMap;  ///< 值绑定映射
    std::function<void(const ParameterRecord&, ParameterAction)> m_actionHandler;  ///< 操作处理函数
    std::unique_ptr<Ui::CommandOperationPage> m_ui;  ///< UI指针
    ComboBoxDelegate* m_dataTypeDelegate = nullptr;  ///< 数据类型列的 delegate
    QStyledItemDelegate* m_valueEditDelegate = nullptr;  ///< 值列编辑 delegate

    // 绘图对话框管理
    QMap<quint32, QPointer<PlotDialog>> m_activePlotDialogs;  ///< 当前打开的绘图对话框（使用QPointer安全管理）
    QMap<quint32, double> m_startTimestamps;  ///< 每个命令的起始时间戳

    // A3数据更新频率限制
    QMap<quint32, QDateTime> m_lastUpdateTime;  ///< 每个key的最后更新时间
    static constexpr int MIN_UPDATE_INTERVAL_MS = 200;  ///< 最小更新间隔200ms

    /**
     * @brief 数据排列方式枚举
     */
    enum class DataLayout {
        Interleaved,  ///< 交错排列: [x0,y0,z0,x1,y1,z1...]
        Blocked       ///< 分块排列: [x0,x1...y0,y1...,z0,z1...]
    };

    /**
     * @brief 解析A3数组数据并发送到绘图对话框
     * @param key 命令键
     * @param bindings 值绑定列表
     * @param payload 数据负载
     * @param layout 数据排列方式
     */
    void parseAndPlotA3Data(quint32 key, const QVector<ValueBinding>& bindings, const QByteArray& payload, DataLayout layout);

    /**
     * @brief 获取同一命令的所有参数记录
     * @param key 命令键
     * @return 参数记录列表
     */
    QVector<ParameterRecord> getCommandRecords(quint32 key) const;

    QMap<quint32, DataLayout> m_dataLayouts;  ///< 每个命令的数据排列方式
};
