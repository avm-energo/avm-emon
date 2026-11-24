#include "EmonWorker.h"

#include <QThread>
#include <libavm-emon/Exceptions.h>

namespace
{
constexpr int kOuterAttempts = 3; // see libavm-emon examples/get_analog_inputs: USB occasionally stalls
}

EmonWorker::EmonWorker(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<avm_emon::SupportedLimits>();
    qRegisterMetaType<avm_emon::CalibratedAnalogInputs>();
    pollTimer_ = new QTimer(this);
    pollTimer_->setSingleShot(true);
    connect(pollTimer_, &QTimer::timeout, this, &EmonWorker::pollOnce);
}

EmonWorker::~EmonWorker()
{
    if (device_)
    {
        try { device_->disconnectDevice(); } catch (...) {}
    }
}

void EmonWorker::schedulePoll(int delayMs)
{
    if (device_ && device_->isConnected())
        pollTimer_->start(delayMs);
}

void EmonWorker::connectDevice(const QString &port, bool usb)
{
    emit busy(true);
    try
    {
        device_ = std::make_unique<avm_emon::Device>(
            port, usb ? avm_emon::LinkType::Usb : avm_emon::LinkType::Rs232);
        device_->connectDevice();
        emit log(QStringLiteral("Connected to %1 (%2).").arg(port, usb ? "USB" : "RS-232"));
        const avm_emon::DeviceInfo info = device_->getDeviceInfo();
        limits_ = device_->getSupportedLimits();
        emit deviceInfo(QStringLiteral("%1, s/n %2, SW %3").arg(info.deviceType).arg(info.serialNumber).arg(info.swVersion));
        emit supportedLimits(limits_);
        emit connectedChanged(true);
        schedulePoll(0);
    } catch (const avm_emon::Exception &e)
    {
        emit log(QStringLiteral("ERROR connect: %1").arg(e.message()));
        if (device_)
        {
            try { device_->disconnectDevice(); } catch (...) {}
            device_.reset();
        }
        emit connectedChanged(false);
    }
    emit busy(false);
}

void EmonWorker::disconnectDevice()
{
    if (inPoll_)
    {
        pendingDisconnect_ = true;
        return;
    }
    doDisconnect();
}

void EmonWorker::shutdown()
{
    pendingShutdown_ = true;
    if (!inPoll_)
        runPending();
}

void EmonWorker::doDisconnect()
{
    pollTimer_->stop();
    if (!device_)
        return;
    emit busy(true);
    try { device_->disconnectDevice(); } catch (const avm_emon::Exception &e)
    {
        emit log(QStringLiteral("ERROR disconnect: %1").arg(e.message()));
    }
    device_.reset();
    emit log(QStringLiteral("Disconnected."));
    emit connectedChanged(false);
    emit busy(false);
}

void EmonWorker::setLimits(quint16 currentType, quint16 currentIndex, quint16 voltageIndex)
{
    if (inPoll_)
    {
        pendingLimits_ = Limits { currentType, currentIndex, voltageIndex };
        return;
    }
    doSetLimits(currentType, currentIndex, voltageIndex);
}

void EmonWorker::doSetLimits(quint16 currentType, quint16 currentIndex, quint16 voltageIndex)
{
    if (!device_)
        return;
    pollTimer_->stop();
    emit busy(true);
    try
    {
        const avm_emon::AppliedLimits a = device_->setLimits(currentType, currentIndex, voltageIndex);
        emit log(QStringLiteral("Limits applied: current type %1 index %2, voltage index %3.")
                     .arg(a.currentLimitType).arg(a.currentLimitIndex).arg(a.voltageLimitIndex));
    } catch (const avm_emon::Exception &e)
    {
        emit log(QStringLiteral("ERROR setLimits: %1").arg(e.message()));
    }
    emit busy(false);
    schedulePoll(0);
}

void EmonWorker::runPending()
{
    if (pendingShutdown_ || pendingDisconnect_)
    {
        pendingDisconnect_ = false;
        pendingLimits_.reset();
        doDisconnect();
        if (pendingShutdown_)
            QThread::currentThread()->quit();
        return;
    }
    if (pendingLimits_)
    {
        const Limits l = *pendingLimits_;
        pendingLimits_.reset();
        doSetLimits(l.type, l.cur, l.volt);
    }
}

void EmonWorker::pollOnce()
{
    if (!device_ || !device_->isConnected())
        return;
    inPoll_ = true;
    QString lastError;
    bool ok = false;
    for (int attempt = 0; attempt < kOuterAttempts && !ok; ++attempt)
    {
        if (pendingShutdown_ || pendingDisconnect_ || pendingLimits_)
            break; // don't keep retrying when the user already asked for something else
        try
        {
            const avm_emon::AnalogInputs raw = device_->getAnalogInputs();
            // The ranges are real measurement ranges (manual, 4.2.3), not a transformer ratio, so the
            // reported RMS values are already in amps/volts: no scaling by the selected range.
            avm_emon::CalibratedAnalogInputs v;
            v.Ia = raw.Ia; v.Ib = raw.Ib; v.Ic = raw.Ic;
            v.Ua = raw.Ua; v.Ub = raw.Ub; v.Uc = raw.Uc;
            v.frequency = raw.frequency;
            v.phiIaUa = raw.phiIaUa; v.phiIbUb = raw.phiIbUb; v.phiIcUc = raw.phiIcUc;
            v.phiUaUb = raw.phiUaUb; v.phiUbUc = raw.phiUbUc; v.phiUcUa = raw.phiUcUa;
            emit measured(v, raw.appliedCurrentLimitType,
                raw.appliedCurrentLimitIndex, raw.appliedVoltageLimitIndex);
            ok = true;
        } catch (const avm_emon::Exception &e)
        {
            lastError = e.message();
        }
    }
    inPoll_ = false;
    if (pendingShutdown_ || pendingDisconnect_ || pendingLimits_)
    {
        runPending();
        return;
    }
    if (ok)
        schedulePoll(0);
    else
    {
        emit log(QStringLiteral("ERROR getAnalogInputs (%1 attempts): %2").arg(kOuterAttempts).arg(lastError));
        schedulePoll(1000);
    }
}
