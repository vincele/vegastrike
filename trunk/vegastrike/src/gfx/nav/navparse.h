

bool NavigationSystem::ParseFile(string filename)
{
	string expression = "";

	string tag = "";
	string data = "";

	char next = ' ';
	int totalitems = 0;
	bool comment = 0;
	int commentdepth = 0;

	ifstream dataset_file;
	dataset_file.open("navdata.xml");
	if (dataset_file.fail())
	{
		return 0;
	}

	int depth = 0;
	while (! dataset_file.eof())
	{
		if(next == '<')							//	trap <*>
		{										//	- know what type it is
			dataset_file.get(next);				//
			while(next != '>')					//
			{									//
				expression = expression+next;	//
				dataset_file.get(next);			//
			}									//

			if(expression[0] == '!')	//	doesnt catch comment in comment
			{
				while( (expression[expression.size()-1] != '>') || (expression[expression.size()-2] != '-') )
				{
					expression = expression+next;
					dataset_file.get(next);
				}
				expression = "";
				continue;
			}

			if ((expression[0] != '/') && (expression[expression.size()-1] != '/'))	//	starter
			{
				tag = expression;
				expression = "";
			}

			else if(expression[0] == '/')	//	terminator = no more to be done
			{
				data = "";
				tag = "";
				expression = "";
			}

			else if (expression[expression.size()-1] == '/')	//	data
			{
				data = expression;

				if(tag == "console")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_small = atof( (retrievedata(data, "x_small")).c_str() );
					float x_large = atof( (retrievedata(data, "x_large")).c_str() );
					float y_small = atof( (retrievedata(data, "y_small")).c_str() );
					float y_large = atof( (retrievedata(data, "y_large")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					screenskipby4[0] = x_small;
					screenskipby4[1] = x_large;
					screenskipby4[2] = y_small;
					screenskipby4[3] = y_large;

					mesh[0] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button1")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_1[0] = x_;
					buttonskipby4_1[1] = x_ + dx_;
					buttonskipby4_1[2] = y_;
					buttonskipby4_1[3] = y_ + dy_;

					mesh[1] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button2")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_2[0] = x_;
					buttonskipby4_2[1] = x_ + dx_;
					buttonskipby4_2[2] = y_;
					buttonskipby4_2[3] = y_ + dy_;

					mesh[2] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button3")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_3[0] = x_;
					buttonskipby4_3[1] = x_ + dx_;
					buttonskipby4_3[2] = y_;
					buttonskipby4_3[3] = y_ + dy_;

					mesh[3] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button4")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_4[0] = x_;
					buttonskipby4_4[1] = x_ + dx_;
					buttonskipby4_4[2] = y_;
					buttonskipby4_4[3] = y_ + dy_;

					mesh[4] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button5")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_5[0] = x_;
					buttonskipby4_5[1] = x_ + dx_;
					buttonskipby4_5[2] = y_;
					buttonskipby4_5[3] = y_ + dy_;

					mesh[5] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button6")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_6[0] = x_;
					buttonskipby4_6[1] = x_ + dx_;
					buttonskipby4_6[2] = y_;
					buttonskipby4_6[3] = y_ + dy_;

					mesh[6] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "button7")
				{
					string mesh_ = retrievedata(data, "Meshfile file");
					float x_ = atof( (retrievedata(data, "x")).c_str() );
					float y_ = atof( (retrievedata(data, "y")).c_str() );
					float z_ = atof( (retrievedata(data, "z")).c_str() );
					float dx_ = atof( (retrievedata(data, "dx")).c_str() );
					float dy_ = atof( (retrievedata(data, "dy")).c_str() );
					float scale_ = atof( (retrievedata(data, "scale")).c_str() );

					buttonskipby4_7[0] = x_;
					buttonskipby4_7[1] = x_ + dx_;
					buttonskipby4_7[2] = y_;
					buttonskipby4_7[3] = y_ + dy_;

					mesh[7] = new Mesh(mesh_.c_str(),1,0,NULL);

				}

				else if(tag == "systemiteminfo")
				{
					float scale_ = atof( (retrievedata(data, "Info itemscale")).c_str() );
					float zmult_ = atof( (retrievedata(data, "zshiftmultiplier")).c_str() );
					float zfactor_ = atof( (retrievedata(data, "itemzscalefactor")).c_str() );


					if(scale_ < 0.5)
						system_item_scale = 0.5;
					else if(scale_ > 4)
						system_item_scale = 4;
					else
						system_item_scale = scale_;



					if(zmult_ < 0.5)
						zshiftmultiplier = 0.5;
					else if(zmult_ > 6)
						zshiftmultiplier = 6;
					else
						zshiftmultiplier = zmult_;



					if(zfactor_ < 1.0)
						item_zscalefactor = 1.0;
					else if(zfactor_ > 8)
						item_zscalefactor = 8;
					else
						item_zscalefactor = zfactor_;
				}

				else if(tag == "factioncolours")
				{
					string factionname = retrievedata(data, "colour faction");
					float r_ = atof( (retrievedata(data, "r")).c_str() );
					float g_ = atof( (retrievedata(data, "g")).c_str() );
					float b_ = atof( (retrievedata(data, "b")).c_str() );
					float a_ = atof( (retrievedata(data, "a")).c_str() );


					for(int k = 0; k < FactionUtil::GetNumFactions(); k++)
					{
						if((FactionUtil::GetFactionName(k)) == factionname)
						{
							factioncolours[k].r = r_;
							factioncolours[k].g = g_;
							factioncolours[k].b = b_;
							factioncolours[k].a = a_;
						}
					}
				}

				data = "";
			//	tag = "";
				expression = "";
				continue;
			}

			else
				continue;

		}

		dataset_file.get(next);

		//	do something
	}

	dataset_file.close();
	if(totalitems == 0)
		return 1;	
	else
		return 0;
}