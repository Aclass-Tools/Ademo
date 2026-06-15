#pragma once
#include "../qcustomplot/qcustomplot.h"
/**
 * @brief QCustomPlot中的一个追踪器
 *
 * ItemTracer继承自QCPAbstractItem和ItemMouseEventInterface。
 * 它可以追踪并显示鼠标指针所在位置的数据点的信息。
 */
class ItemTracer : public QCPAbstractItem {
    Q_OBJECT

public:
    /**
     * @brief 存储追踪器上的击中点信息
     */
    struct HitPoint {
        QString lineName;  ///< 击中点所在的曲线名称
        QString lineType;  ///< 击中点所在的曲线名称
        double xCoord;     ///< 击中点的x坐标
        double yCoodValue; ///< 击中点的y坐标
        int nXIndex;
        int reverseIndex;
        QCPAxis* xAxis = nullptr; ///< 击中点所在的x轴
        QCPAxis* yAxis = nullptr; ///< 击中点所在的y轴
    };

    /**
     * @brief 构造函数
     * @param parent QCustomPlot的指针，ItemTracer的父对象
     */
    ItemTracer(QCustomPlot* parent);

    /**
     * @brief 析构函数
     */
    ~ItemTracer() override;

    /**
     * @brief 设置追踪器所在的层的名称
     * @param name 层名称
     */
    void setLayerName(const QString& name);

    /**
     * @brief 设置追踪器的类型
     * @param type 追踪器的类型，0表示显示鼠标所在列的所有纵坐标，1表示显示鼠标点的坐标，2表示获取颜色图的z值
     */
    void setType(int type = 0);
    void setShowPopText(bool b);

    /**
     * @brief 显示小数位数
     * @param decimal 
     */
    void setDecimal(int decimal = 6);

    /**
     * @brief 更新鼠标位置命中的位置信息
     */
    void updateTracer();

    void replot();

public:
    /**
     * @brief 处理鼠标移动事件
     * @param event 鼠标事件指针
     */
    void handleMouseMoveEvent(QMouseEvent* event);

    /**
     * @brief 处理鼠标按下事件
     * @param event 鼠标事件指针
     */
    void handleMousePressEvent(QMouseEvent* event);

    /**
     * @brief 处理鼠标释放事件
     * @param event 鼠标事件指针
     */
    void handleMouseReleaseEvent(QMouseEvent* event);
    double selectTest(const QPointF& pos, bool onlySelectable, QVariant* details = nullptr) const;
    ;
    /**
     * @brief 测试指定点是否可选中
     * @param pos 待测试点的坐标
     * @param onlySelectable 是否只测试可选中的点
     * @param details 存储测试结果的QVariant指针
     * @return 如果指定点可选中，则返回其x坐标；否则返回0
     */

signals:
    /**
     * @brief 当追踪器的信息更新时发送此信号
     * @param data 更新后的信息
     */
    void tipUpdate(QVariantMap data);

private:
    /**
     * @brief 绘制追踪器
     * @param painter 用于绘制的QCPPainter指针
     */
    void draw(QCPPainter* painter);

    /**
     * @brief 查找指定曲线中x坐标离xValue最近的点的索引
     * @param graph 待查找的曲线
     * @param xValue 目标x坐标
     * @param yValue 目标y坐标
     * @param isDecreasing 是否按照x坐标递减的顺序查找
     * @return 返回离目标x坐标最近的点的索引
     */
    int findXCoord(QCPGraph* graph, double xValue, double yValue, bool isDecreasing);

    /**
     * @brief 处理追踪器类型为1的情况
     * @param event 鼠标事件指针
     */
    void handleType_1(QMouseEvent* event);

private:
    double m_xCoodValue;              ///< 追踪器所追踪的x坐标
    QVector<HitPoint> m_hitPointList; ///< 追踪器上的击中点信息列表
    QPoint m_center;                  ///< 追踪器的中心坐标
    QString m_layerName;              ///< 追踪器所在的层的名称
    Qt::Alignment m_textAlignment;    ///< 追踪器文本的对齐方式
    QFont m_font;                     ///< 追踪器文本的字体
    QColor m_textColor{"#888888"};    ///< 追踪器文本的颜色
    QColor m_lineColor;               ///< 追踪器的边框颜色
    QPen m_borderPen;                 ///< 追踪器边框的画笔
    QBrush m_brush;                   ///< 追踪器的填充画刷
    QPointF m_radius{3, 3};           ///< 追踪器的圆角半径
    QMargins m_padding;               ///< 追踪器的内边距
    QPoint m_offset;                  ///< 鼠标偏移量
    Qt::SizeMode m_sizeMode;          ///< 追踪器文本的大小模式
    int m_type = 0;                   ///< 追踪器的类型
    bool m_showPopText{false};        ///< 数值显示
    std::atomic_int m_decimal;        ///< 小数位数
};