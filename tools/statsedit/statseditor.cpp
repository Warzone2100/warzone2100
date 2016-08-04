#include <QSettings>
#include <QFile>

#include "statseditor.h"
#include "ui_statseditor.h"

static void loadBodies(QStandardItemModel &model, QString path)
{
	QSettings ini(path, QSettings::IniFormat);
	if (ini.status() != QSettings::NoError)
	{
		qFatal("%s could not be opened", path.toUtf8().constData());
	}
	QStringList list = ini.childGroups();
	model.setRowCount(list.size());
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		model.setData(model.index(i, 0, QModelIndex()), ini.value("name").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 1, QModelIndex()), ini.value("size").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 2, QModelIndex()), ini.value("buildPower", 0).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 3, QModelIndex()), ini.value("buildPoints", 0).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 4, QModelIndex()), ini.value("weight").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 5, QModelIndex()), ini.value("hitpoints").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 6, QModelIndex()), ini.value("model").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 7, QModelIndex()), ini.value("weaponSlots").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 8, QModelIndex()), ini.value("powerOutput").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 9, QModelIndex()), ini.value("armourKinetic").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 10, QModelIndex()), ini.value("armourHeat").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 11, QModelIndex()), ini.value("flameModel").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 12, QModelIndex()), ini.value("designable", false).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 13, QModelIndex()), ini.value("droidType", "DROID").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 14, QModelIndex()), list[i], Qt::DisplayRole);
		ini.endGroup();
	}
}

static void loadPropulsions(QStandardItemModel &model, QString path)
{
	QSettings ini(path, QSettings::IniFormat);
	if (ini.status() != QSettings::NoError)
	{
		qFatal("%s could not be opened", path.toUtf8().constData());
	}
	QStringList list = ini.childGroups();
	model.setRowCount(list.size());
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		if (ini.contains("name"))
		{
			model.setData(model.index(i, 0, QModelIndex()), ini.value("name").toString(), Qt::DisplayRole);
		}
		else
		{
			model.setData(model.index(i, 0, QModelIndex()), list[i], Qt::DisplayRole);
		}
		model.setData(model.index(i, 1, QModelIndex()), ini.value("buildPower").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 2, QModelIndex()), ini.value("buildPoints").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 3, QModelIndex()), ini.value("weight").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 4, QModelIndex()), ini.value("hitpoints").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 5, QModelIndex()), ini.value("model").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 6, QModelIndex()), ini.value("type").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 7, QModelIndex()), ini.value("speed").toString(), Qt::DisplayRole);
		model.setData(model.index(i, 8, QModelIndex()), ini.value("designable", false).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 9, QModelIndex()), ini.value("skidDeceleration", 600).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 10, QModelIndex()), ini.value("spinSpeed", 8192/45*3/4).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 11, QModelIndex()), ini.value("turnSpeed", 8192/45/3).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 12, QModelIndex()), ini.value("Acceleration", 250).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 13, QModelIndex()), ini.value("Deceleration", 800).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 14, QModelIndex()), ini.value("spinAngle", 180).toString(), Qt::DisplayRole);
		model.setData(model.index(i, 15, QModelIndex()), list[i], Qt::DisplayRole);
		ini.endGroup();
	}
}

StatsEditor::StatsEditor(QString path, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::StatsEditor),
	mPath(path)
{
	ui->setupUi(this);
	connect(ui->actionQuit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

	// Load bodies
	mBodyModel.setColumnCount(15);
	mBodyModel.setHeaderData(0, Qt::Horizontal, QString("Name"));
	mBodyModel.setHeaderData(1, Qt::Horizontal, QString("Size"));
	mBodyModel.setHeaderData(2, Qt::Horizontal, QString("BuildPower"));
	mBodyModel.setHeaderData(3, Qt::Horizontal, QString("BuildPoints"));
	mBodyModel.setHeaderData(4, Qt::Horizontal, QString("Weight"));
	mBodyModel.setHeaderData(5, Qt::Horizontal, QString("HP"));
	mBodyModel.setHeaderData(6, Qt::Horizontal, QString("Model"));
	mBodyModel.setHeaderData(7, Qt::Horizontal, QString("Weapons"));
	mBodyModel.setHeaderData(8, Qt::Horizontal, QString("Power"));
	mBodyModel.setHeaderData(9, Qt::Horizontal, QString("Armour"));
	mBodyModel.setHeaderData(10, Qt::Horizontal, QString("Thermal"));
	mBodyModel.setHeaderData(11, Qt::Horizontal, QString("FlameModel"));
	mBodyModel.setHeaderData(12, Qt::Horizontal, QString("Designable"));
	mBodyModel.setHeaderData(13, Qt::Horizontal, QString("DroidType"));
	mBodyModel.setHeaderData(14, Qt::Horizontal, QString("Key"));
	QString bodyPath(path + "/body.ini");
	if (!QFile::exists(bodyPath))
	{
		qFatal("%s not found", bodyPath.toUtf8().constData());
	}
	loadBodies(mBodyModel, bodyPath);
	ui->tableViewBodies->setModel(&mBodyModel);

	// Load propulsions
	mPropModel.setColumnCount(16);
	mPropModel.setHeaderData(0, Qt::Horizontal, QString("Name"));
	mPropModel.setHeaderData(1, Qt::Horizontal, QString("BuildPower"));
	mPropModel.setHeaderData(2, Qt::Horizontal, QString("BuildPoints"));
	mPropModel.setHeaderData(3, Qt::Horizontal, QString("Weight"));
	mPropModel.setHeaderData(4, Qt::Horizontal, QString("Hitpoints"));
	mPropModel.setHeaderData(5, Qt::Horizontal, QString("Model"));
	mPropModel.setHeaderData(6, Qt::Horizontal, QString("Type"));
	mPropModel.setHeaderData(7, Qt::Horizontal, QString("Speed"));
	mPropModel.setHeaderData(8, Qt::Horizontal, QString("Designable"));
	mPropModel.setHeaderData(9, Qt::Horizontal, QString("SkidDeceleration"));
	mPropModel.setHeaderData(10, Qt::Horizontal, QString("SpinSpeed"));
	mPropModel.setHeaderData(11, Qt::Horizontal, QString("TurnSpeed"));
	mPropModel.setHeaderData(12, Qt::Horizontal, QString("Acceleration"));
	mPropModel.setHeaderData(13, Qt::Horizontal, QString("Deceleration"));
	mPropModel.setHeaderData(14, Qt::Horizontal, QString("SpinAngle"));
	mPropModel.setHeaderData(15, Qt::Horizontal, QString("Key"));
	QString propPath(path + "/propulsion.ini");
	if (!QFile::exists(propPath))
	{
		qFatal("%s not found", propPath.toUtf8().constData());
	}
	loadPropulsions(mPropModel, propPath);
	ui->tableViewPropulsions->setModel(&mPropModel);
}

StatsEditor::~StatsEditor()
{
	delete ui;
}
