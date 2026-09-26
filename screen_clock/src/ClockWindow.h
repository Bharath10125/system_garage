#pragma once

#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QPainter>
#include <QFont>
#include <vector>

// One flip-card digit panel
struct FlipDigit {
    QString current  = "0";
    QString previous = "0";
    double  t        = 1.0;   // 0=start of flip, 1=settled
};

class ClockWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ClockWindow(QWidget *parent = nullptr);
    ~ClockWindow() override = default;

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private slots:
    void onTick();

private:
    // helpers
    static void seedPanels(std::vector<FlipDigit> &panels, const QStringList &vals);
    static void updatePanels(std::vector<FlipDigit> &panels, const QStringList &vals);

    // drawing
    void drawFlipPanel(QPainter &p, const QRectF &rect,
                       const FlipDigit &d, const QFont &font,
                       double digitScale = 1.0) const;

    QTimer    *m_timer   = nullptr;
    QDateTime  m_now;

    // HH MM SS — 6 panels (2 each)
    std::vector<FlipDigit> m_timePanels;
    // day-of-week, DD, MMM, YYYY — 4 panels
    std::vector<FlipDigit> m_datePanels;
};
