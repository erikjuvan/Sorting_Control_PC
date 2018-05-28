#pragma once

#include <qchart.h>
#include <qchartview.h>

class ChartView : public QtCharts::QChartView {

public:
	ChartView(QtCharts::QChart* chart, QWidget* parent = nullptr);

protected:
	bool viewportEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);


private:
	bool m_isTouching;

};