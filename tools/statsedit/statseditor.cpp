#include "statseditor.h"
#include "ui_statseditor.h"

StatsEditor::StatsEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::StatsEditor)
{
	ui->setupUi(this);
}

StatsEditor::~StatsEditor()
{
	delete ui;
}
