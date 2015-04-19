#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <vector>
#include "global.h"
#include "singleton.h"

class GameObject;

typedef std::vector<GameObject*> dbContainer;


class Database : public Singleton <Database>
{
public:

	Database( void );
	~Database( void );

	void Store( GameObject & object );
	void Remove( objectID id );
	GameObject* Find( objectID id );

	objectID GetNewObjectID( void );

private:

	objectID nextFreeID;

	//Make this a more efficient data structure (like a hash table)
	dbContainer database;

};


#endif	// __DATABASE_H__