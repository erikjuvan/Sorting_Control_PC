#pragma once

#include <qchart.h>
#include <qlineseries.h>
#include <qvalueaxis.h>
#include <qtimer.h>
#include <vector>

class Chart : public QtCharts::QChart {
	Q_OBJECT

public:
	Chart(int lines, QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = 0);
    virtual ~Chart();

private slots:
	void Update();

private:
	int m_lines;
	std::vector<QtCharts::QXYSeries*> m_series;		
	QTimer* m_timer;
	QtCharts::QValueAxis* m_axisX;
};

