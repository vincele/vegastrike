/***************************************************************************
 *                           selector.cpp  -  description
 *                           ----------------------------
 *                           begin                : February 20, 2002
 *                           copyright            : (C) 2002 by David Ranger
 *                           email                : sabarok@start.com.au
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   any later version.                                                    *
 *                                                                         *
 **************************************************************************/

#include "selector.h"

glob_t *MISSIONS;
struct mission_data DATA;

// Primary initialization function. Sets everything up and takes care of the program
char *Start(int run_vegastrike) {
	InitGraphics();

	DATA.name = NewString("");
	DATA.author = NewString("");
	DATA.description = NewString("");
	DATA.briefing = NewString("");
	DATA.path = NewString("");

	glutMouseFunc(ProcessMouseClick);
	glutMotionFunc(ProcessMouseMoveActive);
	glutPassiveMotionFunc(ProcessMouseMovePassive);
	glutReshapeFunc(ChangeSize);
	glutDisplayFunc(show_main);

	glutMainLoop();
	
	return DATA.path;
}

void RunMission(void) {
	if (DATA.path[0] == '\0') { cout << "No mission selected\n"; return; }
	cout << "Starting " << MISSION_PROGRAM << " with mission " << DATA.path << endl;
#ifdef _WIN32
	char execname [2048];
	char mypath[1500];
	getcwd (mypath,1499);
	sprintf (execname,"%s\\Vegastrike",mypath);
	spawnl(P_NOWAIT,execname,execname,DATA.path,NULL);
#else
    #if defined(WITH_MACOSX_BUNDLE)
        chdir(macosxExecutablePath);
    #endif
	execlp("./vegastrike", "./vegastrike", DATA.path, NULL);
#endif
}
