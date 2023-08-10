#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QMediaPlayer>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QAudioOutput>
#include <QThread>
#include <QKeyEvent>
#include <QEvent>

#include <stdlib.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QMessageBox*    msg;
    QMediaPlayer*   player;
    QAudioOutput*   audioOut;
    QString         selectVideo;

    QString format_time(int ms);
    QString get_process_text(QString inTotalDuration, QString inCurrent = "0:0:0");
    int     get_process_percent(qint64 inValue, qint64 inTotal);
    void setPlayerStatus(bool playStatus);
    void init_player();
private:
    Ui::MainWindow *ui;
    bool eventFilter(QObject *obj, QEvent *e) override;
};

class ProcessBarThread : public QThread
{
    Q_OBJECT

public:

protected:
    void run() override;
signals:
    void current_play_duration();
public slots:

};

#endif // MAINWINDOW_H
