#pragma once

#include <QWidget>
#include "ui_legendPlot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class legendPlotClass; };
QT_END_NAMESPACE

class legendPlot : public QWidget
{
	Q_OBJECT

public:
	legendPlot(QWidget* parent = nullptr);
	~legendPlot();
public:
	void showLegendData(const QString& name, const QList<float>& keys);
	void hideLegend();
private:
	Ui::legendPlotClass* ui;
};
