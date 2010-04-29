#ifndef QHEXSPINBOX_H
#define QHEXSPINBOX_H
#include <QtGui/QSpinBox>

class QHexSpinBox : public QSpinBox
{
public:
	QHexSpinBox(QWidget *parent = 0):
			QSpinBox(parent)
	{
		setDisabled(true);
		setPrefix("0x");
	}
	int valueFromText(const QString &text) const
	{
		bool ok;
		int retVal = text.toInt(&ok, 16);
		if (ok)
			return retVal;
		return 0;
	}

	QString textFromValue(int value) const
	{
		return QString::number(value, 16).toUpper();
	}
	QValidator::State validate(const QString &input, const int&)const
	{
		if (valueFromText(input))
			return QValidator::Acceptable;
		return QValidator::Invalid;
	}
};
#endif // QHEXSPINBOX_H
