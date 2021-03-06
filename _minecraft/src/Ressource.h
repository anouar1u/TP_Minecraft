#pragma once
#include <algorithm>
#include <map>
#include "gl/glew.h"
#include "gl/freeglut.h" 
#include "engine/utils/types_3d.h"





enum TypeRessource
{
	NONE = 0,
	HERBE,
	CROTTE,
	//Cet identifiant doit rester � la fin de l'enumeration
	RESSOURCE_NUM
};


class Ressource
{
protected:
	int MaxQuantity;
	bool m_hasParasite = false;
public:
	TypeRessource Type;
	static const int ILLIMITED_QUANTITY = -1;
	int Quantity;
	NYVert3Df Position;
	NYVert3Df CubePostion;

	Ressource();
	Ressource(NYVert3Df position, int maxQuantity);
	~Ressource();

	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;


	/**
	* Retourne la quantit� manger.
	*/
	virtual int Use(int neededQuantity);

	virtual bool GetHasParasite();
	virtual void SetHasParasite(bool val);
};
