#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QSerialMonitor.h"
#include <QSerialPort>

class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;
class ChartWidget;
class QTextEdit;

class QSerialMonitor : public QMainWindow
{
    Q_OBJECT

public:
    QSerialMonitor(QWidget *parent = nullptr);
    ~QSerialMonitor();

private:
    void buildUi();
    void appendLog(const QString& message);

signals:
    void requestClosePort();
    void requestOpenPort(const QString& portName,
        qint32 baudRate,
        QSerialPort::DataBits dataBits,
        QSerialPort::Parity parity,
        QSerialPort::StopBits stopBits);

private slots:
    void refreshPorts();
    void openOrClosePort();

private:
    QComboBox* m_portBox = nullptr;
    QComboBox* m_baudBox = nullptr;
    QComboBox* m_dataBitsBox = nullptr;
    QComboBox* m_parityBox = nullptr;
    QComboBox* m_stopBitsBox = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_openButton = nullptr;

    QLineEdit* m_commandEdit = nullptr;
    QPushButton* m_sendButton = nullptr;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_countLabel = nullptr;
    QLabel* m_lastValueLabel = nullptr;

    ChartWidget* m_chart = nullptr;
    QTextEdit* m_logEdit = nullptr;

    bool m_isOpened = false;

    Ui::QSerialMonitorClass ui;
};

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget* parent = nullptr);
    void addPoint(double value);
    void clear();

protected:
    void paintEvent(QPaintEvent* event);

private:
    QVector<double> m_points;
    int m_maxPoints = 120;
};

