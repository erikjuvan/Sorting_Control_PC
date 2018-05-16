#include "qapplication.h"
#include "Window.hpp"

Window::Window(QWidget* parent) : QWidget(parent) {
	// Set size of the window
	setFixedSize(1620, 800);

	// Create and position the button
	m_button_exit = new QPushButton("Exit", this);
	m_button_exit->setGeometry(1510, 10, 100, 40);
	m_button_exit->setCheckable(true);

	m_chart = new Chart(3);	

	m_chartView = new ChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
	m_chartView->setGeometry(10, 10, 1300, 780);

	connect(m_button_exit, SIGNAL(clicked(bool)), qApp, SLOT(quit()));
}

Window::~Window() {
	delete m_button_exit;
	delete m_chart;
	delete m_chartView;		
}