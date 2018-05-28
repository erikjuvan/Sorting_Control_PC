#include "Chart.hpp"
#include <qmessagebox.h>

#include "Helpers.hpp"

#define N 7000

Chart::Chart(int lines, QGraphicsItem *parent, Qt::WindowFlags wFlags) :
	QChart(QChart::ChartTypeCartesian, parent, wFlags), m_lines(lines) {

	for (int i = 0; i < m_lines; ++i) {
		m_series.push_back(new QtCharts::QLineSeries());
		std::string name("Series " + std::to_string(1 + i));
		m_series[i]->setName(name.c_str());
		for (int j = 0; j < N; ++j)
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
	axisX()->setRange(0, N);
	axisY()->setRange(0, 1000);	

	m_timer = new QTimer(this);
	m_timer->setTimerType(Qt::PreciseTimer);
	m_timer->start(100);	

	connect(m_timer, SIGNAL(timeout()), this, SLOT(Update()));
}

Chart::~Chart()
{
	delete m_timer;
	delete m_axisX;
	for (auto& s: m_series) delete s;
}

void Chart::Update() {

	static int x = 0, y = 0;
	int i = 0;	

	for (auto& s : m_series) {
		TIME_IT(s->replace(x, QPoint(x, 50 * i + y)););
		i++;
	}

	x++;
	y++;
	if (x >= N) {
		x = 0;		
	}	
}
