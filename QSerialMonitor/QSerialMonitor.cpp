#include "QSerialMonitor.h"

#include <QSerialPort>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>

QSerialMonitor::QSerialMonitor(QWidget *parent)
    : QMainWindow(parent)
{
    //ui.setupUi(this);
    buildUi();
}

QSerialMonitor::~QSerialMonitor()
{}

void QSerialMonitor::buildUi()
{
    QWidget* centerWidget = new QWidget(this);

    QGridLayout* gridLayout = new QGridLayout(centerWidget);
    gridLayout->setColumnStretch(0, 0);
    gridLayout->setColumnStretch(1, 1);

    QGroupBox* configBox = new QGroupBox(QStringLiteral("串口配置"), centerWidget);
    QFormLayout* configLayout = new QFormLayout(configBox);

    m_portBox = new QComboBox(configBox);
    m_baudBox = new QComboBox(configBox);
    m_dataBitsBox = new QComboBox(configBox);
    m_parityBox = new QComboBox(configBox);
    m_stopBitsBox = new QComboBox(configBox);
    m_refreshButton = new QPushButton(QStringLiteral("刷新串口"), configBox);
    m_openButton = new QPushButton(QStringLiteral("打开串口"), configBox);

    QList<int> baudRates = { 9600, 19200, 38400, 57600, 115200, 230400 };
    for (int baud : baudRates)
    {
        m_baudBox->addItem(QString::number(baud), baud);
    }
    m_baudBox->setCurrentText(QStringLiteral("115200"));

    m_dataBitsBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
    m_dataBitsBox->addItem(QStringLiteral("8"), QSerialPort::Data8);

    m_parityBox->addItem(QStringLiteral("None"), QSerialPort::NoParity);
    m_parityBox->addItem(QStringLiteral("Even"), QSerialPort::EvenParity);
    m_parityBox->addItem(QStringLiteral("Odd"), QSerialPort::OddParity);

    m_stopBitsBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
    m_stopBitsBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    configLayout->addRow(QStringLiteral("端口"), m_portBox);
    configLayout->addRow(QStringLiteral("波特率"), m_baudBox);
    configLayout->addRow(QStringLiteral("数据位"), m_dataBitsBox);
    configLayout->addRow(QStringLiteral("校验位"), m_parityBox);
    configLayout->addRow(QStringLiteral("停止位"), m_stopBitsBox);
    configLayout->addRow(m_refreshButton);
    configLayout->addRow(m_openButton);

    QGroupBox* sendBox = new QGroupBox(QStringLiteral("命令发送"), centerWidget);
    QVBoxLayout* sendLayout = new QVBoxLayout(sendBox);

    m_commandEdit = new QLineEdit(sendBox);
    m_commandEdit->setPlaceholderText(QStringLiteral("例如：START 或 SET:100"));
    m_sendButton = new QPushButton(QStringLiteral("发送"), sendBox);

    sendLayout->addWidget(m_commandEdit);
    sendLayout->addWidget(m_sendButton);

    QGroupBox* statusBox = new QGroupBox(QStringLiteral("运行状态"), centerWidget);
    QFormLayout* statusLayout = new QFormLayout(statusBox);

    m_statusLabel = new QLabel(QStringLiteral("未连接"), statusBox);
    m_countLabel = new QLabel(QStringLiteral("0"), statusBox);
    m_lastValueLabel = new QLabel(QStringLiteral("--"), statusBox);

    statusLayout->addRow(QStringLiteral("串口状态"), m_statusLabel);
    statusLayout->addRow(QStringLiteral("接收行数"), m_countLabel);
    statusLayout->addRow(QStringLiteral("最新数值"), m_lastValueLabel);

    QVBoxLayout* leftLayout = new QVBoxLayout(centerWidget);
    leftLayout->addWidget(configBox);
    leftLayout->addWidget(sendBox);
    leftLayout->addWidget(statusBox);
    leftLayout->addStretch();

    QVBoxLayout* rightLayout = new QVBoxLayout(centerWidget);
    m_chart = new ChartWidget(centerWidget);
    m_logEdit = new QTextEdit(centerWidget);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(220);
    rightLayout->addWidget(m_chart, 3);
    rightLayout->addWidget(m_logEdit, 2);

    gridLayout->addLayout(leftLayout,0,0);
    gridLayout->addLayout(rightLayout, 0, 1);
    setCentralWidget(centerWidget);

    connect(m_refreshButton, &QPushButton::clicked, this, &QSerialMonitor::refreshPorts);
    connect(m_openButton, &QPushButton::clicked, this, &QSerialMonitor::openOrClosePort);

}

void QSerialMonitor::refreshPorts()
{
    const QString current = m_portBox->currentText();
    m_portBox->clear();
    QList<QSerialPortInfo> m_listPortInfo = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : m_listPortInfo)
    {
        m_portBox->addItem(info.portName());
    }

    const int index = m_portBox->findText(current);
    if (index > 0)
    {
        m_portBox->setCurrentIndex(index);
    }

    appendLog(QStringLiteral("已扫描到%1个串口").arg(m_listPortInfo.size()));
}

void QSerialMonitor::openOrClosePort()
{
    if (m_isOpened)
    {
        emit requestClosePort();
        return;
    }

    if (m_portBox->currentText().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("没有可用串口！"));
        return;
    }

    emit requestOpenPort(m_portBox->currentText(),
        m_baudBox->currentData().toInt(),
        static_cast<QSerialPort::DataBits>(m_dataBitsBox->currentData().toInt()),
        static_cast<QSerialPort::Parity>(m_parityBox->currentData().toInt()),
        static_cast<QSerialPort::StopBits>(m_stopBitsBox->currentData().toInt())
        );
}

void QSerialMonitor::appendLog(const QString& message)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    m_logEdit->append(QStringLiteral("%1，%2").arg(time, message));
}


ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(320);
    setAutoFillBackground(true);
}

void ChartWidget::paintEvent(QPaintEvent* event)
{

}