#include "ApplicationClass.h"
#include <locale>
#include <codecvt>
#include <string>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Callbacks for winapp 
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* Instance of the application */
ApplicationClass* ApplicationClass::m_pInstance = nullptr;
void ApplicationClass::InitAppSystem(void)
{
	// Delete the .cfg file for the following changes take effect

	// Color of the screen
	m_v4ClearColor = vector4(0.4f, 0.6f, 0.9f, 0.0f); // Set the clear color based on Microsoft's CornflowerBlue (default in XNA)
		
	// Indicate window properties
	m_pSystem->WindowName = "Bounding Boxes";
	m_pSystem->WindowWidth = 1280;
	m_pSystem->WindowHeight = 720;
	m_pSystem->WindowFullscreen = false;
	m_pSystem->WindowBorderless = false;

	// Get the singletons
	m_pLightMngr = LightManagerSingleton::GetInstance();
}
void ApplicationClass::Release()
{
	SafeDelete(m_pGrid);
	SafeDelete(m_pWindow);

	BoundingBoxManagerSingleton::ReleaseInstance();

	// Release all the singletons used in the dll
	ReleaseAllSingletons();
}
ApplicationClass* ApplicationClass::GetInstance( HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// If the instance does not exist
	if(m_pInstance == nullptr)
	{
		// Create the instance
		m_pInstance = new ApplicationClass( );
		// Initialize the instance
		m_pInstance->Init(hInstance, lpCmdLine, nCmdShow);
	}
	// Return the instance
	return m_pInstance;
}
ApplicationClass::ApplicationClass()
{
	//Initialize all the singletons to nullptrs
	m_pSystem = nullptr;
	m_pWindow = nullptr;
	m_pGLSystem = nullptr;
	m_pLightMngr = nullptr;
}
ApplicationClass::ApplicationClass(ApplicationClass const& other){}
ApplicationClass& ApplicationClass::operator=(ApplicationClass const& other){	return *this; }
ApplicationClass::~ApplicationClass()
{
	// Call the release method when the instance is destroyed
	Release();
};
void ApplicationClass::ReleaseInstance(void)
{
	// If the instance exists
	if(m_pInstance != nullptr)
	{
		// Delete the instance
		delete m_pInstance;
		// Make the pointer a nullptr reference
		m_pInstance = nullptr;
	}
}
void ApplicationClass::Init( HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Is this running out of Visual Studio?
	if(IsDebuggerPresent())
	{
		printf("Shaders: ");
		system("cmd.exe /C xcopy \"../Solution/Shaders\" \"Shaders\" /y /q");
		m_pWindow->CreateConsoleWindow();
	}

	// Get the system singleton
	m_pSystem = SystemSingleton::GetInstance();

	// Init the App System
	InitAppSystem();

	// Read the configuration file
	ReadConfig();

#pragma region Window Construction and Context setup
	// Create a new window and set its properties
	m_pWindow = new WindowClass( hInstance, nCmdShow, WndProc);
	m_pWindow->SetFullscreen(m_pSystem->IsWindowFullscreen());
	m_pWindow->SetBorderless(m_pSystem->IsWindowBorderless());

	// Make the Window name a wide string
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes( m_pSystem->WindowName);

	// Create a window in the window class
	m_pWindow->CreateMEWindow(wide.c_str(), m_pSystem->WindowWidth, m_pSystem->WindowHeight);
	
	// Get the instance of the class
	m_pGLSystem = GLSystemSingleton::GetInstance();

	// Set OPENGL 3.x Context
	m_pSystem->m_RenderingContext = OPENGL3X;
	
	// Create context
	m_pGLSystem->InitGLDevice(m_pWindow->GetHandler());

	// Verify what is the OpenGL rendering context and save it to system (3.x might fail, in which case exit)
	if(m_pGLSystem->IsNewOpenGLRunning() == false)
		exit(0);
#pragma endregion

	// Get the singletons
	m_pCamera = CameraSingleton::GetInstance();

	// Initialize the App Variables
	InitInternalAppVariables();
	InitUserAppVariables();

	//Color of the window
	if(m_pSystem->m_RenderingContext == OPENGL3X)
		glClearColor( m_v4ClearColor.r, m_v4ClearColor.g, m_v4ClearColor.b, m_v4ClearColor.a);

	//Start the clock
	m_pSystem->StartClock();

	printf("\n");
}
void ApplicationClass::ReadConfig(void)
{
	//If we are reading the changes the changes, know what file to open
	String sRoot = m_pSystem->m_pFolder->GetFolderRoot();
	String App = m_pSystem->ApplicationName;
	App = sRoot + App + ".cfg";

	FileReaderClass reader;
	//If the file doesnt exist, create it and exit this method
	if(reader.ReadFile(App.c_str()) == MEErrors::ERROR_FILE_MISSING)
	{
		WriteConfig();
		return;
	}
	
	//read the file
	reader.Rewind();
	while(reader.ReadNextLine() == RUNNING)
	{
		String sWord = reader.GetFirstWord();

		int nLenght  = reader.m_sLine.length();
		char* zsTemp = new char[nLenght];
		
		if(sWord == "Fullscreen:")
		{
			int nValue;
			sscanf_s(reader.m_sLine.c_str(), "Fullscreen: %d", &nValue);
			if(nValue == 0)
				m_pSystem->SetWindowFullscreen(false);
			else
				m_pSystem->SetWindowFullscreen(true);
		}
		else if(sWord == "Borderless:")
		{
			int nValue;
			sscanf_s(reader.m_sLine.c_str(), "Borderless: %d", &nValue);
			if(nValue == 0)
				m_pSystem->SetWindowBorderless(false);
			else
				m_pSystem->SetWindowBorderless(true);
		}
		else if(sWord == "Resolution:")
		{
			int nValue1;
			int nValue2;
			sscanf_s(reader.m_sLine.c_str(), "Resolution: [ %d x %d ]", &nValue1,  &nValue2);
			m_pSystem->WindowWidth = nValue1;
			m_pSystem->WindowHeight = nValue2;
		}
		else if(sWord == "Ambient:")
		{
			float fValueX;
			float fValueY;
			float fValueZ;
			sscanf_s(reader.m_sLine.c_str(), "Ambient: [%f,%f,%f]", &fValueX, &fValueY, &fValueZ);
			m_pLightMngr->SetColor(vector3(fValueX, fValueY, fValueZ), 0);
		}
		else if(sWord == "Background:")
		{
			float fValueX;
			float fValueY;
			float fValueZ;
			float fValueW;
			sscanf_s(reader.m_sLine.c_str(), "Background: [%f,%f,%f,%f]", &fValueX, &fValueY, &fValueZ, &fValueW);
			m_v4ClearColor = vector4(fValueX, fValueY, fValueZ, fValueW);
		}
		else if(sWord == "AmbientPower:")
		{
			float fValue;
			sscanf_s(reader.m_sLine.c_str(), "AmbientPower: %f", &fValue);
			m_pLightMngr->SetIntensity(fValue, 0);
		}
		else if(sWord == "Data:")
		{
			sscanf_s(reader.m_sLine.c_str(), "Data: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderData( zsTemp );
		}
		else if(sWord == "3DS:")
		{
			sscanf_s(reader.m_sLine.c_str(), "3DS: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderM3DS( zsTemp );
		}
		else if(sWord == "BTO:")
		{
			sscanf_s(reader.m_sLine.c_str(), "BTO: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderMBTO( zsTemp );
		}
		else if(sWord == "FBX:")
		{
			sscanf_s(reader.m_sLine.c_str(), "FBX: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderMFBX( zsTemp );
		}
		else if(sWord == "OBJ:")
		{
			sscanf_s(reader.m_sLine.c_str(), "OBJ: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderMOBJ( zsTemp );
		}
		else if(sWord == "POM:")
		{
			sscanf_s(reader.m_sLine.c_str(), "POM: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderMPOM( zsTemp );
		}
		else if(sWord == "Level:")
		{
			sscanf_s(reader.m_sLine.c_str(), "Level: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderLVL( zsTemp );
		}
		else if(sWord == "Textures:")
		{
			sscanf_s(reader.m_sLine.c_str(), "Textures: %s", zsTemp, nLenght);
			m_pSystem->m_pFolder->SetFolderTextures( zsTemp );
		}

		delete[] zsTemp;
		zsTemp = nullptr;
	}
	reader.CloseFile();
}
void ApplicationClass::WriteConfig(void)
{
	// Write the configuration for this application
	String sRoot = m_pSystem->m_pFolder->GetFolderRoot();
	String App = m_pSystem->ApplicationName;
	App = sRoot + App + ".cfg";

	FILE *pFile; 
	fopen_s(&pFile, App.c_str(),"w");
	if(!pFile)	//If we couldn't create the file we exit without changes.
		return;

	rewind(pFile);
	fprintf(pFile,"# Configuration file for the program: %s", m_pSystem->GetAppName().c_str());
	
	fprintf(pFile,"\n\nFullscreen: ");
	if(m_pSystem->IsWindowFullscreen())
		fprintf(pFile,"1");
	else
		fprintf(pFile,"0");

	fprintf(pFile,"\nBorderless: ");
	if(m_pSystem->IsWindowBorderless())
		fprintf(pFile,"1");
	else
		fprintf(pFile,"0");

	fprintf(pFile,"\nContext: OPENGL3X"); //Only openGL3X context is supported ATM

	fprintf(pFile,"\n\n# Resolution: [ 640 x 480 ]");
	fprintf(pFile,"\n# Resolution: [ 1280 x 720 ]");
	fprintf(pFile,"\n# Resolution: [ 1680 x 1050 ]");
	fprintf(pFile,"\n# Resolution: [ 1920 x 1080 ]");
	fprintf(pFile,"\n# Resolution: [ 2650 x 1440 ]");
	fprintf(pFile,"\nResolution: [ %d x %d ]", m_pSystem->WindowWidth, m_pSystem->WindowHeight);

	fprintf(pFile,"\n\nAmbient: [%.2f,%.2f,%.2f]",
		m_pLightMngr->GetColor(0).r, m_pLightMngr->GetColor(0).g, m_pLightMngr->GetColor(0).b);
	fprintf(pFile,"\nAmbientPower: %.2f", m_pLightMngr->GetIntensity(0));

	fprintf(pFile,"\n\nBackground: [%.3f,%.3f,%.3f,%.3f]",
		m_v4ClearColor.r, m_v4ClearColor.g, m_v4ClearColor.b, m_v4ClearColor.a);

	fprintf(pFile,"\n\n# Folders:");
	
	fprintf(pFile,"\nData:		%s", m_pSystem->m_pFolder->GetFolderData().c_str());
	fprintf(pFile,"\n3DS:		%s", m_pSystem->m_pFolder->GetFolderM3DS().c_str());
	fprintf(pFile,"\nBTO:		%s", m_pSystem->m_pFolder->GetFolderMBTO().c_str());
	fprintf(pFile,"\nFBX:		%s", m_pSystem->m_pFolder->GetFolderMFBX().c_str());
	fprintf(pFile,"\nOBJ:		%s", m_pSystem->m_pFolder->GetFolderMOBJ().c_str());
	fprintf(pFile,"\nLevel:		%s", m_pSystem->m_pFolder->GetFolderLVL().c_str());
	fprintf(pFile,"\nTextures:	%s", m_pSystem->m_pFolder->GetFolderTextures().c_str());

	fclose(pFile);
}
//--------
void ApplicationClass::Reshape(int a_nWidth, int a_nHeight)
{
	//No changes here yet
}
void ApplicationClass::Run (void)
{
	//Run the main loop until the exit message is sent
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			ProcessKeyboard();
			ProcessMouse();
			ProcessJoystick();
			Update();
			Display();
		}
		Idle();
	}
}
//--------
void ApplicationClass::InitInternalAppVariables()
{
	// The camera rotation and the arcball are not being used by default
	m_bFPC = false;
	m_bArcBall = false;
	
	// Set the properties for the light in the scene
	m_pLightMngr->SetPosition( glm::vec3( 0, 0, 10) );
	m_pLightMngr->SetColor( glm::vec3( 1, 1, 1) );
	m_pLightMngr->SetIntensity ( 5.0f ); //Rotating Light
	m_pLightMngr->SetIntensity ( 0.75f, 0 ); //Ambient light (Ambient Light is always the first light, or light[0])

	// Create a new grid initializing its properties and compiling it
	m_pGrid = new GridClass(MEAXIS::XY);
	m_pGrid->CompileGrid();

	m_pBBMngr = BoundingBoxManagerSingleton::GetInstance();
}
void ApplicationClass::Idle (void)
{
	m_bFPC = true; //Enable the First Person Camera
}