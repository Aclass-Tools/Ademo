/**
 * @author: rayzhang
 */
#pragma once
#include <QValidator>

class RyQDoubleValidator : public QDoubleValidator
{
	Q_OBJECT

public:
	explicit RyQDoubleValidator(double   bottom        = std::numeric_limits<double>::lowest(),
	                            double   top           = std::numeric_limits<double>::max(),
	                            int      decimals      = 15,
	                            bool     fixFieldWidth = false,
	                            QObject* parent        = nullptr);

	explicit RyQDoubleValidator(int decimals, bool fixFiledWidth = false, QObject* parent = nullptr);

	QValidator::State validate(QString& input, int& pos) const override;
	void              fixup(QString& input) const override;

	void setDefaultValue(double defaultValue, bool isExistDefaultValue = true);
	void setValueCanIsNull(bool enable);

private:
	int    _decimals = 0;
	bool   _fixFieldWidth{false};
	bool   m_bValueCanIsNull     = false;
	bool   m_isExistDefaultValue = false;
	double m_defaultValue        = 0;
};

class RyQIntValidator : public QIntValidator
{
	Q_OBJECT

public:
	explicit RyQIntValidator(QObject* parent = nullptr)
		: QIntValidator(std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), parent)
	{
	};


	RyQIntValidator(int bottom, int top, QObject* parent = nullptr);

	// virtual QValidator::State validate(QString &input, int &pos) const;
	void fixup(QString& input) const override;

	void setDefaultValue(int defaultValue, bool isExistDefaultValue = true);

	void setValueCanIsNull(bool enable);

private:
	bool m_bValueCanIsNull     = false;
	bool m_isExistDefaultValue = false;
	int  m_defaultValue        = 0;
};
