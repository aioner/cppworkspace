#pragma once

#ifndef ADAPTER_H 

#define ADAPTER_H 

// 需要被Adapt 的类 
class Target 
{ 
public: 
	Target(){} 
	virtual ~Target() {} 
	virtual void Request() = 0; 
}; 

// 与被Adapt 对象提供不兼容接口的类 
class Adaptee 
{ 
public: 
	Adaptee(){} 
	~Adaptee(){} 
	void SpecialRequest(); 
}; 

// 进行Adapt 的类,采用聚合原有接口类的方式 
class Adapter : public Target 
{ 
public: 
	Adapter(Adaptee* pAdaptee); 
	virtual ~Adapter(); 
	virtual void Request(); 

private: 
	Adaptee* m_pAdptee; 
}; 

#endif 
