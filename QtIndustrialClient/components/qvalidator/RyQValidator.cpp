#include "RyQValidator.h"
#include <QDoubleValidator>
#include <QLineEdit>

RyQDoubleValidator::RyQDoubleValidator(double bottom, double top, int decimals, bool fixFieldWidth, QObject* parent)
	: QDoubleValidator(bottom, top, decimals, parent)
	  , _decimals(decimals)
	  , _fixFieldWidth(fixFieldWidth)
{
	if (parent && _fixFieldWidth)
	{
		if (auto edit = dynamic_cast<QLineEdit*>(parent))
		{
			connect(edit, &QLineEdit::editingFinished, this, [this, edit]
			{
				edit->blockSignals(true);
				QString text = edit->text();
				this->fixup(text);
				edit->setText(text);
				edit->blockSignals(false);
			});
		}
	}
}

RyQDoubleValidator::RyQDoubleValidator(int decimals, bool fixFiledWidth, QObject* parent)
	: RyQDoubleValidator(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), decimals,
	                     fixFiledWidth, parent)
{
}

QValidator::State RyQDoubleValidator::validate(QString& input, int& pos) const
{
	if (input.isEmpty())
	{
		return QValidator::Intermediate;
	}
	bool isOK = false;
	if (input.length() == 1 && input.at(0) == '-' && bottom() < 0)
	{
		return QValidator::Acceptable;
	}
	const double value = input.toDouble(&isOK);
	if (!isOK)
	{
		return QValidator::Invalid;
	}
	const int dotPos = input.indexOf(".");
	if (dotPos > 0)
	{
		if (input.right(input.length() - (dotPos + 1)).length() > decimals())
		{
			return QValidator::Invalid;
		}
	}
	if (value >= bottom() && value <= top())
	{
		return QValidator::Acceptable;
	}

	return QValidator::Intermediate;
}

void RyQDoubleValidator::fixup(QString& input) const
{
	bool         ok;
	const double inputValue = input.toDouble(&ok);
	if (!ok)
	{
		if (m_bValueCanIsNull) return;
		if (m_isExistDefaultValue && m_defaultValue > bottom() && m_defaultValue < top())
		{
			input = QString::number(m_defaultValue, 'f', _decimals);
		}
		else
		{
			input = QString::number(bottom(), 'f', _decimals);
		}
		return;
	}

	double fixedValue = inputValue;

	if (fixedValue > top())
	{
		fixedValue = top();
	}
	else if (fixedValue < bottom() || input.trimmed().isEmpty())
	{
		fixedValue = bottom();
	}

	if (_fixFieldWidth)
	{
		// 格式化为字符串，最多 _decimals 位精度，使用 'f' 再去除多余部分
		QString result = QString::number(fixedValue, 'f', _decimals);

		// 如果末尾是 .0 或 .00 ... 进行去除
		if (result.contains('.'))
		{
			// 去掉末尾的0
			result = result.remove(QRegularExpression("0+$"));
			// 如果去掉0后末尾是.，也移除
			if (result.endsWith('.'))
			{
				result.chop(1);
			}
		}

		input = result;
	}
	else
	{
		// 允许非定长格式，直接用 'g'
		input = QString::number(fixedValue, 'f', _decimals);
	}
}

void RyQDoubleValidator::setDefaultValue(double defaultValue, bool isExistDefaultValue)
{
	m_defaultValue        = defaultValue;
	m_isExistDefaultValue = isExistDefaultValue;
}

void RyQDoubleValidator::setValueCanIsNull(bool enable)
{
	bool m_bValueCanIsNull = enable;
}


#if 0
void RyQDoubleValidator::fixup(QString& input) const
{
	//转换失败，默认最小值
	bool         ok;
	const double inputValue = input.toDouble(&ok);
	if (!ok)
	{
		input = QString::number(bottom(), 'f', _decimals);
		return;
	}

	// 如果输入的值大于上限，则设置为上限
	if (inputValue > top())
	{
		input = QString::number(top(), 'f', _decimals);
	}
	// 如果输入的值小于下限或输入的内容为空，则设置为下限
	else if (inputValue < bottom() || input.trimmed().isEmpty())
	{
		input = QString::number(bottom(), 'f', _decimals);
	}
	// 如果输入在有效范围内，则按照有效位数显示
	else if (_fixFieldWidth)
	{
		input = QString::number(inputValue, 'f', _decimals);
	}
}
#endif

// QValidator::State IntValidator::validate(QString &input, int &pos) const {
//  return QValidator::validate(input,pos);
// }


RyQIntValidator::RyQIntValidator(int bottom, int top, QObject* parent) :
	QIntValidator(bottom, top, parent)
{
	if (parent)
	{
		if (auto edit = dynamic_cast<QLineEdit*>(parent))
		{
			connect(edit, &QLineEdit::editingFinished, this, [this, edit]
			{
				edit->blockSignals(true);
				QString text = edit->text();
				this->fixup(text);
				edit->setText(text);
				edit->blockSignals(false);
			});
		}
	}
}


void RyQIntValidator::fixup(QString& input) const
{
	//转换失败，默认最小值
	bool         ok;
	const double inputValue = input.toInt(&ok);
	if (!ok)
	{
		if (m_bValueCanIsNull) return;

		if (m_isExistDefaultValue && m_defaultValue > bottom() && m_defaultValue < top())
		{
			input = QString::number(m_defaultValue);
		}
		else
		{
			input = QString::number(bottom());
		}
		return;
	}
	if (input.toInt() > top())
	{
		input = QString::number(top());
	}
	else if (input.toInt() < bottom() || input.trimmed().isEmpty())
	{
		input = QString::number(bottom());
	}
}

void RyQIntValidator::setDefaultValue(int defaultValue, bool isExistDefaultValue)
{
	m_defaultValue        = defaultValue;
	m_isExistDefaultValue = isExistDefaultValue;
}

void RyQIntValidator::setValueCanIsNull(bool enable)
{
	m_bValueCanIsNull = enable;
}
