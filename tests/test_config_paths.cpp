#include "core/ConfigPaths.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ConfigPathsTest : public QObject {
    Q_OBJECT

private slots:
    void installedBuildUsesAppConfigDirectory();
    void portableBuildUsesWritableExecutableDirectory();
};

void ConfigPathsTest::installedBuildUsesAppConfigDirectory() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(QDir(root.path()).mkpath(QStringLiteral("bin")));

    QFile marker(QDir(root.path()).filePath(QStringLiteral("installed.marker")));
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.close();

    const QString exeDir = QDir(root.path()).filePath(QStringLiteral("bin"));
    const QString appConfigDir = QDir(root.path()).filePath(QStringLiteral("profile-config"));
    QCOMPARE(ConfigPaths::writableDataDir(exeDir, appConfigDir), appConfigDir);
}

void ConfigPathsTest::portableBuildUsesWritableExecutableDirectory() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QCOMPARE(ConfigPaths::writableDataDir(root.path(), QStringLiteral("unused")), root.path());
}

QTEST_MAIN(ConfigPathsTest)
#include "test_config_paths.moc"
