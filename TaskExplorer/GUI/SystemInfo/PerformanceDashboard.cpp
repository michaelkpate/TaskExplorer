#include "stdafx.h"
#include "PerformanceDashboard.h"
#include "../../API/SystemAPI.h"
#include "../../../MiscHelpers/Common/Settings.h"
#include "../../API/Monitors/GpuMonitor.h"
#include <QPainter>
#include <QPainterPath>
#include <QElapsedTimer>
#include <QSignalBlocker>
#include <cmath>
#include <limits>

// These cards intentionally use only Qt: no additional chart or sensor dependency.
class CMetricCard : public QWidget
{
public:
    CMetricCard(const QString& title, const QColor& color, QWidget* parent)
        : QWidget(parent), m_Color(color)
    {
        setObjectName("metricCard");
        setAttribute(Qt::WA_StyledBackground);
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(22, 18, 22, 18);
        auto heading = new QLabel(title, this);
        heading->setStyleSheet("color: " + color.name() + "; font-size: 13px; font-weight: 600;");
        layout->addWidget(heading);
        m_Value = new QLabel(tr("Waiting for data"), this);
        m_Value->setStyleSheet("font-size: 32px; font-weight: 600;");
        layout->addWidget(m_Value);
        m_Detail = new QLabel(this);
        m_Detail->setWordWrap(true);
        m_Detail->setMinimumHeight(40);
        m_Detail->setStyleSheet("color: #a6b2c3; font-size: 12px;");
        layout->addWidget(m_Detail);
        m_Graph = new QWidget(this);
        m_Graph->setMinimumHeight(95);
        m_Graph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_Graph, 1);
        auto axis = new QHBoxLayout();
        auto duration = new QLabel(tr("60 seconds"), this);
        auto scale = new QLabel(tr("0–100%"), this);
        duration->setStyleSheet("color: #8492a6; font-size: 11px;");
        scale->setStyleSheet("color: #8492a6; font-size: 11px;");
        axis->addWidget(duration);
        axis->addStretch();
        axis->addWidget(scale);
        layout->addLayout(axis);
        m_Clock.start();
    }

    void Clear()
    {
        m_History.clear();
        update();
    }

    void Sample(double percent, const QString& value, const QString& detail)
    {
        m_Value->setText(value);
        m_Detail->setText(detail);
        setAccessibleName(value + ". " + detail);
        const qint64 now = m_Clock.elapsed();
        // NaN is a gap, not a fabricated zero when a counter is unavailable.
        m_History.append({now, percent});
        while (!m_History.isEmpty() && m_History.first().time < now - 60000)
            m_History.removeFirst();
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QRectF graph = m_Graph->geometry();
        graph.adjust(1, 1, -1, -1);
        painter.setPen(QPen(QColor("#283344"), 1));
        for (int i = 0; i <= 4; ++i) {
            double y = graph.top() + graph.height() * i / 4;
            painter.drawLine(QPointF(graph.left(), y), QPointF(graph.right(), y));
        }
        for (int i = 0; i <= 12; ++i) {
            double x = graph.left() + graph.width() * i / 12;
            painter.drawLine(QPointF(x, graph.top()), QPointF(x, graph.bottom()));
        }
        if (m_History.isEmpty()) return;
        painter.setClipRect(graph.adjusted(-2, -2, 2, 2));
        QPainterPath line;
        bool started = false;
        const qint64 now = m_History.last().time;
        for (const auto& sample : m_History) {
            if (!std::isfinite(sample.percent)) { started = false; continue; }
            QPointF point(graph.right() - graph.width() * (now - sample.time) / 60000.0,
                graph.bottom() - graph.height() * qBound(0.0, sample.percent, 100.0) / 100.0);
            if (!started) line.moveTo(point); else line.lineTo(point);
            started = true;
        }
        QColor glow = m_Color;
        glow.setAlpha(35);
        painter.setPen(QPen(glow, 7));
        painter.drawPath(line);
        painter.setPen(QPen(m_Color, 2));
        painter.drawPath(line);
    }

private:
    struct SamplePoint { qint64 time; double percent; };
    QList<SamplePoint> m_History;
    QElapsedTimer m_Clock;
    QColor m_Color;
    QLabel* m_Value;
    QLabel* m_Detail;
    QWidget* m_Graph;
};

static QString DashboardGiB(quint64 bytes)
{
    return QString::number(double(bytes) / (1024.0 * 1024 * 1024), 'f', 1);
}

CPerformanceDashboard::CPerformanceDashboard(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("performanceDashboard");
    setWindowTitle(tr("Performance Dashboard — This PC"));
    resize(1000, 730);
    setMinimumSize(720, 600);
    setStyleSheet(
        "QWidget#performanceDashboard { background: #101722; color: #edf3fc; }"
        "QWidget#metricCard { background: #192231; border: 1px solid #2b374a; border-radius: 12px; }"
        "QLabel, QCheckBox { color: #edf3fc; background: transparent; }"
        "QComboBox { color: #edf3fc; background: #202d40; padding: 7px; border: 1px solid #40516b; border-radius: 5px; }"
        "QComboBox QAbstractItemView { color: #edf3fc; background: #202d40; selection-background-color: #365276; }");
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 24);
    layout->setSpacing(16);
    auto header = new QHBoxLayout();
    auto title = new QLabel(tr("Performance"), this);
    title->setStyleSheet("font-size: 26px; font-weight: 600;");
    header->addWidget(title);
    header->addStretch();
    auto pin = new QCheckBox(tr("Always on top"), this);
    header->addWidget(pin);
    connect(pin, &QCheckBox::toggled, this, [this](bool checked) {
        setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        show();
    });
    layout->addLayout(header);
    auto subtitle = new QLabel(tr("THIS PC  /  CPU · SYSTEM RAM · GPU · DEDICATED VRAM"), this);
    subtitle->setStyleSheet("color: #8492a6; font-size: 11px;");
    layout->addWidget(subtitle);
    auto adapterRow = new QHBoxLayout();
    adapterRow->addWidget(new QLabel(tr("Graphics adapter"), this));
    m_Adapters = new QComboBox(this);
    m_Adapters->setAccessibleName(tr("Graphics adapter"));
    m_Adapters->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_Adapters->setMinimumContentsLength(20);
    adapterRow->addWidget(m_Adapters, 1);
    layout->addLayout(adapterRow);
    auto grid = new QGridLayout();
    grid->setSpacing(16);
    m_Cpu = new CMetricCard(tr("CPU"), QColor("#52d9c5"), this);
    m_Ram = new CMetricCard(tr("SYSTEM RAM"), QColor("#bf9bff"), this);
    m_Gpu = new CMetricCard(tr("GPU"), QColor("#50bdff"), this);
    m_Vram = new CMetricCard(tr("GPU VRAM"), QColor("#f3bf70"), this);
    grid->addWidget(m_Cpu, 0, 0);
    grid->addWidget(m_Ram, 0, 1);
    grid->addWidget(m_Gpu, 1, 0);
    grid->addWidget(m_Vram, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    layout->addLayout(grid, 1);
    auto refreshNote = new QLabel(tr("Reads local counters every second. TaskExplorer's Pause Refresh also pauses collection."), this);
    refreshNote->setWordWrap(true);
    refreshNote->setStyleSheet("color: #8492a6; font-size: 11px;");
    layout->addWidget(refreshNote);
    connect(m_Adapters, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { Refresh(); });
    restoreGeometry(theConf->GetBlob("PerformanceDashboard/Geometry"));
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { Refresh(); });
    timer->start(1000);
    Refresh();
}

CPerformanceDashboard::~CPerformanceDashboard()
{
    theConf->SetBlob("PerformanceDashboard/Geometry", saveGeometry());
}

void CPerformanceDashboard::Refresh()
{
    // Deliberately local, even when the main process tree selects a remote host.
    const double unavailable = std::numeric_limits<double>::quiet_NaN();
    double cpu = double(theSystem->GetCpuUsage()) * 100.0;
    m_Cpu->Sample(cpu, tr("%1%").arg(cpu, 0, 'f', 1),
        tr("%1\n%2 logical processors").arg(theSystem->GetCpuModel()).arg(theSystem->GetCpuCount()));
    // Windows stores usable capacity here; Linux stores currently free memory.
#ifdef WIN32
    quint64 total = theSystem->GetAvailableMemory();
#else
    quint64 total = theSystem->GetInstalledMemory();
#endif
    quint64 used = theSystem->GetPhysicalUsed();
    quint64 available = 0;
#ifdef WIN32
    available = total > used ? total - used : 0;
#else
    available = theSystem->GetAvailableMemory();
#endif
    m_Ram->Sample(total ? 100.0 * used / total : unavailable,
        total ? tr("%1 / %2 GiB").arg(DashboardGiB(used), DashboardGiB(total)) : tr("Unavailable"),
        total ? tr("%1% in use · %2 GiB available · %3 GiB installed")
            .arg(100.0 * used / total, 0, 'f', 1).arg(DashboardGiB(available))
            .arg(DashboardGiB(theSystem->GetInstalledMemory())) : tr("Physical memory counters unavailable"));

    auto monitor = theSystem->GetGpuMonitor();
    auto adapters = monitor ? monitor->GetAllGpuList() : QMap<QString, CGpuMonitor::SGpuInfo>();
    QString selected = m_Adapters->currentData().toString();
    if (!adapters.contains(selected)) {
        // Prefer the adapter with the largest dedicated capacity on first open/removal.
        quint64 largest = 0;
        selected.clear();
        for (auto it = adapters.cbegin(); it != adapters.cend(); ++it) {
            if (selected.isEmpty() || it->Memory.DedicatedLimit > largest) {
                selected = it.key();
                largest = it->Memory.DedicatedLimit;
            }
        }
    }
    bool adapterListChanged = adapters.isEmpty() ? m_Adapters->isEnabled() : m_Adapters->count() != adapters.size();
    int adapterIndex = 0;
    for (auto it = adapters.cbegin(); !adapterListChanged && it != adapters.cend(); ++it, ++adapterIndex)
        adapterListChanged = m_Adapters->itemData(adapterIndex).toString() != it.key()
            || m_Adapters->itemText(adapterIndex) != it->Description;
    if (adapterListChanged) {
        QSignalBlocker blocker(m_Adapters);
        m_Adapters->clear();
        for (auto it = adapters.cbegin(); it != adapters.cend(); ++it)
            m_Adapters->addItem(it->Description, it.key());
        m_Adapters->setCurrentIndex(m_Adapters->findData(selected));
        m_Adapters->setEnabled(!adapters.isEmpty());
        if (adapters.isEmpty()) m_Adapters->addItem(tr("No GPU counters available"));
    }
    if (selected != m_SelectedAdapter) {
        m_Gpu->Clear();
        m_Vram->Clear();
        m_SelectedAdapter = selected;
    }
    if (!adapters.contains(selected)) {
        m_Gpu->Sample(unavailable, tr("Unavailable"), tr("No graphics adapter reported by the monitor"));
        m_Vram->Sample(unavailable, tr("Unavailable"), tr("Dedicated memory counters unavailable"));
        return;
    }
    const auto& gpu = adapters[selected];
    m_Gpu->Sample(gpu.UsageAvailable ? 100.0 * gpu.TimeUsage : unavailable,
        gpu.UsageAvailable ? tr("%1%").arg(100.0 * gpu.TimeUsage, 0, 'f', 1) : tr("Unavailable"), gpu.Description);
    const auto& mem = gpu.Memory;
    m_Vram->Sample(mem.DedicatedLimit ? 100.0 * mem.DedicatedUsage / mem.DedicatedLimit : unavailable,
        mem.DedicatedLimit ? tr("%1 / %2 GiB").arg(DashboardGiB(mem.DedicatedUsage), DashboardGiB(mem.DedicatedLimit)) : tr("Unavailable"),
        tr("Dedicated GPU memory · Shared: %1 GiB").arg(DashboardGiB(mem.SharedUsage)));
}
