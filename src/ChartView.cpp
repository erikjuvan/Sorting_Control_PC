#include "ChartView.hpp"

ChartView::ChartView(QtCharts::QChart* chart, QWidget* parent) : 
	QtCharts::QChartView(chart, parent), m_isTouching(false) {

}

bool ChartView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::TouchBegin) {
        // By default touch events are converted to mouse events. So
        // after this event we will get a mouse event also but we want
        // to handle touch events as gestures only. So we need this safeguard
        // to block mouse events that are actually generated from touch.
        m_isTouching = true;

        // Turn off animations when handling gestures they
        // will only slow us down.
        chart()->setAnimationOptions(QtCharts::QChart::NoAnimation);
    }
    return QChartView::viewportEvent(event);
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    if (m_isTouching)
        return;
    QChartView::mousePressEvent(event);
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isTouching)
        return;
    QChartView::mouseMoveEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isTouching)
        m_isTouching = false;

    // Because we disabled animations when touch event was detected
    // we must put them back on.
    chart()->setAnimationOptions(QtCharts::QChart::SeriesAnimations);

    QChartView::mouseReleaseEvent(event);
}

void ChartView::keyPressEvent(QKeyEvent *event)
{
	static int hScroll = 0, vScroll = 0;
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
		hScroll -= 10;
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
		hScroll += 10;
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
		vScroll += 10;
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
		vScroll -= 10;
        break;
	case Qt::Key_Space:
        chart()->zoomReset();
		chart()->scroll(-hScroll, -vScroll);
		hScroll = 0;
		vScroll = 0;
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}