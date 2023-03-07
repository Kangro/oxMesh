#include "stdafx.h"
#include "Include/oxGmsh.h"
/*
命令执行gmsh 剖分
*/
int main(int argc, char *argv[])
{

	if (argc == 2)
	{
		std::string config_path = argv[1];
		
		int enum_code = oxm::oxGmsh::instance()->ExeConfigScript(config_path);
		if (enum_code == 0)
			return 1;
	}




	return 0;
}