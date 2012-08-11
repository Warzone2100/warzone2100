#ifndef STATSEDITOR_H
#define STATSEDITOR_H

#include <QMainWindow>
#include <QStandardItemModel>

namespace Ui {
class StatsEditor;
}

class StatsEditor : public QMainWindow
{
	Q_OBJECT

public:
	explicit StatsEditor(QString path, QWidget *parent = 0);
	~StatsEditor();

private:
	Ui::StatsEditor *ui;
	QString mPath;
	QStandardItemModel mBodyModel;
};

#endif // STATSEDITOR_H
