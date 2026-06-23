#include "app/SingleInstance.h"

#include <QSharedMemory>

SingleInstance::SingleInstance(const QString &key) {
    m_memory = new QSharedMemory(key);
}

SingleInstance::~SingleInstance() {
    if (m_memory) {
        if (m_memory->isAttached()) {
            m_memory->detach();
        }
        delete m_memory;
    }
}

bool SingleInstance::isAnotherRunning() const {
    if (!m_memory) {
        return false;
    }
    if (m_memory->attach(QSharedMemory::ReadOnly)) {
        m_memory->detach();
        return true;
    }
    return !m_memory->create(1);
}
