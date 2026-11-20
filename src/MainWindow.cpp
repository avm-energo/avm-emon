#include "MainWindow.h"

#include <QColor>
#include <QComboBox>
#include <QPalette>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QStatusBar>
#include <QVBoxLayout>

namespace
{
// Read-only value display styled like the RETOM spin boxes.
QDoubleSpinBox *display(int decimals, const QString &suffix = QString())
{
    auto *s = new QDoubleSpinBox;
    s->setRange(-1e9, 1e9);
    s->setDecimals(decimals);
    s->setSuffix(suffix);
    s->setReadOnly(true);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setFocusPolicy(Qt::NoFocus);
    return s;
}

const char *kGreenBtn = "QPushButton{background:#43a047;color:white;font-weight:bold;border:none;border-radius:4px;"
                        "padding:5px 14px}QPushButton:hover{background:#4caf50}"
                        "QPushButton:disabled{background:#d0d0d0;color:#8a8a8a}";
const char *kRoseBtn = "QPushButton{background:#ec6f9a;color:white;font-weight:bold;border:none;border-radius:4px;"
                       "padding:5px 14px}QPushButton:hover{background:#f080a8}"
                       "QPushButton:disabled{background:#d0d0d0;color:#8a8a8a}";

struct PhaseStyle
{
    const char *badge, *text, *tint;
};
// Phase A yellow, B green, C red (same as avm-retom).
const PhaseStyle kPhase[3] = { { "#f2c200", "#000000", "#fff4bd" },
    { "#2e9e44", "#ffffff", "#d3edd6" },
    { "#d32f2f", "#ffffff", "#f8d0d0" } };

QString rangeText(double v, const char *unit)
{
    return QStringLiteral("%1 %2").arg(v, 0, 'g', 6).arg(QLatin1String(unit));
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("AVM EMON"));
    // Tinted window background so this window is easy to tell apart from its sibling app.
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor("#e1f0e1"));
        pal.setColor(QPalette::Base, QColor("#f1faf1"));   // log, editable combo field, list popups
        pal.setColor(QPalette::Button, QColor("#cfe7cf")); // combo boxes, Refresh
        setPalette(pal);
    }
    // Combo boxes and the plain (non-coloured) buttons: the style ignores the palette for these.
    setStyleSheet(QStringLiteral(
        "QComboBox{background:#cfe7cf;color:#000000;border:1px solid #9bbd9b;border-radius:3px;padding:3px 6px}"
        "QComboBox:disabled{background:#dfeadf;color:#8a8a8a}"
        "QComboBox QAbstractItemView{background:#f1faf1;color:#000000;selection-background-color:#43a047;"
        "selection-color:#ffffff}"
        "QPushButton{background:#cfe7cf;color:#000000;border:1px solid #9bbd9b;border-radius:4px;padding:5px 14px}"
        "QPushButton:hover{background:#bfe0bf}"
        "QPushButton:disabled{background:#dfeadf;color:#8a8a8a}"
        "QGroupBox{background:#d3e8d3;border:1px solid #9bbd9b;border-radius:4px;margin-top:1.2ex;color:#000000}"
        "QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 3px}"
        "QLabel{color:#000000}"
        "QPlainTextEdit{background:#f1faf1;color:#000000;border:1px solid #9bbd9b}"
        "QDoubleSpinBox{background:#f1faf1;color:#000000}"));
    auto *central = new QWidget;
    auto *root = new QVBoxLayout(central);
    setCentralWidget(central);

    // --- Connection ---
    auto *connBox = new QGroupBox(tr("Connection"));
    auto *connL = new QHBoxLayout(connBox);
    portCombo_ = new QComboBox;
    portCombo_->setEditable(true);
    portCombo_->setMinimumWidth(260);
    refreshBtn_ = new QPushButton(tr("Refresh"));
    linkCombo_ = new QComboBox;
    linkCombo_->addItem(QStringLiteral("USB"), true);
    linkCombo_->addItem(QStringLiteral("RS-232"), false);
    linkCombo_->setToolTip(tr("USB: 3 000 000 baud 8N2 (device's own USB port). RS-232: 115200 8N1."));
    connectBtn_ = new QPushButton(tr("Connect"));
    connectBtn_->setStyleSheet(QLatin1String(kGreenBtn));
    disconnectBtn_ = new QPushButton(tr("Disconnect"));
    disconnectBtn_->setStyleSheet(QLatin1String(kRoseBtn));
    stateLabel_ = new QLabel;
    infoLabel_ = new QLabel;
    connL->addWidget(new QLabel(tr("Port:")));
    connL->addWidget(portCombo_, 1);
    connL->addWidget(refreshBtn_);
    connL->addWidget(new QLabel(tr("Link:")));
    connL->addWidget(linkCombo_);
    connL->addWidget(connectBtn_);
    connL->addWidget(disconnectBtn_);
    root->addWidget(connBox);

    // --- Measurements ---
    auto *anaBox = new QGroupBox;
    auto *anaV = new QVBoxLayout(anaBox);
    anaV->addWidget(new QLabel(tr("<b>Measurements</b>")));
    auto *g = new QGridLayout;
    anaV->addLayout(g);
    const char *hdr[] = { "", "U, V (RMS)", "I, A (RMS)", "I/U angle, °" };
    for (int c = 0; c < 4; ++c)
        g->addWidget(new QLabel(tr(hdr[c])), 0, c);
    for (int k = 0; k < 3; ++k)
    {
        u_[k] = display(3);
        i_[k] = display(4);
        phi_[k] = display(2);
        auto *badge = new QLabel(QString(QChar('A' + k)));
        badge->setAlignment(Qt::AlignCenter);
        badge->setMinimumWidth(28);
        badge->setStyleSheet(QStringLiteral("QLabel{background:%1;color:%2;font-weight:bold;border-radius:4px;padding:3px}")
                                 .arg(QLatin1String(kPhase[k].badge), QLatin1String(kPhase[k].text)));
        g->addWidget(badge, k + 1, 0);
        for (QDoubleSpinBox *sb : { u_[k], i_[k], phi_[k] })
            sb->setStyleSheet(QStringLiteral("QDoubleSpinBox{background:%1;color:#000000}")
                                  .arg(QLatin1String(kPhase[k].tint)));
        g->addWidget(u_[k], k + 1, 1);
        g->addWidget(i_[k], k + 1, 2);
        g->addWidget(phi_[k], k + 1, 3);
    }
    freq_ = display(3, tr(" Hz"));
    g->addWidget(new QLabel(tr("Frequency:")), 4, 0);
    g->addWidget(freq_, 4, 1);
    root->addWidget(anaBox);

    // --- Ranges ---
    auto *rangeBox = new QGroupBox(tr("Ranges"));
    auto *rg = new QGridLayout(rangeBox);
    curTypeCombo_ = new QComboBox;
    curTypeCombo_->addItem(tr("Transformer"), 0);
    curTypeCombo_->addItem(tr("Clamp"), 1);
    curTypeCombo_->addItem(tr("UPTT"), 2);
    curRangeCombo_ = new QComboBox;
    voltRangeCombo_ = new QComboBox;
    rg->addWidget(new QLabel(tr("Current sensor:")), 0, 0);
    rg->addWidget(curTypeCombo_, 0, 1);
    rg->addWidget(new QLabel(tr("Current range:")), 1, 0);
    rg->addWidget(curRangeCombo_, 1, 1);
    rg->addWidget(new QLabel(tr("Voltage range:")), 2, 0);
    rg->addWidget(voltRangeCombo_, 2, 1);
    rg->setColumnStretch(2, 1);
    root->addWidget(rangeBox);

    log_ = new QPlainTextEdit;
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(2000);
    root->addWidget(log_, 1);
    statusBar()->setStyleSheet(QStringLiteral("QStatusBar, QStatusBar QLabel{color:#000000}"));
    statusBar()->addWidget(new QLabel(tr("Created by AVM-Energo LLC with Claude, 2026")));
    statusBar()->addPermanentWidget(infoLabel_);
    statusBar()->addPermanentWidget(stateLabel_);

    // --- Worker thread ---
    qRegisterMetaType<avm_emon::SupportedLimits>();
    qRegisterMetaType<avm_emon::CalibratedAnalogInputs>();
    worker_ = new EmonWorker;
    worker_->moveToThread(&thread_);
    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(this, &MainWindow::requestConnect, worker_, &EmonWorker::connectDevice);
    connect(this, &MainWindow::requestDisconnect, worker_, &EmonWorker::disconnectDevice);
    connect(this, &MainWindow::requestSetLimits, worker_, &EmonWorker::setLimits);
    connect(worker_, &EmonWorker::log, this, &MainWindow::appendLog);
    connect(worker_, &EmonWorker::connectedChanged, this, &MainWindow::onConnected);
    connect(worker_, &EmonWorker::busy, this, &MainWindow::onBusy);
    connect(worker_, &EmonWorker::deviceInfo, infoLabel_, &QLabel::setText);
    connect(worker_, &EmonWorker::supportedLimits, this, &MainWindow::onSupportedLimits);
    connect(worker_, &EmonWorker::measured, this, &MainWindow::onMeasured);
    thread_.start();

    connect(refreshBtn_, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(connectBtn_, &QPushButton::clicked, this, [this] {
        emit requestConnect(portCombo_->currentData().toString().isEmpty() ? portCombo_->currentText().trimmed()
                                                                          : portCombo_->currentData().toString(),
            linkCombo_->currentData().toBool());
    });
    connect(disconnectBtn_, &QPushButton::clicked, this, [this] { emit requestDisconnect(); });
    // activated (not currentIndexChanged): only user choices send a command, programmatic updates don't.
    connect(curTypeCombo_, &QComboBox::activated, this, [this] {
        fillCurrentCombo(curTypeCombo_->currentData().toInt());
        sendLimits();
    });
    connect(curRangeCombo_, &QComboBox::activated, this, &MainWindow::sendLimits);
    connect(voltRangeCombo_, &QComboBox::activated, this, &MainWindow::sendLimits);

    refreshPorts();
    onConnected(false);
    resize(760, 620);
}

MainWindow::~MainWindow()
{
    // The worker disconnects (after any poll in flight) and quits its own thread.
    QMetaObject::invokeMethod(worker_, &EmonWorker::shutdown, Qt::QueuedConnection);
    thread_.wait();
}

void MainWindow::refreshPorts()
{
    const QString prev = portCombo_->currentText();
    portCombo_->clear();
    int best = -1;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        const bool ftdi = info.vendorIdentifier() == 0x0403 && info.productIdentifier() == 0x6001;
        portCombo_->addItem(QStringLiteral("%1 — %2%3").arg(info.systemLocation(), info.description(),
                                ftdi ? QStringLiteral(" [FTDI 0403:6001]") : QString()),
            info.systemLocation());
        // RETOM shares the VID/PID; prefer a port that isn't the "Dynamics" (RETOM) one.
        if (ftdi && best < 0 && !info.description().contains(QLatin1String("Dynamics"), Qt::CaseInsensitive))
            best = portCombo_->count() - 1;
    }
    if (best >= 0)
        portCombo_->setCurrentIndex(best);
    else if (portCombo_->count() == 0)
        portCombo_->setEditText(prev.isEmpty() ? QStringLiteral("/dev/ttyUSB0") : prev);
}

void MainWindow::fillCurrentCombo(int type)
{
    curRangeCombo_->clear();
    const auto &list = limits_.currentListFor(static_cast<quint16>(type));
    for (int i = 0; i < list.size(); ++i)
        curRangeCombo_->addItem(rangeText(list[i], "A"), i);
}

void MainWindow::sendLimits()
{
    if (!connected_ || curRangeCombo_->currentIndex() < 0 || voltRangeCombo_->currentIndex() < 0)
        return;
    pendingLimits_ = true; // ignore the applied indices of readings until the device has switched
    emit requestSetLimits(static_cast<quint16>(curTypeCombo_->currentData().toInt()),
        static_cast<quint16>(curRangeCombo_->currentData().toInt()),
        static_cast<quint16>(voltRangeCombo_->currentData().toInt()));
}

void MainWindow::onConnected(bool connected)
{
    connected_ = connected;
    stateLabel_->setText(connected ? tr("connected") : tr("disconnected"));
    if (!connected)
    {
        clearValues();
        infoLabel_->clear();
        curRangeCombo_->clear();
        voltRangeCombo_->clear();
        pendingLimits_ = false;
    }
    updateEnabled();
}

void MainWindow::onSupportedLimits(const avm_emon::SupportedLimits &limits)
{
    limits_ = limits;
    voltRangeCombo_->clear();
    for (int i = 0; i < limits.voltage.size(); ++i)
        voltRangeCombo_->addItem(rangeText(limits.voltage[i], "V"), i);
    fillCurrentCombo(curTypeCombo_->currentData().toInt());
}

void MainWindow::onMeasured(const avm_emon::CalibratedAnalogInputs &v, quint16 curType, quint16 curIdx, quint16 voltIdx)
{
    const double u[3] = { v.Ua, v.Ub, v.Uc }, i[3] = { v.Ia, v.Ib, v.Ic }, phi[3] = { v.phiIaUa, v.phiIbUb, v.phiIcUc };
    for (int k = 0; k < 3; ++k)
    {
        u_[k]->setValue(u[k]);
        i_[k]->setValue(i[k]);
        phi_[k]->setValue(phi[k]);
    }
    freq_->setValue(v.frequency);

    if (pendingLimits_)
        return;
    // Mirror what the device is actually using, without sending anything back.
    if (const int t = curTypeCombo_->findData(curType); t >= 0 && t != curTypeCombo_->currentIndex())
    {
        curTypeCombo_->setCurrentIndex(t);
        fillCurrentCombo(curType);
    }
    if (const int c = curRangeCombo_->findData(curIdx); c >= 0)
        curRangeCombo_->setCurrentIndex(c);
    if (const int vi = voltRangeCombo_->findData(voltIdx); vi >= 0)
        voltRangeCombo_->setCurrentIndex(vi);
}

void MainWindow::onBusy(bool on)
{
    busy_ = on;
    if (!on)
        pendingLimits_ = false; // readings emitted after the setLimits call are fresh
    updateEnabled();
}

void MainWindow::clearValues()
{
    for (int k = 0; k < 3; ++k)
        for (QDoubleSpinBox *sb : { u_[k], i_[k], phi_[k] })
            sb->setValue(0);
    freq_->setValue(0);
}

void MainWindow::updateEnabled()
{
    const bool idle = !busy_;
    connectBtn_->setEnabled(idle && !connected_);
    disconnectBtn_->setEnabled(connected_);
    portCombo_->setEnabled(!connected_);
    refreshBtn_->setEnabled(!connected_);
    linkCombo_->setEnabled(!connected_);
    for (QComboBox *c : { curTypeCombo_, curRangeCombo_, voltRangeCombo_ })
        c->setEnabled(idle && connected_);
    setCursor(busy_ ? Qt::BusyCursor : Qt::ArrowCursor);
}

void MainWindow::appendLog(const QString &line)
{
    log_->appendPlainText(QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), line));
}
