#pragma once

#include "engine/core/Types.h"
#include "engine/thread/AtomicUtil.h"

class AtomicInt
{
public:
	inline AtomicInt();
	explicit inline AtomicInt(I32 value);

	inline I32 GetValue() const;

	inline AtomicInt& operator=(I32 value);
	inline AtomicInt& operator=(const AtomicInt& value);

	inline AtomicInt operator+(I32 value) const;
	inline AtomicInt operator-(I32 value) const;
	inline AtomicInt operator+(const AtomicInt& value) const;
	inline AtomicInt operator-(const AtomicInt& value) const;

	inline AtomicInt& operator+=(I32 value);
	inline AtomicInt& operator-=(I32 value);
	inline AtomicInt& operator+=(const AtomicInt& value);
	inline AtomicInt& operator-=(const AtomicInt& value);

	inline AtomicInt& operator++();
	inline AtomicInt operator++(I32);
	inline AtomicInt& operator--();
	inline AtomicInt operator--(I32);

private:
	volatile I32 m_value;
};

inline AtomicInt::AtomicInt()
	:m_value(0)
{
}

inline
AtomicInt::AtomicInt(I32 value)
	:m_value(value)
{
}

inline 
I32 AtomicInt::GetValue() const 
{ 
	return m_value; 
}

inline
AtomicInt& AtomicInt::operator=(I32 value)
{
	AtomicSet(&m_value, value);
	return *this;
}

inline
AtomicInt& AtomicInt::operator=(const AtomicInt& value)
{
	AtomicSet(&m_value, value.m_value);
	return *this;
}

inline
AtomicInt AtomicInt::operator+(I32 value) const
{
	AtomicInt copy(m_value);
	copy += value;
	return copy;
}

inline
AtomicInt AtomicInt::operator-(I32 value) const
{
	AtomicInt copy(m_value);
	copy -= value;
	return copy;
}

inline
AtomicInt AtomicInt::operator+(const AtomicInt& value) const
{
	AtomicInt copy(m_value);
	copy += value;
	return copy;
}

inline
AtomicInt AtomicInt::operator-(const AtomicInt& value) const
{
	AtomicInt copy(m_value);
	copy -= value;
	return copy;
}

inline
AtomicInt& AtomicInt::operator++()
{
	AtomicIncr(&m_value);
	return *this;
}

inline
AtomicInt AtomicInt::operator++(I32)
{
	AtomicInt old = *this;
	operator++();
	return old;
}

inline
AtomicInt& AtomicInt::operator--()
{
	AtomicDecr(&m_value);
	return *this;
}

inline
AtomicInt AtomicInt::operator--(I32)
{
	AtomicInt old = *this;
	operator--();
	return old;
}

inline
AtomicInt& AtomicInt::operator+=(I32 value)
{
	AtomicAdd(&m_value, value);
	return *this;
}

inline
AtomicInt& AtomicInt::operator-=(I32 value)
{
	AtomicAdd(&m_value, -value);
	return *this;
}

inline
AtomicInt& AtomicInt::operator+=(const AtomicInt& value)
{
	AtomicAdd(&m_value, value.m_value);
	return *this;
}

inline
AtomicInt& AtomicInt::operator-=(const AtomicInt& value)
{
	AtomicAdd(&m_value, -value.m_value);
	return *this;
}
