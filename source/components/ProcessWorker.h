// ProcessWorker.h
#pragma once
#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QTimer>

class ProcessWorker : public QObject
{
    Q_OBJECT
public:
    explicit ProcessWorker(const QString &program, const QStringList &args, const QString &workingDir);

signals:
    void outputReady(const QString &text);
    void errorReady(const QString &text);
    void finished(int exitCode);

public slots:
    void start();
    void sendInput(const QString &text);

private slots:
    void onReadyReadOutput();
    void onReadyReadError();
    void flushBuffers();

private:
    QProcess *process{};
    QString program{};
    QStringList args{};
    QString workingDir{};
    QString outputBuffer{};
    QString errorBuffer{};
    QTimer *flushTimer{};
};
