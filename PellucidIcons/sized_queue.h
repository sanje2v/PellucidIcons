#pragma once
#include <deque>

template<typename T>
class sized_queue : public std::deque<T>
{
private:
	size_t m_maxSize;

public:
	sized_queue(size_t maxSize)
		: std::deque<T>(),
		m_maxSize(maxSize) {}
	virtual ~sized_queue() {}

	void push(const T& value)
	{
		if (std::deque<T>::size() == m_maxSize)
			std::deque<T>::pop_front();

		std::deque<T>::push_back(value);
	}

	void clear()
	{
		while (!std::deque<T>::empty())
			std::deque<T>::pop_front();
	}

	T& front()
	{
		if (std::deque<T>::empty())
			throw std::out_of_range("sized_queue is empty");

		return std::deque<T>::front();
	}

	T& back()
	{
		if (std::deque<T>::empty())
			throw std::out_of_range("sized_queue is empty");

		return std::deque<T>::back();
	}
};