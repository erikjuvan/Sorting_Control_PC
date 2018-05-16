#pragma once

#include <qchart.h>
#include <qchartview.h>

class ChartView : public QtCharts::QChartView {

public:
	ChartView(QtCharts::QChart* chart, QWidget* parent);

};