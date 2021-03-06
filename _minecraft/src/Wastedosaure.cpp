#include "Wastedosaure.h"


Wastedosaure::Wastedosaure(NYWorld * _world, NYVert2Df _positionInitiale) :
IABase(_world), m_cone(m_viewAngle, m_viewDistance)
{
	Initialize();//Initialisation de la state machine
	type = WASTEDOSAURE;//Sp�cification du type
	//On set la position initiale
	position = NYVert3Df(_positionInitiale.X * NYCube::CUBE_SIZE,
		_positionInitiale.Y*NYCube::CUBE_SIZE,
		_world->_MatriceHeights[(int)_positionInitiale.X][(int)_positionInitiale.Y] * NYCube::CUBE_SIZE + NYCube::CUBE_SIZE / 2.0f);
	life = 50;//On set la vie
	m_lastUpdate.start();//On lance le timer
}

Wastedosaure::~Wastedosaure()
{

}

//Permet de sp�cifier l'ensemble des entit�s du monde 
void Wastedosaure::SetEntities(std::map<eTypeCreature, CreatureVector> * entities)
{
	m_entities = entities;
}

//R�cup�re l'ensemble des cr�atures attaquables dans le champ de vision.
void Wastedosaure::GetCreaturesInSight()
{
	m_creaturesInSight.clear();
	for (int i = 0; i < CREATURE_NUM; ++i)
	{
		eTypeCreature type = (eTypeCreature)i;
		for (int j = 0; j < (*m_entities)[type].size(); ++j)
		{
			/////DEBUG
			//if (m_cone.IsInSight((*m_entities)[type][j]->position) && //Si j'ai une entity dans mon champ de vision
			//	(*m_entities)[type][j]->GetID() != this->GetID() &&  //Si ce n'est pas moi. On ne sais jamais
			//	type != WASTEDOSAURE)
			//{
			//	cout << (*m_entities)[type][j]->type << endl;
			//}
			/////DEBUG
			if (m_cone.IsInSight((*m_entities)[type][j]->position) && //Si j'ai une entity dans mon champ de vision
				(*m_entities)[type][j]->GetID() != this->GetID() &&  //Si ce n'est pas moi. On ne sais jamais
				type != WASTEDOSAURE &&  //Si ce n'est pas un de mes cong�n�res...
				type != GRIFFONKITU && //...Ni un de mes pr�dateurs...
				type != NEON_BLEU &&
				type != VAUTOUR &&
				type != GEVAULOL &&
				type != MOUCHE &&
				type != PARASITE &&//...Ni les cr�atures aquatiques...
				type != BIXI)
			{
				m_creaturesInSight.push_back((*m_entities)[type][j]);//...On le consid�re dans le champ de vision
			}
		}
	}

}

//Renvoie le chemin de l'entit�
Path Wastedosaure::GetPath()
{
	return m_path;
}

//Renvoie vrai si l'entit� a un chemin qu'elle peut suivre.
bool Wastedosaure::HasAPath()
{
	return m_path.GetSize() > 0;
}

//Renvoie le type de cube sur lequel se trouve une entit�.
NYCubeType Wastedosaure::GetCubeUnderType(IABase * target)
{
	//On r�cup�re la bonne position et on renvoie le type du cube � la position trouv�e.
	NYVert3Df positionCube = target->position / NYCube::CUBE_SIZE;
	positionCube.Z = m_world->_MatriceHeights[(int)positionCube.X][(int)positionCube.Y];
	return m_world->getCube(positionCube.X, positionCube.Y, positionCube.Z)->_Type;
}

//Renvoie le type de cube � une position X,Y donn�e.
NYCubeType Wastedosaure::GetCubeUnderType(NYVert2Df position)
{
	return m_world->getCube(position.X, position.Y, m_world->_MatriceHeights[(int)position.X][(int)position.Y])->_Type;
}

//Trouve le cube d'eau le plus proche de l'entit�.
//On parcours tous les cubes pour trouver le cube d'eau le plus proche.
//A appler qu'une seule fois dans le OnEnter du State_Suicide
NYVert3Df Wastedosaure::FindClosestCubeWater()
{
	NYVert3Df offset;
	float lenght = 10000000.0f;
	float tmpLenght = 0.0f;
	NYCube * tmpCube = NULL;
	for (int x = 0; x<MAT_SIZE_CUBES; x++)
	{
		for (int y = 0; y<MAT_SIZE_CUBES; y++)
		{
			for (int z = 0; z<MAT_HEIGHT_CUBES; z++)
			{
				tmpCube = m_world->getCube(x, y, z);
				if (tmpCube->_Type == CUBE_EAU && m_world->getCube(x, y, z + 1)->_Type == CUBE_AIR)
				{
					offset = position - NYVert3Df(x, y, z)*NYCube::CUBE_SIZE;
					tmpLenght = offset.getMagnitude();
					if (tmpLenght < lenght)
					{
						lenght = tmpLenght;
						m_cubeWater = NYVert3Df(x, y, z);
					}
				}
			}
		}
	}

	return m_cubeWater;
}

//Permet de mettre � jour l'IA
void Wastedosaure::UpdateIA()
{
	Update();//Update the state machine

	//Si on est pas mort
	if (m_currentState != STATE_Dead)
	{
		//Update Cone de vision
		m_cone.SetPosition(position);
		m_cone.SetOrientation(direction);

		//Update Creatures in sight
		GetCreaturesInSight();

		//Updatde des timers 
		UpdateTimers();

		//Si le leader est abscent, on va faire un AssignToAGroup pour d�signer le nouveau leader et r�organiser le groupe
		if (leader != NULL && leader->m_currentState == STATE_Dead)
		{
			WastedosaureManager::GetSingleton()->AssignToAGroup(this);
		}

		//Si On a une cr�ature dans notre champ de vision
		if (m_creaturesInSight.size() > 0 &&
			target == NULL && //Que l'on a pas de cible
			m_currentState != STATE_Reproduction && //Que l'on est pas dans les states ci dessous
			m_currentState != STATE_Suicide &&
			m_currentState != STATE_Dead &&
			m_currentSize >= m_maxSize / 2.0f && //Qu'on est assez grand pour attaquer
			GetCubeUnderType(m_creaturesInSight[0]) != NYCubeType::CUBE_EAU) //Et que le cr�ature que l'on veut attaquer ne se situe pas sur de l'eau.
		{
			//cout << "Creature in sight\n";
			WastedosaureManager::GetSingleton()->PrepareAttack(m_creaturesInSight[0]); //On pr�pare une attaque
		}

		//On met � jour la taille du Wastedosaure
		m_currentSize = (1 - (m_timerGrow / m_durationGrow))*m_minSize + (m_timerGrow / m_durationGrow)*m_maxSize;
		m_currentSize = min(m_currentSize, m_maxSize);
		m_timerGrow += m_lastUpdate.getElapsedSeconds();
	}

	m_lastUpdate.start();//Update du timer

}


void Wastedosaure::UpdateTimers()
{
	//Si on est en mode Wander ( Move ou find path)
	if (m_currentState == STATE_Move || m_currentState == STATE_FindPath)
	{
		if (m_timerWandering >= m_durationWandering)//On met � jour les timers
		{
			PushState(STATE_Reproduction);
			m_timerWandering = 0.0f;
			m_timerReproduction = 0.0f;
			m_timerAttack = 0.0f;
		}
		m_timerWandering += m_lastUpdate.getElapsedSeconds();
	}
	else if (m_currentState == STATE_Reproduction)//Idem lorsque l'on est dans l'etat reproduction
	{
		if (m_timerReproduction >= m_durationReproduction)
		{
			PushState(STATE_Move);
			m_timerReproduction = 0.0f;
			m_timerWandering = 0.0f;
			m_timerAttack = 0.0f;
		}
		m_timerReproduction += m_lastUpdate.getElapsedSeconds();
	}
	else if (m_currentState == STATE_Attack)//Et l'etat attaque
	{
		if (m_timerAttack >= m_durationAttack)
		{
			PushState(STATE_Reproduction);
			m_timerAttack = 0.0f;
			m_timerWandering = 0.0f;
			m_timerReproduction = 0.0f;
			//target = NULL;
		}
		m_timerAttack += m_lastUpdate.getElapsedSeconds();
	}


}

//Renvoie l'index du waypoint le plus proche.
//On parcours tous les WP d'un path pour trouver le plus proche
int Wastedosaure::FindClosestWaypoint(Path _path)
{
	int record = MAXINT;
	int index = 0;
	for (int i = 0; i < _path.GetSize(); ++i)
	{
		NYVert3Df diff = _path.GetWaypoint(i) / NYCube::CUBE_SIZE - position / NYCube::CUBE_SIZE;
		float lenght = diff.getSize();
		if (lenght < record)
		{
			record = lenght;
			index = i;
		}
	}

	return index;
}

//Affiche le Wastedosaure
void Wastedosaure::Draw()
{
	if (m_currentState == STATE_Reproduction)
	{
		glColor3f(0.0f, 0.0f, 1.0f);
	}
	else if (m_currentState == STATE_Attack)
	{
		glColor3f(0.7f, 0.2, 0.8);
	}
	else if (m_currentState == STATE_Dead)
	{
		glColor3f(0.0f, 1.0f, 1.0f);
	}
	else
	{
		glColor3f(1.0f, 0, 0);
	}

	glPushMatrix();
	glTranslatef(position.X, position.Y, position.Z - (NYCube::CUBE_SIZE - m_currentSize) / 2.0f);
	if (1/*m_currentState != STATE_Dead*/)
	{
		if (m_currentState == STATE_Egg)
		{
			glutSolidSphere(m_minSize, 20, 20);
		}
		else
		{
			glutSolidCube(m_currentSize);
		}
	}

	glPopMatrix();

	//Affichage de d�bug
	if (m_debugDraw)
	{
		m_cone.DebugDraw();
		m_path.DrawPath();
	}
}



//Machine � �tats
bool Wastedosaure::States(StateMachineEvent event, MSG_Object * msg, int state)
{
	BeginStateMachine

		//Reception des messages
		OnMsg(MSG_Attack)//If i'm attacked
	{
			int * data = (int*)msg->GetMsgData();//On recoit le message d'attaque
			life -= *data;//On enleve de la vie
			delete data;//Delete the data

			if (life <= 0)//Si j'ai plus de vie
			{
				PushState(STATE_Dead);//I'm dead
			}
		}//Message Attack


	OnMsg(MSG_PrepareAttack)
	{
		m_timerAttack = 0.0f;
	}//Message PA


	//==========================================================//

	//---Initialize---//
	State(STATE_Initialize)
		OnEnter
		WastedosaureManager::GetSingleton()->AddWastedosaure(this);//On ajoute le W courant dans le vecteur de wastodosaures
	PushState(STATE_Egg);//On devient un oeuf
	partner = NULL;		 //On set le partner � null au cas o�




	//==========================================================//





	//---Egg---//
	State(STATE_Egg)
		OnEnter
		m_timerEgg = 0.0f;

	OnUpdate

	if (m_timerEgg >= m_timeEgg)
	{
		PushState(STATE_FindPath);//Quand on a fini d'etre un oeuf, on va chercher un chemin pour se d�placer.
	}
	m_timerEgg += m_lastUpdate.getElapsedSeconds();
	OnExit
		WastedosaureManager::GetSingleton()->AssignToAGroup(this); //Une fois que je sors du mode oeuf, on assigne l'entit� au groupe
	//On assigne son sexe
	m_isMale = (bool)(rand() % 2);


	//---Find path---//
	State(STATE_FindPath)
		OnEnter
		//On initialise les variables utiles
		m_timerTryFindPath = 0.0f;
	m_path.Clear();
	NYVert2Df arrival = NYVert2Df(WastedosaureManager::GetSingleton()->rand_a_b(1, MAT_SIZE_CUBES - 1),
		WastedosaureManager::GetSingleton()->rand_a_b(1, MAT_SIZE_CUBES - 1));
	if (GetCubeUnderType(arrival) == NYCubeType::CUBE_EAU)//Si le cube d'arriv� es un cube d'eau, on sait que l'on ne va pas trouver de chemin...
	{
		PushState(STATE_FindPath);//...alors on retourne dans le m�me state sans chercher de chemin pour �viter des freeze du au parcours de toutes les nodes pour trouver un chemin.
	}
	else
	{
		if (leader == NULL)//Si je suis leader
		{
			//Alors je cherche un chemin
			m_pf->FindPath(NYVert2Df(position.X / NYCube::CUBE_SIZE, position.Y / NYCube::CUBE_SIZE), arrival, 1, m_path);
		}
		else
		{
			//Sinon je prends le chemin du leader pour le suivre. Permet de faire 1 pf seulement pour toute la meute.
			m_path = leader->GetPath(); //On va suivre le chemin du leader pour �viter de faire 1 pf/cr�ature
		}

		m_currentIndex = FindClosestWaypoint(m_path);//On trouve l'index le plus pret pour �viter de revenir au tout d�but du chemin pour le suivre. Ca ferait bizarre.
		if (HasAPath())//Si on a bien un chemin.
		{
			NYVert3Df distanceClosestWP = m_path.GetWaypoint(m_currentIndex) / NYCube::CUBE_SIZE - position / NYCube::CUBE_SIZE;
			if (distanceClosestWP.getSize() > 20.0f)//Si le closest point est trop loin, on lance quand meme un pf vers le leader pour �viter que l'entit� passe � travers les murs le plus possible, m�me si il y a des rat�s des fois :(.
			{
				arrival = NYVert2Df(leader->position.X / NYCube::CUBE_SIZE, leader->position.Y / NYCube::CUBE_SIZE);
				m_pf->FindPath(NYVert2Df(position.X / NYCube::CUBE_SIZE, position.Y / NYCube::CUBE_SIZE), arrival, 1, m_path);
				m_currentIndex = 0;
			}
		}
	}

	isArrived = false;

	OnUpdate
	if (HasAPath())
	{
		PushState(STATE_Move);//Si on a un chemin, va passer dans l'�tat Move pour le suivre
	}

	if (m_timerTryFindPath >= m_timeTryFindPath)
	{
		PushState(STATE_FindPath);//Sinon on attend un certain nombre de secondes pour rechercher un chemin.
	}
	m_timerTryFindPath += m_lastUpdate.getElapsedSeconds();



	//==========================================================//


	//---Move---//
	State(STATE_Move)
		OnEnter
		//Initialisation de variables
		//cout << "Moving\n";
		m_timerWandering = 0.0f;
	m_timerReproduction = 0.0f;
	if (partner != NULL && //Si j'ai un partenaire
		partner->GetState() != STATE_Move &&  //Qu'il n'est pas en train de bouger 
		partner->GetState() != STATE_Dead) //Et qu'il n'est pas mort
	{
		partner->PushState(STATE_Move); //Il se met � bouger aussi
	}
	OnUpdate
		//On suit le chemin si on peut.
	if (m_currentIndex < m_path.GetSize() - (m_path.GetSize() > groupPosition ? groupPosition : 0))
	{
		//On r�cup�re la direction
		direction = m_path.GetWaypoint(m_currentIndex) - position;
		float lenght = direction.getSize();
		direction.normalize();

		if (lenght < 4.0f)
		{
			++m_currentIndex;
		}
		else
		{
			position += direction * m_speed * m_lastUpdate.getElapsedSeconds();
		}
	}
	else
	{
		PushState(STATE_FindPath);//Si on arrive au bout, on cherche un autre chemin
		isArrived = true;
	}

	OnExit
		m_path.Clear();



	//==========================================================//



	//---Attack---//
	State(STATE_Attack)
		OnEnter
		m_timeElapsedBetween2Attacks = 0;
	OnUpdate
		//Si on est assez proche de la cible
		m_distanceToTarget = NYVert3Df(position / NYCube::CUBE_SIZE - target->position / NYCube::CUBE_SIZE).getSize();
	if (m_distanceToTarget <= m_viewDistance)
	{
		direction = target->position - position;
		direction.normalize();
		position += direction * m_speed * m_lastUpdate.getElapsedSeconds();

		if (m_timeElapsedBetween2Attacks >= m_timeBetween2Attacks && m_distanceToTarget <= m_viewDistance / 2.0f)
		{
			SendMsg(MSG_Attack, target->GetID(), new int(m_damages + rand() % 5));//On va lui envoyer un message comme quoi on l'attaque tous les m_timeBetween2Attacks secondes
			//cout << "Send message\n" << endl;
			m_timeElapsedBetween2Attacks = 0.0f;
		}

		m_timeElapsedBetween2Attacks += m_lastUpdate.getElapsedSeconds();
	}

	//Si on a tu� la cible, on va se reproduire 
	if (target->GetState() == STATE_Dead)
	{
		target = NULL;
		PushState(STATE_Reproduction);
	}

	OnExit
		m_path.Clear();
	target == NULL;


	//==========================================================//


	//---Reproduction---//
	State(STATE_Reproduction)
	OnEnter
	//Si on a pas encore de partenaire
	if (partner == NULL)
		WastedosaureManager::GetSingleton()->FindPartner(this);//On va essayer d'en trouver un.

	m_needPartner = true;

	if (!hasPartner || 
		partner == NULL || 
		partner->GetState() == STATE_Dead)//Le wastedosaure se suicide si on a pas de partenaire ou si celui ci dest mort :(
	{
		PushState(STATE_Suicide);//Snif
	}
	else
	{
		//On va se pr�parer � trouver un endroit pour se reproduire
		m_path.Clear();
		m_timerWandering = 0.0f;
		m_timerReproduction = 0.0f;
		m_currentIndex = 0;

		//Le partenaire se met aussi en mode de reproduction.
		if (partner->GetState() != STATE_Reproduction)
		{
			partner->PushState(STATE_Reproduction);
		}

		//On va trouver un endroit pour se reproduire � proximit� des autres wastedosaures.
		if (arrivalPartner == NYVert2Df(0, 0))
		{
			WastedosaureManager::GetSingleton()->FindReproductionPlace(this, this->partner);
		}

		//Mais si on essaie de trouver un endroit de reproduction sur un cube d'eau. On ne va pas chercher de chemin et on se remet en mode reproduction.
		if (GetCubeUnderType(arrivalPartner) == NYCubeType::CUBE_EAU)
		{
			PushState(STATE_Reproduction);
			partner->PushState(STATE_Reproduction);
		}
		else
		{
			//Sinon on trouve le chemin jusqu'au lieu de reproduction;
			m_pf->FindPath(NYVert2Df(position.X / NYCube::CUBE_SIZE, position.Y / NYCube::CUBE_SIZE), arrivalPartner, 1, m_path);
		}


	}
	OnUpdate

		//Follow the path
	if (HasAPath() && m_currentIndex < m_path.GetSize())
	{
		//On r�cup�re la direction
		direction = m_path.GetWaypoint(m_currentIndex) - position;
		float lenght = direction.getSize();
		direction.normalize();

		if (lenght < 4.0f)
		{
			++m_currentIndex;
		}
		else
		{
			position += direction * m_speed * m_lastUpdate.getElapsedSeconds();
		}
	}
	else
	{
		//Des qu'on est arriv�
		m_path.Clear();
		NYVert3Df tmpArrival = NYVert3Df(arrivalPartner.X, arrivalPartner.Y, m_world->_MatriceHeights[(int)arrivalPartner.X][(int)arrivalPartner.Y]);
		NYVert3Df offsetPartners1 = position / NYCube::CUBE_SIZE - tmpArrival;
		NYVert3Df offsetPartners2 = partner->position / NYCube::CUBE_SIZE - tmpArrival;
		//If close enought, reproduce
		if (m_canReproduce && //Que l'on peut se reproduire
			m_counterReproduction < m_maxReproductions && //Qu'on a pas atteint le nombre de reproduction amx
			offsetPartners1.getSize() <= 1 && //Que l'entit� est arriv� au point de reproduction
			offsetPartners2.getSize() <= 1 &&//et que son partenaire aussi
			(*m_entities)[WASTEDOSAURE].size() < MAX_WASTEDOSAURE //Qu'il n'y a pas d�j� trop de wastedosaures
			/*&& m_isMale != partner->m_isMale //et qu'ils ont le m�me sexe*/)
		{
			//On incr�mentes les compteurs de reproduction
			m_counterReproduction++;
			partner->m_counterReproduction++;
			//On dit qu'on ne peut plus se reproduire actuellement
			partner->m_canReproduce = false;
			m_canReproduce = false;

			//On cr�e un b�b� wastedosaure
			Wastedosaure * w = new Wastedosaure(m_world, NYVert2Df(position.X / NYCube::CUBE_SIZE, position.Y / NYCube::CUBE_SIZE));
			w->SetEntities(m_entities);
			(*m_entities)[WASTEDOSAURE].push_back(w);
		}
	}

	OnExit
	m_canReproduce = true;//On peut se reproduire � nouveau
	arrivalPartner = NYVert2Df(0, 0); //Nouveau lieu de ponte � determiner
	m_path.Clear();




	//==========================================================//




	//---Suicide---//
	State(STATE_Suicide)
		OnEnter
		m_path.Clear();
	FindClosestCubeWater();//On trouve le cube d'eau le plus proche
	//On cherche le chemin vers le cube d'eau le plus proche
	m_pf->FindPath(NYVert2Df(position.X / NYCube::CUBE_SIZE, position.Y / NYCube::CUBE_SIZE),
		NYVert2Df(m_cubeWater.X, m_cubeWater.Y), 2, m_path, true);
	m_currentIndex = 0;
	target = NULL; //Au cas o�
	partner = NULL;//Au cas o�
	OnUpdate
		//Si on a un chemin, on va jusqu'� l'eau
	if (HasAPath())
	{
		if (m_currentIndex < m_path.GetSize())
		{
			//On r�cup�re la direction
			direction = m_path.GetWaypoint(m_currentIndex) - position;
			float lenght = direction.getSize();
			direction.normalize();

			if (lenght < 4.0f)
			{
				++m_currentIndex;
			}
			else
			{
				position += direction * m_speed * m_lastUpdate.getElapsedSeconds();
			}
		}
		else
		{
			PushState(STATE_Dead);//Quand on est arriv�, l'entit� meurt
		}
	}
	else
	{
		PushState(STATE_Dead); //Si on a pas trouv� de chemin, on meurt sur place
	}


	//==========================================================//


	//---Dead---//
	State(STATE_Dead)
		OnMsg(MSG_Attack)
		OnMsg(MSG_PrepareAttack)
		OnEnter
		//cout << "Im dead. ID " << GetID() <<"\n";


		EndStateMachine
}