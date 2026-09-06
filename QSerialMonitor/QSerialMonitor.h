#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QSerialMonitor.h"
#include <QSerialPort>
#include <QThread>
#include "SerialWorker.h"

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
    void setupWorker();
    void appendLog(const QString& message);
    bool parseValue(const QString& line, double* value) const;

signals:
    void requestClosePort();
    void requestOpenPort(const QString& portName,
        qint32 baudRate,
        QSerialPort::DataBits dataBits,
        QSerialPort::Parity parity,
        QSerialPort::StopBits stopBits);
    void requestWriteText(const QString& text);

private slots:
    void refreshPorts();
    void openOrClosePort();
    void sendCommand();
    void onPortOpened();
    void onPortClosed();
    void onRawReceived(const QByteArray& data);
    void onLineReceived(const QString& line);
    void onErrorOccurred(const QString& message);

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

    QThread m_workerThread;
    SerialWorker* m_worker = nullptr;
    bool m_opened = false;
    int m_lineCount = 0;

   // Ui::QSerialMonitorClass ui;
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

