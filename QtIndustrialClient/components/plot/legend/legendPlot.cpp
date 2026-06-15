#include "legendPlot.h"
#include <QTimer>
#include <QResizeEvent>
legendPlot::legendPlot(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::legendPlotClass())
{
	ui->setupUi(this);
}

legendPlot::~legendPlot()
{
	delete ui;
}

void legendPlot::showLegendData(const QString& name, const QList<float>& keys)
{
	if (keys.isEmpty()) {
		hide();
		return;
	}

	ui->graphRedWnd->hide();
	ui->graphBlueWnd->show();

	setFixedHeight(38);
	QString graphBlue = QString("%1-%2").arg(name).arg(QString::number(keys.at(0)));
	ui->blueLabel->setText(graphBlue);
	if (keys.size() == 2) {
		QString graphRed = QString("%1-%2").arg(name).arg(QString::number(keys.at(1)));
		ui->redLabel->setText(graphRed);
		ui->graphRedWnd->show();
		setFixedHeight(60);
	}

	QTimer::singleShot(20, [this]() {
		this->adjustSize();
		});
}

void legendPlot::hideLegend()
{
	ui->legendWnd->hide();
}
