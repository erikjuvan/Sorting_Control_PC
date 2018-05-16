#include "Chart.hpp"
#include <qmessagebox.h>

Chart::Chart(int lines, QGraphicsItem *parent, Qt::WindowFlags wFlags) :
	QChart(QChart::ChartTypeCartesian, parent, wFlags), m_lines(lines) {

	for (int i = 0; i < m_lines; ++i) {
		m_series.push_back(new QtCharts::QLineSeries());
		std::string name("Series " + std::to_string(1 + i));
		m_series[i]->setName(name.c_str());
		for (int j = 0; j < 7000; ++j)
			m_series[i]->append(j, 0);		
	}	

	for (auto& s : m_series) {
		addSeries(s);
	}				
	createDefaultAxes();

	m_axisX = new QtCharts::QValueAxis();
	m_axisX->setTickCount(8);

	for (auto& s : m_series) {
		setAxisX(m_axisX, s);
	}	
	axisX()->setRange(-100, 7100);
	axisY()->setRange(-20, 1020);	

	m_timer = new QTimer(this);
	m_timer->start(15);

	connect(m_timer, SIGNAL(timeout()), this, SLOT(Update()));
}

Chart::~Chart()
{
	delete m_timer;
	delete m_axisX;
	for (auto& s: m_series) delete s;
}

void Chart::Update() {
	static int x, y;
	int i = 0;

	for (auto& s : m_series) {				
		s->replace(x, QPoint(x, 50 * i + y));		
		i++;
	}
	x++;
	y++;
	if (x >= 7000) {
		x = 0;		
	}	
}
