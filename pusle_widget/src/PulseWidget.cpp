#include "PulseWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QScreen>
#include <QApplication>
#include <QScrollBar>
#include <QSizePolicy>
#include <QFont>
#include <QPen>
#include <QColor>
#include <QFile>
#include <QDir>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>
#include <cstdio>

// ════════════════════════════════════════════════════════════════════════════
//  Palette: black glass, everything white
// ════════════════════════════════════════════════════════════════════════════
namespace P {
    inline QColor w(int a) { return QColor(255,255,255,a); }
    inline QColor text()   { return w(240); }
    inline QColor dim()    { return w(110); }
    inline QColor faint()  { return w(38);  }
    inline QColor track()  { return w(20);  }
}

static QFont monoFont(int px) {
    QFont f("Monospace"); f.setPixelSize(px); return f;
}
static QFont sansFont(int px, int weight = QFont::Normal) {
    QFont f("Inter,SF Pro Display,Helvetica Neue,Sans");
    f.setPixelSize(px); f.setWeight(QFont::Weight(weight));
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.3);
    return f;
}
static QFont capFont(int px) {
    QFont f = sansFont(px, QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.8);
    return f;
}

// ════════════════════════════════════════════════════════════════════════════
//  SparkLine
// ════════════════════════════════════════════════════════════════════════════
SparkLine::SparkLine(QWidget *parent) : QWidget(parent)
{
    setFixedSize(44, 18);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}
void SparkLine::addValue(double v)
{
    m_data.append(std::clamp(v, 0.0, 1.0));
    if (m_data.size() > kMaxPoints) m_data.removeFirst();
    QWidget::update();
}
void SparkLine::paintEvent(QPaintEvent *)
{
    if (m_data.size() < 2) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    double step = double(w) / (kMaxPoints - 1);

    QPainterPath fill;
    fill.moveTo(0, h);
    for (int i = 0; i < m_data.size(); ++i) {
        double x = (i-(kMaxPoints-m_data.size()))*step;
        double y = h - m_data[i]*(h-2) - 1;
        fill.lineTo(x, y);
    }
    fill.lineTo((m_data.size()-1-(kMaxPoints-m_data.size()))*step, h);
    fill.closeSubpath();
    p.fillPath(fill, P::w(18));

    QPainterPath line;
    for (int i = 0; i < m_data.size(); ++i) {
        double x = std::max(0.0,(i-(kMaxPoints-m_data.size()))*step);
        double y = h - m_data[i]*(h-2) - 1;
        if (i==0) line.moveTo(x,y); else line.lineTo(x,y);
    }
    p.setPen(QPen(P::w(185), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(line);
}

// ════════════════════════════════════════════════════════════════════════════
//  RingGauge
// ════════════════════════════════════════════════════════════════════════════
RingGauge::RingGauge(int size, bool showLabel, QWidget *parent)
    : QWidget(parent), m_size(size), m_showLabel(showLabel)
{
    setFixedSize(size, size);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}
void RingGauge::setValue(double v)
{
    m_value = std::clamp(v, 0.0, 1.0);
    update();
}
void RingGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int   mg     = m_size <= 26 ? 2 : 4;
    float stroke = m_size <= 26 ? 2.0f : 3.0f;
    QRectF r(mg, mg, m_size-2*mg, m_size-2*mg);

    // track
    p.setPen(QPen(P::track(), stroke));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(r);

    if (m_value > 0.001) {
        int span = int(-m_value * 360.0 * 16);
        // glow
        p.setPen(QPen(P::w(50), stroke+3.0f, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(r, 90*16, span);
        // bright arc
        p.setPen(QPen(P::w(232), stroke, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(r, 90*16, span);
        // tip dot
        double endRad = (90.0 - m_value*360.0) * M_PI / 180.0;
        QPointF tip(r.center().x() + r.width()/2.0*std::cos(endRad),
                    r.center().y() - r.height()/2.0*std::sin(endRad));
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(tip, stroke*0.9f, stroke*0.9f);
    }

    if (m_showLabel) {
        p.setFont(monoFont(m_size<=26 ? 7 : m_size<=36 ? 9 : 11));
        p.setPen(P::w(m_value>0.001 ? 225 : 75));
        p.drawText(QRectF(mg,mg,m_size-2*mg,m_size-2*mg),
                   Qt::AlignCenter, QString::number(int(m_value*100)));
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RingRow
// ════════════════════════════════════════════════════════════════════════════
RingRow::RingRow(const QString &label, QWidget *parent) : QWidget(parent)
{
    setFixedHeight(52);
    auto *hl = new QHBoxLayout(this);
    hl->setContentsMargins(16, 5, 16, 5);
    hl->setSpacing(12);

    m_ring = new RingGauge(42, true, this);

    m_nameLabel = new QLabel(label.toUpper(), this);
    m_nameLabel->setFont(capFont(10));
    m_nameLabel->setStyleSheet("color:rgba(255,255,255,0.50);");
    m_nameLabel->setFixedWidth(38);
    m_nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_valueLabel = new QLabel("—", this);
    m_valueLabel->setFont(monoFont(12));
    m_valueLabel->setStyleSheet("color:rgba(255,255,255,0.88);");
    m_valueLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    m_valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_valueLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_spark = new SparkLine(this);

    hl->addWidget(m_ring);
    hl->addWidget(m_nameLabel);
    hl->addWidget(m_valueLabel);
    hl->addWidget(m_spark);
}
void RingRow::update(double fraction, const QString &valueText, const QString &extraText)
{
    m_ring->setValue(fraction);
    m_valueLabel->setText(extraText.isEmpty() ? valueText : valueText+"  "+extraText);
    m_spark->addValue(fraction);
    QWidget::update();
}

// ════════════════════════════════════════════════════════════════════════════
//  SettingsPanel
// ════════════════════════════════════════════════════════════════════════════
SettingsPanel::SettingsPanel(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(200);
    setStyleSheet("background:transparent;");

    auto *vl = new QVBoxLayout(this);
    vl->setContentsMargins(18,18,18,18);
    vl->setSpacing(10);

    auto *tr = new QHBoxLayout;
    auto *title = new QLabel("SETTINGS", this);
    title->setFont(capFont(11));
    title->setStyleSheet("color:rgba(255,255,255,0.50);");
    auto *closeBtn = new QPushButton("x", this);
    closeBtn->setFixedSize(26,26);
    closeBtn->setFont(sansFont(13));
    closeBtn->setStyleSheet(R"(
        QPushButton{background:rgba(255,255,255,0.08);border:none;border-radius:13px;
            color:rgba(255,255,255,0.55);}
        QPushButton:hover{background:rgba(200,50,50,0.70);color:white;}
    )");
    tr->addWidget(title); tr->addStretch(); tr->addWidget(closeBtn);
    vl->addLayout(tr);
    vl->addWidget(makeSeparator());

    auto *opLbl = new QLabel("OPACITY", this);
    opLbl->setFont(capFont(10));
    opLbl->setStyleSheet("color:rgba(255,255,255,0.38);");
    vl->addWidget(opLbl);

    auto *opRow = new QHBoxLayout;
    auto *opSlider = new QSlider(Qt::Horizontal, this);
    opSlider->setRange(30,100); opSlider->setValue(88);
    opSlider->setStyleSheet(R"(
        QSlider::groove:horizontal{height:3px;background:rgba(255,255,255,0.12);border-radius:2px;}
        QSlider::handle:horizontal{background:white;width:14px;height:14px;margin:-6px 0;border-radius:7px;}
        QSlider::sub-page:horizontal{background:rgba(255,255,255,0.60);border-radius:2px;}
    )");
    auto *opVal = new QLabel("88%", this);
    opVal->setFixedWidth(32); opVal->setFont(monoFont(11));
    opVal->setAlignment(Qt::AlignRight);
    opVal->setStyleSheet("color:rgba(255,255,255,0.65);");
    opRow->addWidget(opSlider); opRow->addWidget(opVal);
    vl->addLayout(opRow);
    vl->addWidget(makeSeparator());

    auto *aoLbl = new QLabel("ALWAYS ON TOP", this);
    aoLbl->setFont(capFont(10));
    aoLbl->setStyleSheet("color:rgba(255,255,255,0.38);");
    vl->addWidget(aoLbl);

    auto *tog = new QPushButton(this);
    tog->setCheckable(true); tog->setChecked(true);
    tog->setFixedSize(38,20);
    auto ss=[](bool on){
        return QString("QPushButton{border:none;border-radius:10px;background:%1;}")
            .arg(on?"rgba(255,255,255,0.65)":"rgba(255,255,255,0.14)");
    };
    tog->setStyleSheet(ss(true));
    connect(tog, &QPushButton::toggled, [tog,ss,this](bool v){
        tog->setStyleSheet(ss(v)); emit alwaysOnTopChanged(v);
    });
    auto *aoRow = new QHBoxLayout;
    auto *aoText = new QLabel("Enabled",this);
    aoText->setFont(sansFont(13)); aoText->setStyleSheet("color:rgba(255,255,255,0.70);");
    aoRow->addWidget(aoText); aoRow->addStretch(); aoRow->addWidget(tog);
    vl->addLayout(aoRow);

    vl->addStretch();

    connect(opSlider, &QSlider::valueChanged, this, [this,opVal](int v){
        opVal->setText(QString::number(v)+"%"); emit opacityChanged(v);
    });
    connect(closeBtn, &QPushButton::clicked, this, &SettingsPanel::closeRequested);
}
QFrame *SettingsPanel::makeSeparator()
{
    auto *f = new QFrame(this);
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet("color:rgba(255,255,255,0.08);");
    return f;
}

// ════════════════════════════════════════════════════════════════════════════
//  BatteryIcon — custom-painted white battery shape
// ════════════════════════════════════════════════════════════════════════════
BatteryIcon::BatteryIcon(QWidget *parent) : QWidget(parent)
{
    setFixedSize(28, 14);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}
void BatteryIcon::setLevel(int pct, bool charging)
{
    m_pct = std::clamp(pct, 0, 100);
    m_charging = charging;
    update();
}
void BatteryIcon::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Body rect (leave 4px on right for the terminal nub)
    QRectF body(0.5, 1.5, 23.0, 11.0);
    // Terminal nub
    QRectF nub(23.5, 4.5, 3.5, 5.0);

    const QColor white(255, 255, 255, 210);
    const QColor dimW(255, 255, 255, 120);

    // Outline
    p.setPen(QPen(white, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(body, 2.5, 2.5);

    // Nub (filled solid)
    p.setPen(Qt::NoPen);
    p.setBrush(white);
    p.drawRoundedRect(nub, 1.2, 1.2);

    // Fill level
    float innerW = float(body.width())  - 4.f;
    float innerH = float(body.height()) - 4.f;
    float fillW  = innerW * float(m_pct) / 100.f;
    if (fillW > 0.5f) {
        QRectF fill(body.x()+2.f, body.y()+2.f, fillW, innerH);
        p.setBrush(QColor(255, 255, 255, m_pct < 20 ? 180 : 240));
        p.drawRoundedRect(fill, 1.0, 1.0);
    }

    // Charging indicator — small white bolt (two line segments)
    if (m_charging) {
        p.setPen(QPen(QColor(0, 0, 0, 200), 1.0, Qt::SolidLine, Qt::RoundCap));
        float cx = body.x() + body.width()/2.f;
        float cy = body.y() + body.height()/2.f;
        // Simple lightning: top-right → center-left → bottom-right
        p.drawLine(QPointF(cx+1.5, cy-3.5), QPointF(cx-2.0, cy+0.5));
        p.drawLine(QPointF(cx-2.0, cy+0.5), QPointF(cx+1.5, cy+3.5));
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  PulseLogoWidget
// ════════════════════════════════════════════════════════════════════════════
PulseLogoWidget::PulseLogoWidget(QWidget *parent) : QWidget(parent)
{
    setFixedSize(44, 44);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    m_anim = new QPropertyAnimation(this, "beatScale", this);
    m_anim->setDuration(300);
    m_anim->setStartValue(1.3f);
    m_anim->setEndValue(1.0f);
    m_anim->setEasingCurve(QEasingCurve::OutQuad);
}
void PulseLogoWidget::triggerBeat()
{
    if (m_anim->state() != QAbstractAnimation::Running) {
        m_anim->start();
    }
}
void PulseLogoWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.translate(width()/2.f, height()/2.f);
    p.scale(m_beatScale, m_beatScale);
    p.translate(-width()/2.f, -height()/2.f);

    QPainterPath path;
    path.moveTo(8,  22);
    path.lineTo(15, 22);
    path.lineTo(19, 12);
    path.lineTo(25, 32);
    path.lineTo(29, 22);
    path.lineTo(36, 22);

    p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);
}

// ════════════════════════════════════════════════════════════════════════════
//  CollapsedBar
// ════════════════════════════════════════════════════════════════════════════
CollapsedBar::CollapsedBar(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedHeight(44);

    auto *hl = new QHBoxLayout(this);
    hl->setContentsMargins(16, 0, 16, 0);
    hl->setSpacing(0);
    hl->setAlignment(Qt::AlignVCenter);   // keep everything on the vertical midline

    m_timeLabel = new QLabel("00:00  01 Jan 2000", this);
    m_timeLabel->setFont(sansFont(13, QFont::Medium));
    m_timeLabel->setStyleSheet("color:white;");
    m_timeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_timeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    hl->addWidget(m_timeLabel);

    auto div = [&](){
        auto *d = new QFrame(this);
        d->setFrameShape(QFrame::VLine);
        d->setFixedSize(1, 18);
        d->setStyleSheet("background:rgba(255,255,255,0.14);border:none;");
        d->setAttribute(Qt::WA_TransparentForMouseEvents);
        return d;
    };

    hl->addSpacing(14); hl->addWidget(div()); hl->addSpacing(14);

    m_cpuRing = new RingGauge(24, false, this);
    m_cpuLabel = new QLabel("CPU  0%", this);
    m_cpuLabel->setFont(sansFont(13, QFont::Medium));
    m_cpuLabel->setStyleSheet("color:white;");
    m_cpuLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_cpuLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    hl->addWidget(m_cpuRing); hl->addSpacing(7); hl->addWidget(m_cpuLabel);

    hl->addSpacing(14); hl->addWidget(div()); hl->addSpacing(14);

    m_ramRing = new RingGauge(24, false, this);
    m_ramLabel = new QLabel("RAM  0%", this);
    m_ramLabel->setFont(sansFont(13, QFont::Medium));
    m_ramLabel->setStyleSheet("color:white;");
    m_ramLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_ramLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    hl->addWidget(m_ramRing); hl->addSpacing(7); hl->addWidget(m_ramLabel);

    hl->addSpacing(14); hl->addWidget(div()); hl->addSpacing(14);

    m_netLabel = new QLabel("0.0 MB  ·  0 KB up", this);
    m_netLabel->setFont(sansFont(13, QFont::Medium));
    m_netLabel->setStyleSheet("color:white;");
    m_netLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_netLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    hl->addWidget(m_netLabel);

    hl->addSpacing(14); hl->addWidget(div()); hl->addSpacing(14);

    m_batIcon = new BatteryIcon(this);
    m_batPctLabel = new QLabel("--%", this);
    m_batPctLabel->setFont(sansFont(13, QFont::Medium));
    m_batPctLabel->setStyleSheet("color:white;");
    m_batPctLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_batPctLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    hl->addWidget(m_batIcon);
    hl->addSpacing(7);
    hl->addWidget(m_batPctLabel);

}
void CollapsedBar::setCpu(double pct){
    if(m_cpuRing)  m_cpuRing->setValue(pct/100.0);
    if(m_cpuLabel) m_cpuLabel->setText(QString("CPU  %1%").arg(int(pct)));
}
void CollapsedBar::setRam(double pct){
    if(m_ramRing)  m_ramRing->setValue(pct/100.0);
    if(m_ramLabel) m_ramLabel->setText(QString("RAM  %1%").arg(int(pct)));
}
void CollapsedBar::setNet(double down, double up){
    if(m_netLabel) m_netLabel->setText(
        QString("%1 MB  ·  %2 KB up").arg(down,0,'f',1).arg(int(up)));
}
void CollapsedBar::setBattery(int pct, bool charging){
    if(m_batIcon)     m_batIcon->setLevel(pct, charging);
    if(m_batPctLabel) {
        QString txt = QString("%1%").arg(pct);
        if (charging) txt += "  +";
        m_batPctLabel->setText(txt);
    }
}
void CollapsedBar::setTime(const QString &t){
    if(m_timeLabel) m_timeLabel->setText(t);
}

// ════════════════════════════════════════════════════════════════════════════
//  Perimeter helpers — map a pixel distance to a point on a rounded-rect edge
// ════════════════════════════════════════════════════════════════════════════
static double perimLength(const QRectF &r, double R)
{
    R = std::min({R, r.width()/2.0, r.height()/2.0});
    return 2.0*(r.width()-2*R) + 2.0*(r.height()-2*R) + 2.0*M_PI*R;
}

static QPointF perimPoint(double d, const QRectF &r, double R)
{
    R = std::min({R, r.width()/2.0, r.height()/2.0});
    double W = r.width(), H = r.height();
    double tL = W-2*R, sL = H-2*R, aL = M_PI*R/2.0;
    double total = 2*(tL+sL) + 4*aL;
    d = fmod(d, total); if (d < 0) d += total;

    // Clockwise from start of top edge
    if (d < tL)  { return QPointF(r.left()+R+d, r.top()); }           d -= tL;
    if (d < aL)  { double a=-M_PI/2+(d/aL)*(M_PI/2);
                   return QPointF(r.right()-R+R*cos(a), r.top()+R+R*sin(a)); } d -= aL;
    if (d < sL)  { return QPointF(r.right(), r.top()+R+d); }          d -= sL;
    if (d < aL)  { double a=(d/aL)*(M_PI/2);
                   return QPointF(r.right()-R+R*cos(a), r.bottom()-R+R*sin(a)); } d -= aL;
    if (d < tL)  { return QPointF(r.right()-R-d, r.bottom()); }       d -= tL;
    if (d < aL)  { double a=M_PI/2+(d/aL)*(M_PI/2);
                   return QPointF(r.left()+R+R*cos(a), r.bottom()-R+R*sin(a)); } d -= aL;
    if (d < sL)  { return QPointF(r.left(), r.bottom()-R-d); }        d -= sL;
    { double a = M_PI+(std::min(d,aL)/aL)*(M_PI/2);
      return QPointF(r.left()+R+R*cos(a), r.top()+R+R*sin(a)); }
}

// Unit tangent (direction of travel) at perimeter distance d — computed numerically
static QPointF perimTangent(double d, const QRectF &r, double R)
{
    QPointF p0 = perimPoint(d,       r, R);
    QPointF p1 = perimPoint(d + 1.0, r, R);
    double dx = p1.x()-p0.x(), dy = p1.y()-p0.y();
    double len = sqrt(dx*dx + dy*dy);
    return (len < 1e-6) ? QPointF(1,0) : QPointF(dx/len, dy/len);
}

// ════════════════════════════════════════════════════════════════════════════
//  SpikeArc — tiny spark particles racing around the widget border
//
//  Each spark is an independent particle with:
//    • A bright white head dot + radial glow halo
//    • A 4-step comet tail fading behind it
//    • Individual speed variation (±25 %)
//    • A random lifetime (0.5–1.8 s) after which it fades and dies
//
//  setLoad(load) drives the system: more sparks + faster at high load.
// ════════════════════════════════════════════════════════════════════════════
SpikeArc::SpikeArc(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);

    m_spinTimer = new QTimer(this);
    m_spinTimer->setInterval(16);   // ~60 fps
    connect(m_spinTimer, &QTimer::timeout, this, &SpikeArc::spinStep);

    m_opacAnim = new QPropertyAnimation(this, "arcOpacity", this);
    m_opacAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void SpikeArc::setLoad(double load)
{
    load = std::clamp(load, 0.0, 1.0);

    // How many live sparks we want
    m_targetCount = load > 0.03 ? 1 : 0;   // always exactly one spark

    // Speed: 300 → 1100 px/sec — fast even at low load
    m_baseSpeed = float(300.0 + load * 800.0);

    // Global alpha envelope (0 → 1 mapped from load)
    m_opacAnim->stop();
    m_opacAnim->setStartValue(double(m_globalAlpha));
    m_opacAnim->setEndValue(load > 0.03 ? std::min(load * 1.15, 1.0) : 0.0);
    m_opacAnim->setDuration(load > 0.03 ? 500 : 1000);
    m_opacAnim->start();

    if (load > 0.03 && !m_spinTimer->isActive())
        m_spinTimer->start();
}

void SpikeArc::spinStep()
{
    const float dt = 0.016f;

    // Compute perimeter length from live widget size
    const int   mg = 3;
    const float CR = 16.f;
    QRectF      r(mg, mg, width()-2*mg, height()-2*mg);
    float perimLen = float(perimLength(r, double(CR)));
    if (perimLen < 1.f) { update(); return; }

    // ── Age + move existing sparks ──────────────────────────────────────────
    for (auto &s : m_sparks) {
        s.pos += s.speed * dt;
        if (s.pos > perimLen) s.pos -= perimLen;
        s.alpha -= s.decay * dt;
    }

    // ── Cull dead sparks ────────────────────────────────────────────────────
    m_sparks.erase(
        std::remove_if(m_sparks.begin(), m_sparks.end(),
            [](const Spark &s){ return s.alpha <= 0.f; }),
        m_sparks.end());

    // ── Spawn new sparks to reach target count ──────────────────────────────
    while (m_sparks.size() < m_targetCount && m_globalAlpha > 0.02f) {
        Spark s;
        // Random starting position spread around perimeter
        s.pos = float(QRandomGenerator::global()->bounded(int(perimLen) + 1));
        // Speed ±25 % variation
        float v = 0.75f + 0.50f * float(QRandomGenerator::global()->generateDouble());
        s.speed = m_baseSpeed * v;
        s.alpha = 1.0f;
        // Lifetime 0.5–1.8 s
        float lt = 60.0f;   // effectively permanent — fades only via globalAlpha
        s.decay = 1.0f / lt;
        // Size 1.5–3.0 px
        s.size  = 4.0f + 2.0f * float(QRandomGenerator::global()->generateDouble());
        m_sparks.append(s);
    }

    // ── Stop timer when nothing left to draw ────────────────────────────────
    if (m_sparks.isEmpty() && m_globalAlpha < 0.005f)
        m_spinTimer->stop();

    update();
}

void SpikeArc::paintEvent(QPaintEvent *)
{
    if (m_sparks.isEmpty() && m_globalAlpha < 0.005f) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int    mg = 3;
    const double CR = 16.0;
    QRectF r(mg, mg, width()-2*mg, height()-2*mg);
    if (perimLength(r, CR) < 1.0) return;

    for (const auto &spark : m_sparks) {
        float ga = spark.alpha * m_globalAlpha;
        if (ga < 0.01f) continue;

        QPointF head = perimPoint(double(spark.pos), r, CR);
        QPointF tang = perimTangent(double(spark.pos), r, CR);

        // streak geometry: tail behind, small overshoot in front
        float tailLen  = 40.f;
        float frontLen = 10.f;
        float strokeW  = spark.size * 1.10f;   // core line width

        QPointF tail(head.x() - tang.x()*tailLen,  head.y() - tang.y()*tailLen);
        QPointF front(head.x() + tang.x()*frontLen, head.y() + tang.y()*frontLen);

        // half-way point (where medium glow starts)
        QPointF mid(head.x() - tang.x()*tailLen*0.45f,
                    head.y() - tang.y()*tailLen*0.45f);

        p.setBrush(Qt::NoBrush);

        // ── Pass 1: wide outer glow covering full streak ──────────────────────
        p.setPen(QPen(QColor(255,255,255, int(ga*50)),
                      strokeW*5.f, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(mid, front);

        // ── Pass 2: medium glow, front two-thirds only ────────────────────────
        p.setPen(QPen(QColor(255,255,255, int(ga*110)),
                      strokeW*2.5f, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(head.x()-tang.x()*tailLen*0.30f,
                           head.y()-tang.y()*tailLen*0.30f), front);

        // ── Pass 3: bright thin core — full tail to front ─────────────────────
        p.setPen(QPen(QColor(255,255,255, int(ga*220)),
                      strokeW, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(tail, front);

        // ── Bright tip dot at the very front ─────────────────────────────────
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        float tipR = strokeW * 1.1f;
        p.drawEllipse(front, double(tipR), double(tipR));
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  PulseWidget
// ════════════════════════════════════════════════════════════════════════════
PulseWidget::PulseWidget(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    buildUi();

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &PulseWidget::onTick);
    m_timer->start();

    readCpuStat(m_prevIdle, m_prevTotal);
    onTick();

    // Compute layout and set initial position before window is mapped
    adjustSize();
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        move(geo.right()-width()-12, geo.bottom()-height()-12);
    }
}

void PulseWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        // Fallback: If WM still centers it on map, forcefully snap it after 500ms
        QTimer::singleShot(500, this, [this]() {
            QScreen *screen = QApplication::primaryScreen();
            if (screen) {
                QRect geo = screen->availableGeometry();
                move(geo.right()-width()-12, geo.bottom()-height()-12);
            }
        });
    }
}

void PulseWidget::buildUi()
{
    m_rootLayout = new QHBoxLayout(this);
    m_rootLayout->setContentsMargins(0,0,0,0);
    m_rootLayout->setSpacing(0);

    m_pulseLogo = new PulseLogoWidget(this);
    m_pulseLogo->setVisible(true); // Start in icon mode

    buildCollapsedBar();
    m_collapsedBar->setVisible(false);
    buildExpandedPanel();
    buildSettingsPanel();

    m_rootLayout->addWidget(m_pulseLogo);
    m_rootLayout->addWidget(m_collapsedBar);
    m_rootLayout->addWidget(m_expandedPanel);
    m_rootLayout->addWidget(m_settingsPanel);

    m_expandedPanel->hide();
    m_settingsPanel->hide();

    m_spikeArc = new SpikeArc(this);
    m_spikeArc->setVisible(!m_isIconMode);
    m_spikeArc->raise();

    adjustSize();
    if (m_spikeArc) m_spikeArc->setGeometry(rect());
}

void PulseWidget::buildCollapsedBar()
{
    m_collapsedBar = new CollapsedBar(this);
}

void PulseWidget::buildExpandedPanel()
{
    m_expandedPanel = new QWidget(this);
    m_expandedPanel->setFixedWidth(360);

    auto *vl = new QVBoxLayout(m_expandedPanel);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(0);

    // ── Compact header ────────────────────────────────────────────────────────
    auto *header = new QWidget(m_expandedPanel);
    header->setFixedHeight(68);
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(18,12,14,10);
    hl->setSpacing(0);

    auto *clockCol = new QVBoxLayout;
    clockCol->setSpacing(1);
    m_clockLabel = new QLabel("00:00:00", header);
    m_clockLabel->setFont(monoFont(30));
    m_clockLabel->setStyleSheet("color:white;");
    m_clockLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_dateLabel = new QLabel("—", header);
    m_dateLabel->setFont(capFont(10));
    m_dateLabel->setStyleSheet("color:rgba(255,255,255,0.36);");
    m_dateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    clockCol->addWidget(m_clockLabel);
    clockCol->addWidget(m_dateLabel);
    hl->addLayout(clockCol);
    hl->addStretch();

    auto mkBtn = [&](const QString &txt){
        auto *b = new QPushButton(txt, header);
        b->setFixedSize(28,28);
        b->setFont(sansFont(14));
        b->setStyleSheet(R"(
            QPushButton{background:rgba(255,255,255,0.06);border:none;border-radius:14px;
                color:rgba(255,255,255,0.45);}
            QPushButton:hover{background:rgba(255,255,255,0.16);color:white;}
        )");
        return b;
    };
    auto *settingBtn = mkBtn("=");
    auto *closeBtn   = mkBtn("x");

    hl->addWidget(settingBtn); hl->addSpacing(6); hl->addWidget(closeBtn);
    connect(settingBtn, &QPushButton::clicked, this, &PulseWidget::toggleSettings);
    connect(closeBtn,   &QPushButton::clicked, this, &PulseWidget::toggleExpanded);
    vl->addWidget(header);

    // ── Separator ─────────────────────────────────────────────────────────────
    auto sep = [&](){
        auto *f = new QFrame(m_expandedPanel);
        f->setFrameShape(QFrame::HLine);
        f->setStyleSheet("color:rgba(255,255,255,0.07);");
        f->setAttribute(Qt::WA_TransparentForMouseEvents);
        return f;
    };
    vl->addWidget(sep());

    // ── Six ring rows ─────────────────────────────────────────────────────────
    auto *statsWidget = new QWidget(m_expandedPanel);
    auto *svl = new QVBoxLayout(statsWidget);
    svl->setContentsMargins(0,4,0,6);
    svl->setSpacing(0);

    m_cpuRow  = new RingRow("CPU",  statsWidget);
    m_ramRow  = new RingRow("RAM",  statsWidget);
    m_netRow  = new RingRow("NET",  statsWidget);
    m_batRow  = new RingRow("BAT",  statsWidget);
    m_tempRow = new RingRow("TEMP", statsWidget);
    m_diskRow = new RingRow("DISK", statsWidget);

    auto addRow = [&](RingRow *row){
        svl->addWidget(row);
        auto *d = new QFrame(statsWidget);
        d->setFrameShape(QFrame::HLine);
        d->setStyleSheet("color:rgba(255,255,255,0.05);");
        d->setAttribute(Qt::WA_TransparentForMouseEvents);
        svl->addWidget(d);
    };
    addRow(m_cpuRow);
    addRow(m_ramRow);
    addRow(m_netRow);
    addRow(m_batRow);
    addRow(m_tempRow);
    svl->addWidget(m_diskRow);

    vl->addWidget(statsWidget);
}

void PulseWidget::buildSettingsPanel()
{
    m_settingsPanel = new SettingsPanel(this);
    connect(m_settingsPanel, &SettingsPanel::opacityChanged,
            this, &PulseWidget::applyOpacity);
    connect(m_settingsPanel, &SettingsPanel::alwaysOnTopChanged,
            this, &PulseWidget::applyAlwaysOnTop);
    connect(m_settingsPanel, &SettingsPanel::closeRequested,
            this, &PulseWidget::toggleSettings);
}

// ════════════════════════════════════════════════════════════════════════════
//  paintEvent  —  black glass + shiny gloss
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r = QRectF(rect()).adjusted(0.5,0.5,-0.5,-0.5);
    constexpr double rad = 16.0;
    QPainterPath path;
    path.addRoundedRect(r, rad, rad);

    // solid black base
    p.fillPath(path, QColor(0,0,0, int(m_baseOpacity*218)));

    // depth: slightly lighter at top
    QLinearGradient dg(r.topLeft(), r.bottomLeft());
    dg.setColorAt(0.0, P::w(14)); dg.setColorAt(0.4, P::w(4)); dg.setColorAt(1.0, P::w(0));
    p.fillPath(path, dg);

    // shiny top-gloss stripe
    QPainterPath gp;
    QRectF gr(r.x()+1, r.y()+1, r.width()-2, r.height()*0.36);
    gp.addRoundedRect(gr, rad-1, rad-1);
    gp &= path;
    QLinearGradient gg(gr.topLeft(), gr.bottomLeft());
    gg.setColorAt(0.0, P::w(30)); gg.setColorAt(0.55, P::w(8)); gg.setColorAt(1.0, P::w(0));
    p.fillPath(gp, gg);

    // outer border
    p.setPen(QPen(m_isIconMode ? P::w(160) : P::w(36), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    // inner top-edge glint
    QPainterPath ip;
    ip.addRoundedRect(r.adjusted(1,1,-1,-1), rad-1, rad-1);
    p.setPen(QPen(P::w(10), 1.0));
    p.drawPath(ip);
}

void PulseWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (m_spikeArc) { m_spikeArc->setGeometry(rect()); m_spikeArc->raise(); }
}

// ════════════════════════════════════════════════════════════════════════════
//  Toggle
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::toggleExpanded()
{
    m_expanded = !m_expanded;
    m_expandedPanel->setVisible(m_expanded);
    if (!m_expanded) { m_settingsPanel->hide(); m_showSettings = false; }
    adjustSize();
    if (m_spikeArc) { m_spikeArc->setGeometry(rect()); m_spikeArc->raise(); }
}
void PulseWidget::toggleSettings()
{
    m_showSettings = !m_showSettings;
    m_settingsPanel->setVisible(m_showSettings);
    adjustSize();
    if (m_spikeArc) { m_spikeArc->setGeometry(rect()); m_spikeArc->raise(); }
}

// ════════════════════════════════════════════════════════════════════════════
//  Drag
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::toggleMode()
{
    QRect oldGeo = geometry();
    
    m_isIconMode = !m_isIconMode;
    m_pulseLogo->setVisible(m_isIconMode);
    m_collapsedBar->setVisible(!m_isIconMode);
    if (m_spikeArc) m_spikeArc->setVisible(!m_isIconMode);
    
    adjustSize();
    
    // Pin to the right edge: move it so the new right edge matches the old right edge
    move(oldGeo.right() - width() + 1, pos().y());
    
    if (m_snapEdge) snapToEdge();
    if (m_spikeArc) { m_spikeArc->setGeometry(rect()); m_spikeArc->raise(); }
    update();
}

void PulseWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button()==Qt::LeftButton && !m_lockPos) {
        m_dragging=true; m_dragStart=e->globalPosition().toPoint();
        m_widgetStart=pos(); e->accept();
    }
}
void PulseWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {
        move(m_widgetStart + (e->globalPosition().toPoint()-m_dragStart));
        e->accept();
    }
}
void PulseWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button()==Qt::LeftButton) {
        if (m_dragging && (e->globalPosition().toPoint() - m_dragStart).manhattanLength() < 5) {
            toggleMode();
        }
        m_dragging=false; 
        if (m_snapEdge) snapToEdge(); 
        e->accept();
    }
}
void PulseWidget::snapToEdge()
{
    QScreen *sc = QApplication::primaryScreen();
    QRect geo = sc->availableGeometry();
    QPoint pos = this->pos();
    const int d = 40;
    if (pos.x()         < geo.left()  +d) pos.setX(geo.left()+8);
    if (pos.x()+width() > geo.right() -d) pos.setX(geo.right()-width()-8);
    if (pos.y()          < geo.top()  +d) pos.setY(geo.top()+8);
    if (pos.y()+height() > geo.bottom()-d) pos.setY(geo.bottom()-height()-8);
    move(pos);
}

// ════════════════════════════════════════════════════════════════════════════
//  Settings
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::applyOpacity(int pct) { m_baseOpacity=pct/100.0; update(); }
void PulseWidget::applyAlwaysOnTop(bool v)
{
    Qt::WindowFlags f = windowFlags();
    setWindowFlags(v ? (f|Qt::WindowStaysOnTopHint) : (f&~Qt::WindowStaysOnTopHint));
    show();
}

// ════════════════════════════════════════════════════════════════════════════
//  CPU stat
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::readCpuStat(double &idle, double &total)
{
    QFile f("/proc/stat");
    if (!f.open(QIODevice::ReadOnly)) return;
    QString line = f.readLine(); f.close();
    QStringList p = line.split(' ', Qt::SkipEmptyParts);
    if (p.size()<5) return;
    double user=p[1].toDouble(), nice=p[2].toDouble(), sys=p[3].toDouble();
    double idleV=p[4].toDouble();
    double iow  = p.size()>5?p[5].toDouble():0;
    double irq  = p.size()>6?p[6].toDouble():0;
    double sirq = p.size()>7?p[7].toDouble():0;
    idle  = idleV+iow;
    total = user+nice+sys+idle+irq+sirq;
}

// ════════════════════════════════════════════════════════════════════════════
//  updateStats
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::updateStats()
{
    // CPU
    double idle, total;
    readCpuStat(idle, total);
    double dI=idle-m_prevIdle, dT=total-m_prevTotal;
    if (dT>0) m_cpuPct=(1.0-dI/dT)*100.0;
    m_prevIdle=idle; m_prevTotal=total;

    // RAM — whole-file byte scanner (bulletproof)
    {
        QFile mf("/proc/meminfo");
        if (mf.open(QIODevice::ReadOnly)) {
            QByteArray raw = mf.readAll(); mf.close();
            auto extractKB = [&](const char *key) -> double {
                int ki = raw.indexOf(key);
                if (ki<0) return 0.0;
                ki += (int)strlen(key);
                while (ki<raw.size() && (raw[ki]==' '||raw[ki]=='\t')) ki++;
                double val=0;
                while (ki<raw.size() && raw[ki]>='0' && raw[ki]<='9') {
                    val=val*10+(raw[ki]-'0'); ki++;
                }
                return val;
            };
            double total_kb = extractKB("MemTotal:");
            double avail_kb = extractKB("MemAvailable:");
            if (avail_kb==0.0) avail_kb = extractKB("MemFree:");
            if (total_kb>0) {
                double used_kb = total_kb-avail_kb;
                m_ramTotal = total_kb/(1024.0*1024.0);
                m_ramUsed  = used_kb /(1024.0*1024.0);
                m_ramPct   = (used_kb/total_kb)*100.0;
            }
        }
    }

    // Network
    {
        QFile nf("/proc/net/dev");
        if (nf.open(QIODevice::ReadOnly)) {
            unsigned long long rx=0,tx=0;
            nf.readLine(); nf.readLine();
            while (!nf.atEnd()) {
                QString ln=nf.readLine().trimmed();
                if (ln.startsWith("lo")) continue;
                QStringList p=ln.split(' ',Qt::SkipEmptyParts);
                if (p.size()>=10) { rx+=p[1].toULongLong(); tx+=p[9].toULongLong(); }
            }
            nf.close();
            if (m_prevRxBytes>0) {
                m_downMB=(rx-m_prevRxBytes)/(1024.0*1024.0);
                m_upKB  =(tx-m_prevTxBytes)/1024.0;
            }
            m_prevRxBytes=rx; m_prevTxBytes=tx;
        }
    }

    // Battery
    {
        QDir bd("/sys/class/power_supply");
        for (const auto &e : bd.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot)) {
            QFile tf(e.filePath()+"/type");
            if (!tf.open(QIODevice::ReadOnly)) continue;
            bool isBat = tf.readAll().trimmed()=="Battery"; tf.close();
            if (!isBat) continue;
            QFile cf(e.filePath()+"/capacity");
            if (cf.open(QIODevice::ReadOnly)) { m_batPct=cf.readAll().trimmed().toInt(); cf.close(); }
            QFile sf(e.filePath()+"/status");
            if (sf.open(QIODevice::ReadOnly)) { m_charging=sf.readAll().trimmed()=="Charging"; sf.close(); }
            break;
        }
    }

    // Temperature
    {
        QDir td("/sys/class/thermal");
        for (const auto &e : td.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot)) {
            if (!e.fileName().startsWith("thermal_zone")) continue;
            QFile tf(e.filePath()+"/temp");
            if (tf.open(QIODevice::ReadOnly)) {
                int milli=tf.readAll().trimmed().toInt(); tf.close();
                if (milli>0) { m_tempC=milli/1000.0; break; }
            }
        }
    }

    // Disk
    {
        FILE *f=popen("df / --block-size=1G --output=used,size 2>/dev/null | tail -1","r");
        if (f) { double u=0,s=0; fscanf(f,"%lf %lf",&u,&s); pclose(f); m_diskUsed=u; m_diskTotal=s; }
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  onTick
// ════════════════════════════════════════════════════════════════════════════
void PulseWidget::onTick()
{
    updateStats();

    // ── Feed live load to SpikeArc and trigger heartbeat ──────────────────────
    if (m_spikeArc) {
        double load = (m_cpuPct * 0.65 + m_ramPct * 0.35) / 100.0;
        m_spikeArc->setLoad(load);
    }
    if (m_pulseLogo) {
        m_pulseLogo->triggerBeat();
    }

    // ── Clock ─────────────────────────────────────────────────────────────────
    QDateTime now = QDateTime::currentDateTime();
    if (m_clockLabel) m_clockLabel->setText(now.toString("HH:mm:ss"));
    if (m_dateLabel)  m_dateLabel->setText(now.toString("ddd, dd MMM yyyy").toUpper());

    // ── Collapsed bar ─────────────────────────────────────────────────────────
    // Time + date merged into one same-size label
    m_collapsedBar->setTime(
        now.toString("HH:mm   dd MMM yyyy").toUpper());
    m_collapsedBar->setCpu(m_cpuPct);
    m_collapsedBar->setRam(m_ramPct);
    m_collapsedBar->setNet(m_downMB, m_upKB);
    m_collapsedBar->setBattery(m_batPct, m_charging);

    // ── Ring rows ─────────────────────────────────────────────────────────────
    if (m_cpuRow)
        m_cpuRow->update(m_cpuPct/100.0, QString("%1%").arg(int(m_cpuPct)));

    if (m_ramRow)
        m_ramRow->update(m_ramPct/100.0,
            QString("%1 / %2 GB").arg(m_ramUsed,0,'f',1).arg(m_ramTotal,0,'f',1));

    if (m_netRow)
        m_netRow->update(std::min(m_downMB/10.0,1.0),
            QString("%1 MB/s").arg(m_downMB,0,'f',1),
            QString("%1 KB/s up").arg(int(m_upKB)));

    if (m_batRow)
        m_batRow->update(m_batPct/100.0,
            QString("%1%  %2").arg(m_batPct).arg(m_charging?"charging":"discharging"));

    if (m_tempRow)
        m_tempRow->update(std::min(m_tempC/100.0,1.0),
            QString("%1 C").arg(m_tempC,0,'f',1));

    if (m_diskRow && m_diskTotal>0)
        m_diskRow->update(m_diskUsed/m_diskTotal,
            QString("%1 / %2 GB").arg(int(m_diskUsed)).arg(int(m_diskTotal)));
}
