#pragma once

#include <QWidget>

class CPerformanceDashboard : public QWidget
{
public:
    explicit CPerformanceDashboard(QWidget* parent = nullptr);
    ~CPerformanceDashboard() override;

private:
    void Refresh();
    class CMetricCard* m_Cpu;
    class CMetricCard* m_Ram;
    class CMetricCard* m_Gpu;
    class CMetricCard* m_Vram;
    class QComboBox* m_Adapters;
    QString m_SelectedAdapter;
};
