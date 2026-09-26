#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QPoint>
#include <QString>
#include <QList>
#include <QFrame>
#include <QSlider>
#include <QCheckBox>
#include <QScrollArea>

// ─── Sparkline ────────────────────────────────────────────────────────────────
class SparkLine : public QWidget
{
    Q_OBJECT
public:
    explicit SparkLine(QWidget *parent = nullptr);
    void addValue(double v);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    QList<double> m_data;
    static constexpr int kMaxPoints = 30;
};

// ─── Ring gauge ───────────────────────────────────────────────────────────────
class RingGauge : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue)
public:
    explicit RingGauge(int size = 42, bool showLabel = false, QWidget *parent = nullptr);
    double value() const { return m_value; }
    void   setValue(double v);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    double m_value     = 0.0;
    int    m_size;
    bool   m_showLabel;
};

// ─── Stat row: ring + label + value + spark ───────────────────────────────────
class RingRow : public QWidget
{
    Q_OBJECT
public:
    explicit RingRow(const QString &label, QWidget *parent = nullptr);
    void update(double fraction, const QString &valueText, const QString &extraText = {});
private:
    RingGauge *m_ring;
    QLabel    *m_nameLabel;
    QLabel    *m_valueLabel;
    SparkLine *m_spark;
};

// ─── Settings panel ───────────────────────────────────────────────────────────
class SettingsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPanel(QWidget *parent = nullptr);
signals:
    void opacityChanged(int pct);
    void alwaysOnTopChanged(bool v);
    void closeRequested();
private:
    QFrame *makeSeparator();
};

// ─── Painted white battery icon ───────────────────────────────────────────────
class BatteryIcon : public QWidget
{
    Q_OBJECT
public:
    explicit BatteryIcon(QWidget *parent = nullptr);
    void setLevel(int pct, bool charging);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    int  m_pct      = 0;
    bool m_charging = false;
};

// ─── Collapsed bar ────────────────────────────────────────────────────────────
class CollapsedBar : public QWidget
{
    Q_OBJECT
public:
    explicit CollapsedBar(QWidget *parent = nullptr);
    void setCpu(double pct);
    void setRam(double pct);
    void setNet(double downMB, double upKB);
    void setBattery(int pct, bool charging);
    void setTime(const QString &t);
private:
    QLabel      *m_timeLabel   = nullptr;
    QLabel      *m_cpuLabel    = nullptr;
    QLabel      *m_ramLabel    = nullptr;
    QLabel      *m_netLabel    = nullptr;
    QLabel      *m_batPctLabel = nullptr;   // shows "87%"
    BatteryIcon *m_batIcon     = nullptr;   // drawn white battery icon
    RingGauge   *m_cpuRing     = nullptr;
    RingGauge   *m_ramRing     = nullptr;
};

// ─── Collapsed pulse symbol (beating heartbeat) ───────────────────────────────
class PulseLogoWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float beatScale READ beatScale WRITE setBeatScale)
public:
    explicit PulseLogoWidget(QWidget *parent = nullptr);
    float beatScale() const { return m_beatScale; }
    void  setBeatScale(float v) { m_beatScale = v; update(); }
    void  triggerBeat();
protected:
    void paintEvent(QPaintEvent *) override;
private:
    float m_beatScale = 1.0f;
    QPropertyAnimation *m_anim = nullptr;
};

// ─── Tiny spark particles racing around the widget border ─────────────────────
class SpikeArc : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double arcOpacity READ arcOpacity WRITE setArcOpacity)
public:
    explicit SpikeArc(QWidget *parent = nullptr);
    // Feed current load (0–1) each second; sparks appear/speed up proportionally.
    void setLoad(double load);

    double arcOpacity() const { return double(m_globalAlpha); }
    void   setArcOpacity(double o) { m_globalAlpha = float(o); }

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void spinStep();   // ~60 fps inner timer

private:
    struct Spark {
        float pos;    // pixel distance along widget perimeter
        float speed;  // px / sec (individual variation)
        float alpha;  // current brightness 0–1
        float decay;  // alpha units lost per second
        float size;   // head dot radius (px)
    };

    QList<Spark>        m_sparks;
    int                 m_targetCount = 0;    // desired live spark count
    float               m_baseSpeed   = 80.f; // px/sec base
    float               m_globalAlpha = 0.f;
    QTimer             *m_spinTimer;
    QPropertyAnimation *m_opacAnim;
};

// ─── Main widget ──────────────────────────────────────────────────────────────
class PulseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PulseWidget(QWidget *parent = nullptr);
protected:
    void mousePressEvent(QMouseEvent *e)   override;
    void mouseMoveEvent(QMouseEvent *e)    override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e)        override;
    void resizeEvent(QResizeEvent *e)      override;
    void showEvent(QShowEvent *e)          override;
private slots:
    void onTick();
    void toggleMode();
    void toggleExpanded();
    void toggleSettings();
    void applyOpacity(int pct);
    void applyAlwaysOnTop(bool v);
private:
    void buildUi();
    void buildCollapsedBar();
    void buildExpandedPanel();
    void buildSettingsPanel();
    void updateStats();
    void readCpuStat(double &idle, double &total);
    void snapToEdge();

    bool   m_expanded     = false;
    bool   m_showSettings = false;
    bool   m_dragging     = false;
    QPoint m_dragStart;
    QPoint m_widgetStart;
    double m_baseOpacity  = 0.88;
    bool   m_snapEdge     = true;
    bool   m_lockPos      = false;

    double             m_cpuPct      = 0;
    double             m_ramPct      = 0;
    double             m_ramUsed     = 0;
    double             m_ramTotal    = 0;
    double             m_downMB      = 0;
    double             m_upKB        = 0;
    int                m_batPct      = 0;
    bool               m_charging    = false;
    double             m_tempC       = 0;
    double             m_diskUsed    = 0;
    double             m_diskTotal   = 0;
    double             m_prevIdle    = 0;
    double             m_prevTotal   = 0;
    unsigned long long m_prevRxBytes = 0;
    unsigned long long m_prevTxBytes = 0;

    QTimer          *m_timer;
    bool            m_isIconMode    = true;
    PulseLogoWidget *m_pulseLogo    = nullptr;
    CollapsedBar    *m_collapsedBar = nullptr;
    QWidget         *m_expandedPanel= nullptr;
    SettingsPanel   *m_settingsPanel= nullptr;
    QHBoxLayout     *m_rootLayout   = nullptr;
    SpikeArc        *m_spikeArc     = nullptr;

    RingRow *m_cpuRow  = nullptr;
    RingRow *m_ramRow  = nullptr;
    RingRow *m_netRow  = nullptr;
    RingRow *m_batRow  = nullptr;
    RingRow *m_tempRow = nullptr;
    RingRow *m_diskRow = nullptr;
    QLabel  *m_clockLabel = nullptr;
    QLabel  *m_dateLabel  = nullptr;
};
