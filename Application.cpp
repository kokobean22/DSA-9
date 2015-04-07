#include "ApplicationClass.h"
void ApplicationClass::InitUserAppVariables()
{
	m_pMeshMngr = MeshManagerSingleton::GetInstance();
	m_pMeshMngr->LoadModel("Minecraft\\MC_Creeper.obj", "Creeper");
	m_pMeshMngr->LoadModel("Minecraft\\MC_Cow.obj", "Cow", glm::translate(vector3(-4.0f, 0.0f, 0.0f)));
	m_pMeshMngr->LoadModel("Minecraft\\MC_Zombie.obj", "Zombie", glm::translate(vector3(-2.0f, 0.0f, 0.0f)));
	m_pMeshMngr->LoadModel("Minecraft\\MC_Steve.obj", "Steve", glm::translate(vector3( 2.0f, 0.0f, 0.0f)));
	m_pMeshMngr->LoadModel("Minecraft\\MC_Pig.obj", "Pig", glm::translate(vector3(4.0f, 0.0f, 0.0f)));

	m_pCamera->SetPosition(vector3(0.0f,1.0f,7.5f));

	m_sSelectedObject = "Creeper";
}
void ApplicationClass::Update (void)
{
	m_pSystem->UpdateTime(); //Update the system
	float fTimeSpan = m_pSystem->LapClock(0); //Check the time difference between this method calls
	static float fRunTime = 0.0f;
	fRunTime += fTimeSpan; //Update the run time count

	m_pMeshMngr->Update(); //Update the mesh information

	m_pMeshMngr->SetModelMatrix(m_m4SelectedObject, m_sSelectedObject); //Setting up the Model Matrix
	
	m_pBBMngr->GenerateBoundingBox(m_pMeshMngr->GetModelMatrix("Creeper"), "Creeper");
	m_pBBMngr->GenerateBoundingBox(m_pMeshMngr->GetModelMatrix("Cow"), "Cow");
	m_pBBMngr->GenerateBoundingBox(m_pMeshMngr->GetModelMatrix("Zombie"), "Zombie");
	m_pBBMngr->GenerateBoundingBox(m_pMeshMngr->GetModelMatrix("Steve"), "Steve");
	m_pBBMngr->GenerateBoundingBox(m_pMeshMngr->GetModelMatrix("Pig"), "Pig");

	m_pBBMngr->CalculateCollision();

	m_pBBMngr->AddBoxToRenderList("ALL");
	m_pMeshMngr->AddInstanceToRenderList("ALL");
	
	//First person camera movement
	if(m_bFPC == true)
		CameraRotation();

	if(m_bArcBall == true)
		ArcBall();

	printf("FPS: %d, %.3f\r", m_pSystem->FPS, fRunTime);//print the Frames per Second
}
void ApplicationClass::Display (void) //for OpenGL 3.X Applications
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); // clear the window

	m_pGrid->Render(100.0f); //renders the grid with a 100 scale

	m_pMeshMngr->Render();

	m_pGLSystem->GLSwapBuffers(); //Swaps the OpenGL buffers
}
