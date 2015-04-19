#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <assert.h>

//Singleton class as authored by Scott Bilas in the book Game Programming Gems

#define NULL 0

template <typename T>
class Singleton
{
public:
	Singleton( void )
	{
		assert( ms_Singleton == NULL );
		int offset = (int)(T*)1 - (int)(Singleton <T> *)(T*)1;
		ms_Singleton = (T*)((int)this + offset);
	}
	~Singleton( void )  {  assert( ms_Singleton != NULL );  ms_Singleton = NULL;  }

	static T&   GetSingleton      ( void )  {  assert( ms_Singleton != NULL );  return ( *ms_Singleton );  }
	static T*   GetSingletonPtr   ( void )  {  return ( ms_Singleton );  }
	static bool DoesSingletonExist( void )  {  return ( ms_Singleton != NULL );  }

private:
	static T* ms_Singleton;

	//SET_NO_COPYING( Singleton <T> );
};

template <typename T> T* Singleton <T>::ms_Singleton = NULL;


#endif	// __SINGLETON_H__