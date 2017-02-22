#pragma once

template<class T>
class Singleton
{
public:
	static T& Instance()
	{
		static T theSingleton;
		return theSingleton;
	}
private:
	Singleton() {};								
	Singleton(Singleton const&) = delete;
	Singleton& operator= (Singleton const&) = delete;
	~Singleton() {};							
};