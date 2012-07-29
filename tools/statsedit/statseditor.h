#ifndef STATSEDITOR_H
#define STATSEDITOR_H

#include <QMainWindow>

namespace Ui {
    class StatsEditor;
}

class StatsEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit StatsEditor(QWidget *parent = 0);
    ~StatsEditor();

private:
    Ui::StatsEditor *ui;
};

#endif // STATSEDITOR_H
