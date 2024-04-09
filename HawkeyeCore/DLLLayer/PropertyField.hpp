#pragma once

#include <iostream>

template<typename T>
class PropertyFieldReadOnly
{
public:
	PropertyFieldReadOnly()
	{
		init ({ });
	}

	void init (T* varAddr)
	{
		varAddr_ = varAddr;
	}

	operator T() const
	{
		return *varAddr_;
	}

	T get() const
	{
		return *varAddr_;
	}

protected:
	T* varAddr_;
};

template<typename T>
class PropertyFieldReadStatic : public PropertyFieldReadOnly<T>
{
public:

	PropertyFieldReadStatic()
	{ }

	void operator = (const T &value)
	{
		*(this->varAddr_) = value;
	}
};

template<typename T>
class PropertyField : public PropertyFieldReadOnly<T>
{
public:
	PropertyField()
	{
		init ({ }, nullptr);
	}

	void init (T* addr, std::function<bool(const PropertyField<T>&)> onValueChange)
	{
		PropertyFieldReadOnly<T>::init (addr);
		onValueChange_ = onValueChange;
	}

	void operator = (const T &value)
	{
		if (*varAddr_ == value)
		{
			return;
		}
		*varAddr_ = value;
		triggerValueChange();
	}

private:
	void triggerValueChange()
	{
		if (onValueChange_ != nullptr)
		{
			onValueChange_(*this);
		}
	}

	std::function<bool(const PropertyField<T>&)> onValueChange_;
};
