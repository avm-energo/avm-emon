#pragma once

#include "EmonWorker.h"

#include <QMainWindow>
#include <QThread>
#include <array>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
namespace avm_vectorwidget { class VectorDiagram; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void requestConnect(const QString &port, bool usb);
    void requestDisconnect();
    void requestSetLimits(quint16 currentType, quint16 currentIndex, quint16 voltageIndex);

private:
    void refreshPorts();
    void fillCurrentCombo(int type);
    void sendLimits();
    void onConnected(bool connected);
    void onSupportedLimits(const avm_emon::SupportedLimits &limits);
    void onMeasured(const avm_emon::CalibratedAnalogInputs &v, quint16 curType, quint16 curIdx, quint16 voltIdx);
    void onBusy(bool on);
    void clearValues();
    void appendLog(const QString &line);
    void updateEnabled();

    QThread thread_;
    EmonWorker *worker_ = nullptr;

    QComboBox *portCombo_, *linkCombo_;
    QPushButton *refreshBtn_, *connectBtn_, *disconnectBtn_;
    std::array<QDoubleSpinBox *, 3> u_, i_, phi_, uu_;
    QDoubleSpinBox *freq_;
    QComboBox *curTypeCombo_, *curRangeCombo_, *voltRangeCombo_;
    avm_vectorwidget::VectorDiagram *diagram_;
    QLabel *infoLabel_, *stateLabel_;
    QPlainTextEdit *log_;

    avm_emon::SupportedLimits limits_;
    bool connected_ = false, busy_ = false, pendingLimits_ = false;
};
