#include "base.h"
#ifdef BASE_MAKER
//#ifndef BASE_XML
#include <stdio.h>

void BaseInterface::Room::Link::EndXML (FILE *fp) {
	fprintf(fp,"room, '%s', %g, %g, %g, %g, '%s'",index.c_str(),x,y,wid,hei,text.c_str());
}

void BaseInterface::Room::Goto::EndXML (FILE *fp) {
	fprintf(fp,"Base.Link (");
	Link::EndXML(fp);
	fprintf(fp,", %d)\n", Goto::index);
}

void BaseInterface::Room::Python::EndXML (FILE *fp) {
	fprintf(fp,"Base.Python (");
	Link::EndXML(fp);
	fprintf(fp,", '%s')\n",file.c_str());
}

void BaseInterface::Room::Talk::EndXML (FILE *fp) {
	char randstr[100];
	sprintf(randstr,"NEW_SCRIPT_%d.py",(int)(rand()));
	fprintf(fp,"Base.Python (");
	Link::EndXML(fp);
	fprintf(fp,", '%s')\n",randstr);
	FILE *py=fopen(randstr,"wt");
	fprintf(py,"import Base\nimport VS\nimport random\n\nrandnum=random.randrange(0,%d)\n",say.size());
	for (int i=0;i<say.size();i++) {
		fprintf(fp,"if (randnum==%d):\n",i);
		for (int j=0;j<say[i].size();j++) {
			if (say[i][j]=='\n') {
				say[i][j]='\\';
				static const char *ins="n";
				say[i].insert(j,ins);
			}
		}
		fprintf(fp,"  Base.Message ('%s')\n",say[i].c_str());
		if (!(soundfiles[i].empty()))
			fprintf(fp,"  VS.playSound ('%s', (0,0,0), (0,0,0))\n",soundfiles[i].c_str());
	}
	//obolete... creates a file that uses the Python function instead.
}

void BaseInterface::Room::Launch::EndXML (FILE *fp) {
	fprintf(fp,"Base.Launch (");
	Link::EndXML(fp);
	fprintf(fp,")\n");
}

void BaseInterface::Room::Comp::EndXML (FILE *fp) {
	fprintf(fp,"Base.Comp (");
	Link::EndXML(fp);
	fwrite(", '",3,1,fp);
	for (int i=0;i<modes.size();i++) {
		char *mode=NULL;
		switch(modes[i]) {
		case UpgradingInfo::NEWSMODE:
			mode="NewsMode";
			break;
		case UpgradingInfo::SHIPDEALERMODE:
			mode="ShipMode";
			break;
		case UpgradingInfo::UPGRADEMODE:
			mode="UpgradeMode";
			break;
		case UpgradingInfo::DOWNGRADEMODE:
			mode="DowngradeMode";
			break;
		case UpgradingInfo::BRIEFINGMODE:
			mode="BriefingMode";
			break;
		case UpgradingInfo::MISSIONMODE:
			mode="MissionMode";
			break;
		case UpgradingInfo::SELLMODE:
			mode="SellMode";
			break;
		case UpgradingInfo::BUYMODE:
			mode="BuyMode";
			break;
		}
		if (mode)
			fprintf(fp,"%s ",mode);
		if ((i+1)==(modes.size()))
			fprintf(fp,"'");
	}
	fprintf(fp,")\n");
}

void BaseInterface::Room::BaseObj::EndXML (FILE *fp) {
//		Do nothing
}

void BaseInterface::Room::BaseShip::EndXML (FILE *fp) {
	fprintf(fp,"Base.Ship (room, '%s', (%lg,%lg,%lg), (%g, %g, %g), (%g, %g, %g))\n",index.c_str()
			,mat.p.i,mat.p.j,mat.p.k
			,mat.getR().i,mat.getR().j,mat.getR().k
			,mat.getQ().i,mat.getQ().j,mat.getQ().k);
}

void BaseInterface::Room::BaseSprite::EndXML (FILE *fp) {
	float x,y;
	spr.GetPosition(x,y);
	fprintf(fp,"Base.Texture (room, '%s', '%s', %g, %g)\n",index.c_str(),texfile.c_str(),x,y);
}

void BaseInterface::Room::EndXML (FILE *fp) {
	int i;
	i=fprintf(fp,"room = Base.Room ('%s')\n",deftext.c_str());
	for (i=0;i<links.size();i++) {
		if (links[i])
			links[i]->EndXML(fp);
	}
	for (i=0;i<objs.size();i++) {
		if (objs[i])
			objs[i]->EndXML(fp);
	}
	fprintf(fp,"\n");
	fflush(fp);
}

void BaseInterface::EndXML (FILE *fp) {
	fprintf(fp,"import Base\n\n");
	for (int i=0;i<rooms.size();i++) {
		rooms[i]->EndXML(fp);
	}
}

//#endif
#endif
