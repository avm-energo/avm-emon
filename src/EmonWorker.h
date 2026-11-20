#pragma once

#include <QObject>
#include <QTimer>
#include <libavm-emon/Calibration.h>
#include <libavm-emon/Device.h>
#include <memory>
#include <optional>

Q_DECLARE_METATYPE(avm_emon::SupportedLimits)
Q_DECLARE_METATYPE(avm_emon::CalibratedAnalogInputs)

// Owns the avm_emon::Device and lives on its own QThread: every call blocks (getAnalogInputs() takes
// ~2.5 s, more over USB when it stalls), so the GUI must never call the device directly.
// All slots run serialised on the worker thread. Measurements are polled in a loop of single-shot
// timers, so queued requests (setLimits, disconnect) are served between two polls.
class EmonWorker : public QObject
{
    Q_OBJECT
public:
    explicit EmonWorker(QObject *parent = nullptr);
    ~EmonWorker() override;

public slots:
    void connectDevice(const QString &port, bool usb);
    void disconnectDevice();
    void setLimits(quint16 currentType, quint16 currentIndex, quint16 voltageIndex);
    // Disconnects and stops the thread this worker lives on (call via a queued connection).
    void shutdown();

signals:
    void log(const QString &line);
    void connectedChanged(bool connected);
    void busy(bool on);
    void deviceInfo(const QString &text);
    void supportedLimits(const avm_emon::SupportedLimits &limits);
    // Calibrated values plus the limit configuration they were measured under.
    void measured(const avm_emon::CalibratedAnalogInputs &values, quint16 currentType, quint16 currentIndex,
        quint16 voltageIndex);

private:
    void pollOnce();
    void doDisconnect();
    void doSetLimits(quint16 currentType, quint16 currentIndex, quint16 voltageIndex);
    void runPending();
    void schedulePoll(int delayMs);

    std::unique_ptr<avm_emon::Device> device_;
    avm_emon::SupportedLimits limits_;
    QTimer *pollTimer_ = nullptr;

    // Transport::request() spins a nested QEventLoop, which would run our queued slots *inside* a
    // device call (destroying the transport under it, or interleaving two requests on one port).
    // So while a poll is in flight, requests are only recorded and executed once it has returned.
    bool inPoll_ = false;
    bool pendingDisconnect_ = false, pendingShutdown_ = false;
    struct Limits { quint16 type, cur, volt; };
    std::optional<Limits> pendingLimits_;
};
