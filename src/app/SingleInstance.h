#pragma once

class SingleInstance {
public:
    explicit SingleInstance(const QString &key);
    ~SingleInstance();

    bool isAnotherRunning() const;

private:
    class QSharedMemory *m_memory = nullptr;
};
