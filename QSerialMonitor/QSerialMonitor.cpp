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
    setupWorker();
    refreshPorts();
}

QSerialMonitor::~QSerialMonitor()
{
    emit requestClosePort();
    m_workerThread.quit();
    m_workerThread.wait();
}

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
    m_sendButton->setEnabled(false);

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
    connect(m_sendButton, &QPushButton::clicked, this, &QSerialMonitor::sendCommand);
    connect(m_commandEdit, &QLineEdit::returnPressed, this, &QSerialMonitor::sendCommand);
}

void QSerialMonitor::setupWorker()
{
    m_worker = new SerialWorker;
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &QSerialMonitor::requestOpenPort, m_worker, &SerialWorker::openPort);
    connect(this, &QSerialMonitor::requestClosePort, m_worker, &SerialWorker::closePort);
    connect(this, &QSerialMonitor::requestWriteText, m_worker, &SerialWorker::writeText);

    connect(m_worker, &SerialWorker::portOpened, this, &QSerialMonitor::onPortOpened);
    connect(m_worker, &SerialWorker::portClosed, this, &QSerialMonitor::onPortClosed);
    connect(m_worker, &SerialWorker::rawReceived, this, &QSerialMonitor::onRawReceived);
    connect(m_worker, &SerialWorker::lineReceived, this, &QSerialMonitor::onLineReceived);
    connect(m_worker, &SerialWorker::errorOccurred, this, &QSerialMonitor::onErrorOccurred);

    m_workerThread.start();
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
    if (m_opened)
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

void QSerialMonitor::sendCommand()
{
    const QString command = m_commandEdit->text().trimmed();
    if (command.isEmpty()) {
        return;
    }

    emit requestWriteText(command);
    appendLog(QStringLiteral("[TX] %1").arg(command));
    m_commandEdit->clear();
}

void QSerialMonitor::onPortOpened()
{
    m_opened = true;
    m_openButton->setText(QStringLiteral("关闭串口"));
    m_sendButton->setEnabled(true);
    m_statusLabel->setText(QStringLiteral("已连接"));
    m_lineCount = 0;
    m_chart->clear();
    m_countLabel->setText(QStringLiteral("0"));
    m_lastValueLabel->setText(QStringLiteral("--"));
    appendLog(QStringLiteral("串口已打开"));
}

void QSerialMonitor::onPortClosed()
{
    m_opened = false;
    m_openButton->setText(QStringLiteral("打开串口"));
    m_sendButton->setEnabled(false);
    m_statusLabel->setText(QStringLiteral("未连接"));
    appendLog(QStringLiteral("串口已关闭"));
}

void QSerialMonitor::onRawReceived(const QByteArray& data)
{
    appendLog(QStringLiteral("[RX RAW] %1").arg(QString::fromUtf8(data).trimmed()));
}

void QSerialMonitor::onLineReceived(const QString& line)
{
    ++m_lineCount;
    m_countLabel->setText(QString::number(m_lineCount));
    appendLog(QStringLiteral("[RX LINE] %1").arg(line));

    double value = 0.0;
    if (parseValue(line, &value)) {
        m_lastValueLabel->setText(QString::number(value, 'f', 2));
        m_chart->addPoint(value);
    }
}

void QSerialMonitor::onErrorOccurred(const QString& message)
{
    appendLog(QStringLiteral("[ERROR] %1").arg(message));
}

void QSerialMonitor::appendLog(const QString& message)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    m_logEdit->append(QStringLiteral("%1，%2").arg(time, message));
}

bool QSerialMonitor::parseValue(const QString& line, double* value) const
{
    const int splitIndex = line.indexOf(':');
    const QString numberText = splitIndex >= 0 ? line.mid(splitIndex + 1).trimmed() : line.trimmed();

    bool ok = false;
    const double parsed = numberText.toDouble(&ok);
    if (ok && value != nullptr) {
        *value = parsed;
    }

    return ok;
}

ChartWidget::ChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(320);
    setAutoFillBackground(true);
}

void ChartWidget::addPoint(double value)
{
	m_points.append(value);
	if (m_points.size() > m_maxPoints) 
    {
		m_points.removeFirst();
	}
	update();
}

void ChartWidget::clear()
{
    m_points.clear();
    update();
}

void ChartWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

    QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(248, 250, 252));

	const QRect plot = rect().adjusted(48, 24, -24, -42);
	painter.setPen(QPen(QColor(203, 213, 225),1));
    painter.drawRect(plot);

	// 绘制水平网格线，分为5等份
    for (int i = 0; i < 5; ++i)
    {
		const int y = plot.top() + i * plot.height() / 5;
		painter.drawLine(plot.left(), y, plot.right(), y);
    }

    // 在控件上方绘制标题
	painter.setPen(QPen(QColor(71, 85, 105), 1));
    painter.drawText(14,22, QStringLiteral("实时曲线"));

    if (m_points.isEmpty())
    {
        painter.setPen(QColor(100, 116, 139));
		painter.drawText(plot, Qt::AlignCenter, QStringLiteral("暂无数据"));
        return;
    }

    // 计算数据中的最小和最大值；若相等（qFuzzyCompare），将它们扩展 ±1，避免除以 0 和显示平直线。
    const auto [minIt, maxIt] = std::minmax_element(m_points.begin(), m_points.end());
	double minValue = *minIt;
	double maxValue = *maxIt;
    if (qFuzzyCompare(minValue, maxValue))
    {
		minValue -= 1.0;
		maxValue += 1.0;
    }

    // 将 m_points 的每个样本按在绘图区内的 x,y 比例映射为 QPointF，构建曲线路径。xRatio 根据索引均匀分布，yRatio 根据 min/max 归一化
	QPainterPath path;
    for (int i = 0; i < m_points.size(); ++i)
    {
		const double xRatio = static_cast<double>(i) / (m_points.size() - 1);
		const double yRatio = (m_points[i] - minValue) / (maxValue - minValue);
        const QPointF point(plot.left() + xRatio * plot.width(),
            plot.bottom() - yRatio * plot.height());
		if (i == 0)
		{
			path.moveTo(point);
		}
		else
		{
			path.lineTo(point);
		}
    }

    painter.setPen(QPen(QColor(37, 99, 235), 2.5));
    painter.drawPath(path);

    // 在左侧绘制最大/最小数值标签，并在底部显示当前采样点数量
    painter.drawText(8, plot.top() + 12, QString::number(maxValue, 'f', 1));
    painter.drawText(8, plot.bottom(), QString::number(minValue, 'f', 1));
    painter.drawText(plot.left(), height() - 16, QStringLiteral("采样点：%1").arg(m_points.size()));
}