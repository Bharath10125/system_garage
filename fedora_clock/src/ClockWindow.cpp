#include "ClockWindow.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFontMetrics>
#include <QtMath>

static const QColor BG        {  0,   0,   0};
static const QColor PANEL_BG  { 18,  18,  18};
static const QColor PANEL_TOP { 24,  24,  24};
static const QColor DIGIT_CLR {210, 210, 210};
static const QColor HLINE     {  0,   0,   0};
static const QColor DATE_CLR  {160, 160, 160};

static double flipEase(double t)
{
    return 1.0 - (1.0 - t) * (1.0 - t);
}

void ClockWindow::seedPanels(std::vector<FlipDigit> &panels, const QStringList &vals)
{
    int n = vals.size();
    panels.resize(n);
    for (int i = 0; i < n; ++i) {
        panels[i].current  = vals[i];
        panels[i].previous = vals[i];
        panels[i].t        = 1.0;
    }
}

void ClockWindow::updatePanels(std::vector<FlipDigit> &panels, const QStringList &vals)
{
    int n = vals.size();
    if (static_cast<int>(panels.size()) != n) {
        panels.resize(n);
        for (int k = 0; k < n; ++k) {
            panels[k].current = panels[k].previous = "0";
            panels[k].t = 1.0;
        }
    }
    for (int i = 0; i < n; ++i) {
        if (vals[i] != panels[i].current) {
            panels[i].previous = panels[i].current;
            panels[i].current  = vals[i];
            panels[i].t        = 0.0;
        }
    }
}

ClockWindow::ClockWindow(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_now = QDateTime::currentDateTime();

    QString ts = m_now.toString("HHmmss");
    QStringList tv;
    for (QChar c : ts) tv << c;
    seedPanels(m_timePanels, tv);

    QStringList dv;
    dv << m_now.toString("ddd").toUpper()
       << m_now.toString("dd")
       << m_now.toString("MMM").toUpper()
       << m_now.toString("yyyy");
    seedPanels(m_datePanels, dv);

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &ClockWindow::onTick);
    m_timer->start();
}

void ClockWindow::onTick()
{
    m_now = QDateTime::currentDateTime();

    QString ts = m_now.toString("HHmmss");
    QStringList tv;
    for (QChar c : ts) tv << c;
    updatePanels(m_timePanels, tv);

    QStringList dv;
    dv << m_now.toString("ddd").toUpper()
       << m_now.toString("dd")
       << m_now.toString("MMM").toUpper()
       << m_now.toString("yyyy");
    updatePanels(m_datePanels, dv);

    const double step = 0.10;
    for (FlipDigit &d : m_timePanels) if (d.t < 1.0) d.t = qMin(1.0, d.t + step);
    for (FlipDigit &d : m_datePanels) if (d.t < 1.0) d.t = qMin(1.0, d.t + step);

    update();
}

void ClockWindow::drawFlipPanel(QPainter &p, const QRectF &rc,
                                 const FlipDigit &d, const QFont &font,
                                 double /*unused*/) const
{
    double PW  = rc.width();
    double PH  = rc.height();
    double cx  = rc.center().x();
    double cy  = rc.center().y();
    double rad = PW * 0.09;

    // Bottom half — always shows current digit (static)
    {
        QPainterPath bp;
        bp.addRoundedRect(rc, rad, rad);
        p.save();
        p.setClipRect(QRectF(rc.left(), cy, PW, PH / 2.0 + 1.0));
        p.fillPath(bp, PANEL_BG);
        p.setPen(DIGIT_CLR);
        p.setFont(font);
        p.drawText(rc, Qt::AlignCenter, d.current);
        p.restore();
    }

    // Top half — animated flip
    {
        double scaleY;
        QString text;
        QColor  bg;

        if (d.t >= 1.0) {
            scaleY = 1.0;
            text   = d.current;
            bg     = PANEL_TOP;
        } else if (d.t < 0.5) {
            double e = flipEase(d.t / 0.5);
            scaleY   = 1.0 - e;
            text     = d.previous;
            bg       = PANEL_BG;
        } else {
            double e = flipEase((d.t - 0.5) / 0.5);
            scaleY   = e;
            text     = d.current;
            bg       = PANEL_TOP;
        }

        p.save();
        p.setClipRect(QRectF(rc.left(), rc.top(), PW, PH / 2.0));
        QTransform xf;
        xf.translate(cx, cy);
        xf.scale(1.0, scaleY);
        xf.translate(-cx, -cy);
        p.setTransform(xf, true);
        QPainterPath tp;
        tp.addRoundedRect(rc, rad, rad);
        p.fillPath(tp, bg);
        p.setPen(DIGIT_CLR);
        p.setFont(font);
        p.drawText(rc, Qt::AlignCenter, text);
        p.restore();
    }

    // Centre split line
    p.save();
    p.setPen(QPen(HLINE, 2.5));
    p.drawLine(QPointF(rc.left() + rad, cy), QPointF(rc.right() - rad, cy));
    p.restore();

    // Subtle shadow below split line
    {
        QLinearGradient sh(0, cy, 0, cy + PH * 0.05);
        sh.setColorAt(0.0, QColor(0, 0, 0, 70));
        sh.setColorAt(1.0, Qt::transparent);
        p.fillRect(QRectF(rc.left(), cy, PW, PH * 0.05), sh);
    }
}

void ClockWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing,     true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    int W = width(), H = height();
    p.fillRect(rect(), BG);

    // --- Layout: derive all sizes from screen so nothing ever overflows ------
    //
    // Time row has 3 groups [H1 H2] [M1 M2] [S1 S2]
    //   innerGap  = gap between the 2 panels inside one group
    //   groupSep  = gap between groups (colon dots live here)
    //   totalTimeW = 6*panelW + 4*innerGap + 2*groupSep
    //
    // We solve for panelW so totalTimeW fits inside W * 0.90

    double margin   = W * 0.05;
    double usableW  = W - 2.0 * margin;
    double innerGap = qMax(6.0,  W * 0.005);
    double groupSep = qMax(36.0, W * 0.030);

    // panelW so 6 panels + gaps = usableW
    double panelW = (usableW - 4.0 * innerGap - 2.0 * groupSep) / 6.0;
    double panelH = panelW * 1.55;

    // Space for date row and hint line below time panels
    double datePH   = H * 0.085;
    double dateGapV = H * 0.038;
    double hintH    = H * 0.058;
    double totalCH  = panelH + dateGapV + datePH + hintH;

    // If height overflows, shrink everything
    if (totalCH > H * 0.93) {
        double scale = (H * 0.93) / totalCH;
        panelH   *= scale;
        panelW    = panelH / 1.55;
        innerGap *= scale;
        groupSep *= scale;
        datePH   *= scale;
        dateGapV *= scale;
    }

    double totalTimeW   = 6.0 * panelW + 4.0 * innerGap + 2.0 * groupSep;
    double totalCHFinal = panelH + dateGapV + datePH + hintH;

    // Centre entire block on screen
    double topY   = (H - totalCHFinal) / 2.0;
    double startX = (W - totalTimeW)   / 2.0;

    // Time digit font
    int timePx = qMax(8, int(panelH * 0.72));
    QFont timeFont("sans-serif", 1);
    timeFont.setWeight(QFont::Bold);
    timeFont.setPixelSize(timePx);

    // x origin of group g (0=HH, 1=MM, 2=SS)
    auto gx = [&](int g) -> double {
        return startX + g * (2.0 * panelW + innerGap + groupSep);
    };

    // Draw 6 flip panels
    for (int g = 0; g < 3; ++g) {
        for (int d = 0; d < 2; ++d) {
            int    idx = g * 2 + d;
            double px  = gx(g) + d * (panelW + innerGap);
            drawFlipPanel(p, QRectF(px, topY, panelW, panelH),
                          m_timePanels[idx], timeFont);
        }
    }

    // Colon dots centred in each groupSep
    p.setBrush(QColor(55, 55, 55));
    p.setPen(Qt::NoPen);
    double dotR  = qMax(4.0, groupSep * 0.09);
    double dotCY = topY + panelH * 0.50;
    for (int g = 0; g < 2; ++g) {
        double rx = gx(g) + 2.0 * panelW + innerGap + groupSep * 0.50;
        p.drawEllipse(QPointF(rx, dotCY - panelH * 0.13), dotR, dotR);
        p.drawEllipse(QPointF(rx, dotCY + panelH * 0.13), dotR, dotR);
    }

    // --- Date row: DAY | DD | MMM | YYYY ------------------------------------
    double dateY = topY + panelH + dateGapV;

    // Panel widths based on char count (DAY=3, DD=2, MMM=3, YYYY=4)
    double unit = datePH * 0.95;
    double dW[4] = { unit * 1.30,
                     unit * 0.88,
                     unit * 1.30,
                     unit * 1.88 };
    double dg = qMax(4.0, datePH * 0.12);
    double dateRowW = dW[0] + dW[1] + dW[2] + dW[3] + 3.0 * dg;
    double dateX    = (W - dateRowW) / 2.0;   // centred independently

    int dateFontPx = qMax(8, int(datePH * 0.54));
    QFont dateFont("sans-serif", 1);
    dateFont.setWeight(QFont::Bold);
    dateFont.setPixelSize(dateFontPx);

    double dx = dateX;
    for (int i = 0; i < 4; ++i) {
        double rad = dW[i] * 0.10;
        QRectF dr(dx, dateY, dW[i], datePH);
        double cy2 = dr.center().y();

        // Bottom half (current)
        {
            QPainterPath bp;
            bp.addRoundedRect(dr, rad, rad);
            p.save();
            p.setClipRect(QRectF(dr.left(), cy2, dW[i], datePH / 2.0 + 1.0));
            p.fillPath(bp, PANEL_BG);
            p.setPen(DATE_CLR);
            p.setFont(dateFont);
            p.drawText(dr, Qt::AlignCenter, m_datePanels[i].current);
            p.restore();
        }

        // Top half (animated)
        {
            const FlipDigit &fd = m_datePanels[i];
            double scaleY;
            QString text;
            if (fd.t >= 1.0) {
                scaleY = 1.0;  text = fd.current;
            } else if (fd.t < 0.5) {
                double e = flipEase(fd.t / 0.5);
                scaleY = 1.0 - e;  text = fd.previous;
            } else {
                double e = flipEase((fd.t - 0.5) / 0.5);
                scaleY = e;  text = fd.current;
            }
            p.save();
            p.setClipRect(QRectF(dr.left(), dr.top(), dW[i], datePH / 2.0));
            QTransform xf;
            xf.translate(dr.center().x(), cy2);
            xf.scale(1.0, scaleY);
            xf.translate(-dr.center().x(), -cy2);
            p.setTransform(xf, true);
            QPainterPath tp;
            tp.addRoundedRect(dr, rad, rad);
            p.fillPath(tp, PANEL_TOP);
            p.setPen(DATE_CLR);
            p.setFont(dateFont);
            p.drawText(dr, Qt::AlignCenter, text);
            p.restore();
        }

        // Split line
        p.setPen(QPen(HLINE, 2.0));
        p.drawLine(QPointF(dr.left() + rad, cy2),
                   QPointF(dr.right() - rad, cy2));

        dx += dW[i] + dg;
    }

    // Exit hint at bottom
    {
        int hintPx = qMax(8, int(H * 0.016));
        QFont hf("sans-serif", 1);
        hf.setPixelSize(hintPx);
        QFontMetrics hfm(hf);
        QString hint = "";
        double hx = (W - hfm.horizontalAdvance(hint)) / 2.0;
        double hy = topY + totalCHFinal - hintH * 0.18;
        p.setFont(hf);
        p.setPen(QColor(38, 38, 38));
        p.drawText(QPointF(hx, hy), hint);
    }
}

void ClockWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape || e->key() == Qt::Key_Q)
        QApplication::quit();
    QWidget::keyPressEvent(e);
}

void ClockWindow::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    QApplication::quit();
}
