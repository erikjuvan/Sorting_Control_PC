#pragma once

#include <qwidget.h>
#include <qpushbutton.h>
#include "ChartView.hpp"
#include "Chart.hpp"

class Window : public QWidget {
	Q_OBJECT

public:
	Window(QWidget* parent = nullptr);
	~Window();

private:
	QPushButton* m_button_exit;
	Chart* m_chart;
	ChartView* m_chartView;		
};

