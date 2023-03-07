#include "stdafx.h"
#include "Include/oxGmsh.h"
#include "Include/oxTool.h"
#include "gmsh.h"
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <iostream>
#include <iomanip>
#include <future>
#include <set>
#include "Include/qxPoint.h"
#include <fstream>


#include <occ/inc/TopExp_Explorer.hxx>
#include <occ/inc/TopoDS.hxx>
#include <occ/inc/BRepBuilderAPI_MakeVertex.hxx>
#include <occ/inc/TopoDS_Vertex.hxx>
#include <occ/inc/BRepBuilderAPI_Copy.hxx>
#include <occ/inc/BRepAlgoAPI_BooleanOperation.hxx>
#include <occ/inc/GProp_GProps.hxx>
#include <occ/inc/BRepGProp.hxx>




//必须在最后
#include "setDebugNew.h"


using namespace std;
namespace oxm {
	double CalcShapeArea(const cxShape& shape)
	{
		//面积
		GProp_GProps G;
		BRepGProp::SurfaceProperties(shape.occshape(), G);
		return G.Mass();
	}

	bool MakeVertexByPoint(const qxGeom::qxPoint& pnt, cxShape& vtx_shape)
	{
		gp_Pnt gppnt(pnt[0], pnt[1], pnt[2]);
		BRepBuilderAPI_MakeVertex mk_vtx(gppnt);
		TopoDS_Vertex vtx = mk_vtx.Vertex();
		vtx_shape.occshape() = vtx;

		return true;
	}
	bool IntersectionOperation(TopoDS_Shape& arg, TopoDS_Shape& tool, TopoDS_Shape& rslt_shape)
	{
		BRepAlgoAPI_BooleanOperation common_operator;

		TopTools_ListOfShape arguments;
		arguments.Append(arg);
		common_operator.SetArguments(arguments);

		TopTools_ListOfShape toollist;
		toollist.Append(tool);
		common_operator.SetTools(toollist);

		common_operator.SetRunParallel(true);
		common_operator.SetOperation(BOPAlgo_COMMON);
		common_operator.Build();

		TopoDS_Shape shape = common_operator.Shape();

		bool empty_vtx = true;
		TopExp_Explorer Ex;
		for (Ex.Init(shape, TopAbs_VERTEX); Ex.More(); Ex.Next())
		{
			empty_vtx = false;
			break;
		}

		double shape_area = CalcShapeArea(shape);
		if (empty_vtx == true && fabs(shape_area - 0.0) < 1.0e-6)
		{
			shape.Nullify();
			return false;
		}

		rslt_shape = shape;
		return true;
	}
	bool IsPointInShape(const qxGeom::qxPoint &p1, const cxShape& shape)
	{
		cxShape vtx_shape;
		if (!MakeVertexByPoint(p1, vtx_shape))
			return false;

		TopoDS_Shape copy_shape = BRepBuilderAPI_Copy(shape.occshape(), Standard_True, Standard_False).Shape();

		//boolean to judge is intersect ??
		TopoDS_Shape rslt_shape;
		IntersectionOperation(vtx_shape.occshape(), copy_shape, rslt_shape);

		TopExp_Explorer Ex;
		for (Ex.Init(rslt_shape, TopAbs_VERTEX); Ex.More(); Ex.Next())
		{
			return true;
		}

		return false;
	}
	bool GetShapeAsFaces(const cxShape& shape, std::vector<cxShape>& vec_faces)
	{
		TopExp_Explorer Ex;
		for (Ex.Init(shape.occshape(), TopAbs_FACE); Ex.More(); Ex.Next())
		{
			const TopoDS_Face& face = TopoDS::Face(Ex.Current());
			vec_faces.push_back(face);
		}

		return true;
	}
	bool FindFaceSideSolids(const cxShape& face, std::vector<cxShape>& solids, std::pair<int, int>& solidIdxPair)
	{
		std::vector<int> solid_idxes;
		for (int i = 0; i < solids.size(); i++)
		{
			std::vector<cxShape> vec_faces;
			GetShapeAsFaces(solids[i], vec_faces);
			//
			for (auto& eachface : vec_faces)
			{
				if (eachface.occshape().TShape() == face.occshape().TShape() &&
					eachface.occshape().Location() == face.occshape().Location())
				{
					solid_idxes.push_back(i);
					break;
				}
			}
		}

#ifdef _DEBUG
		assert(solid_idxes.size() == 2);
#endif // _DEBUG

		if (solid_idxes.size() == 2)
		{
			solidIdxPair = std::make_pair(solid_idxes[0], solid_idxes[1]);
		}

		return true;
	}

	bool oxGmsh::ExeConfigScript(const std::string& config_script_path)
	{
		//导入脚本文件
		std::ifstream ifs(config_script_path + "/" + "config.json");
		rapidjson::IStreamWrapper isw(ifs);

		rapidjson::Document d;
		d.ParseStream(isw);


		//解析出数据
		std::string str_work_path;
		std::vector<std::pair<int, cxShape>> vec_id_parts;
		std::vector<std::pair<int, cxShape>> vec_id_solids;
		std::vector<std::pair<int, cxShape>> vec_id_innerfaces;
		std::vector<std::pair<int, cxShape>> vec_id_faces;
		std::vector<int> ovv_ids;
		std::vector<std::string> vec_part_name;
		std::map<int, std::vector<std::pair<std::string, std::vector<int>>>> map_solidid_subFaceGroups;

		if (d.HasMember("str_work_path"))
		{
			const rapidjson::Value& str_val = d["str_work_path"];
			str_work_path = str_val.GetString();
		}

		if (d.HasMember("vec_id_parts"))
		{
			const rapidjson::Value& array_val = d["vec_id_parts"];
			for (rapidjson::Value::ConstMemberIterator itr = array_val.MemberBegin();
				itr != array_val.MemberEnd(); ++itr)
			{
				std::string id_string = itr->name.GetString();
				int id = atoi(id_string.c_str());

				cxShape cx_shape;
				cx_shape.fromJson(itr->value, 1.0);

				vec_id_parts.push_back(std::make_pair(id, cx_shape));
				vec_part_name.push_back(cx_shape.name());
			}
		}

		if (d.HasMember("vec_id_solids"))
		{
			const rapidjson::Value& array_val = d["vec_id_solids"];
			for (rapidjson::Value::ConstMemberIterator itr = array_val.MemberBegin();
				itr != array_val.MemberEnd(); ++itr)
			{
				std::string id_string = itr->name.GetString();
				int id = atoi(id_string.c_str());

				cxShape cx_shape;
				cx_shape.fromJson(itr->value, 1.0);

				vec_id_solids.push_back(std::make_pair(id, cx_shape));
			}
		}

		if (d.HasMember("vec_id_innerfaces"))
		{
			const rapidjson::Value& array_val = d["vec_id_innerfaces"];
			for (rapidjson::Value::ConstMemberIterator itr = array_val.MemberBegin();
				itr != array_val.MemberEnd(); ++itr)
			{
				std::string id_string = itr->name.GetString();
				int id = atoi(id_string.c_str());

				cxShape cx_shape;
				cx_shape.fromJson(itr->value, 1.0);

				vec_id_innerfaces.push_back(std::make_pair(id, cx_shape));
			}
		}

		if (d.HasMember("vec_id_faces"))
		{
			const rapidjson::Value& array_val = d["vec_id_faces"];
			for (rapidjson::Value::ConstMemberIterator itr = array_val.MemberBegin();
				itr != array_val.MemberEnd(); ++itr)
			{
				std::string id_string = itr->name.GetString();
				int id = atoi(id_string.c_str());

				cxShape cx_shape;
				cx_shape.fromJson(itr->value, 1.0);

				vec_id_faces.push_back(std::make_pair(id, cx_shape));
			}
		}

		if (d.HasMember("ovv_ids"))
		{
			const rapidjson::Value& array_val = d["ovv_ids"];
			for (rapidjson::Value::ConstValueIterator itr = array_val.Begin();
				itr != array_val.End(); ++itr)
			{
				int id = itr->GetInt();
				ovv_ids.push_back(id);
			}
		}



		//开始导入操作
		std::vector<std::pair<int, int>> part_tags;
		std::vector<std::pair<int, int>> solid_tags;
		std::vector<std::pair<int, int>> face_tags;

		//特殊的只保留part的信息
		std::vector<std::pair<int, std::string>> vec_tag_solidName;
		std::map<int, std::vector<std::pair<std::string, std::vector<int>>>> part_map_solidTag_subFaceGroups;

		//输出<id-tag>对应表
		std::vector<std::pair<int, int>> vec_pair_idTag;
		
		int cnt(0);
		for (int i = 0; i < vec_id_parts.size(); i++)
		{
			void* inner_shape = &(vec_id_parts[i].second.occshape());
			gmsh::vectorpair temp_outDimTags;
			gmsh::model::occ::importShapesNativePointer(inner_shape, temp_outDimTags, true);
			gmsh::model::occ::synchronize();
			for (auto& temp_pair : temp_outDimTags)
			{
				part_tags.push_back(temp_pair);

				//name
				vec_tag_solidName.push_back(std::make_pair(temp_pair.second, vec_part_name[i]));

				//subFaceGroup
				std::vector<int> temp_face_tags;
				cx_getFaceTagsFromSolidTag(temp_pair.second, temp_face_tags);
				std::pair<std::string, std::vector<int>> temp_pair2("default", temp_face_tags);
				part_map_solidTag_subFaceGroups[temp_pair.second].push_back(temp_pair2);
			}
			cnt++;
		}
	
		for (auto& pair : vec_id_solids)
		{
			void* inner_shape = &(pair.second.occshape());
			gmsh::vectorpair temp_outDimTags;
			gmsh::model::occ::importShapesNativePointer(inner_shape, temp_outDimTags, true);
			//gmsh::model::occ::synchronize();

			for (auto& temp_pair : temp_outDimTags)
			{
				solid_tags.push_back(temp_pair);
			}	
		}

		for (auto& pair : vec_id_faces)
		{
			void* inner_shape = &(pair.second.occshape());
			gmsh::vectorpair temp_outDimTags;
			gmsh::model::occ::importShapesNativePointer(inner_shape, temp_outDimTags, true);
			for (auto& temp_pair : temp_outDimTags)
			{
				face_tags.push_back(temp_pair);
			}
		}
		gmsh::model::occ::synchronize();

		//输出solid和face的id-tag对照表
		OutputIdTagTable(solid_tags, face_tags, ovv_ids, config_script_path);

		//输出part信息表
		OutputPartTable(vec_tag_solidName, part_map_solidTag_subFaceGroups, config_script_path);
		
		//执行fragment操作
		std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>> ovv_table;
		cx_fragment(solid_tags, face_tags, ovv_ids, ovv_table);
		{
			OutputOvvTable(ovv_table, config_script_path);

			OutputSolidTag2FaceTagsTable(solid_tags, ovv_table, config_script_path);
		}


		//连通域筛选操作
		std::vector<int> ids;
		if (d.HasMember("interfaceIds"))
		{
			const rapidjson::Value& array_val = d["interfaceIds"];
			for (rapidjson::Value::ConstValueIterator itr = array_val.Begin();
				itr != array_val.End(); ++itr)
			{
				int id = itr->GetInt();
				ids.push_back(id);
			}
		}

		std::vector<double> seedPos; seedPos.reserve(3);
		if (d.HasMember("seedPos"))
		{
			const rapidjson::Value& array_val = d["seedPos"];
			for (rapidjson::Value::ConstValueIterator itr = array_val.Begin();
				itr != array_val.End(); ++itr)
			{
				int id = itr->GetDouble();
				seedPos.push_back(id);
			}
		}

		FilterConnectedDomain(ids, seedPos);

		
#pragma region	//设置选项
		int method_type;
		int smooth_steps;
		//
		double meshSizeFactor;
		double minElementSize;
		double maxElementSize;
		double curElementSize;
		//
		bool bmeshSizeFromPoints;
		bool bmeshSizeExtendFromBoundary;
		bool boptimize;
		bool boptimizeNetgen;


		if (d.HasMember("options"))
		{
			method_type = d["options"]["method_type"].GetInt();
			smooth_steps = d["options"]["smooth_steps"].GetInt();

			meshSizeFactor = d["options"]["meshSizeFactor"].GetDouble();
			minElementSize = d["options"]["minElementSize"].GetDouble();
			maxElementSize = d["options"]["maxElementSize"].GetDouble();
			curElementSize = d["options"]["curElementSize"].GetDouble();

			bmeshSizeFromPoints = d["options"]["bmeshSizeFromPoints"].GetBool();
			bmeshSizeExtendFromBoundary = d["options"]["bmeshSizeExtendFromBoundary"].GetBool();
			boptimize = d["options"]["boptimize"].GetBool();
			boptimizeNetgen = d["options"]["boptimizeNetgen"].GetBool();

			//设置
			switch (method_type)
			{
			case 0:
				m_option_algorithm3D = 1; //1 : Delaunay
				break;
			case 1:
				m_option_algorithm3D = 4; //4 : Frontal
				break;
			case 2:
				m_option_algorithm3D = 10; //10 : HXT
				break;
			case 3:
				m_option_algorithm3D = 7; //7 : MMG3D
				break;
			case 4:
				m_option_algorithm3D = 3; //3 : Initial mesh only
				break;
			default:
				break;
			}

			m_option_smoothSteps = smooth_steps;
			m_option_meshSizeFactor = meshSizeFactor;
			m_option_minElementSize = minElementSize;
			m_option_maxElementSize = maxElementSize;

			m_option_meshSizeFromPoints = bmeshSizeFromPoints;
			m_option_meshSizeExtendFromBoundary = bmeshSizeExtendFromBoundary;
			m_option_optimize = boptimize;
			m_option_optimizeNetgen = boptimizeNetgen;

			//set all points mesh size
			std::vector<std::pair<int, int>> pnts_dimTags;
			cx_getEntities(pnts_dimTags, 0);
			cx_setMeshSize(pnts_dimTags, curElementSize);
		}
#pragma endregion

		//设置mesh size field
		SetMeshSize(d);
	

		//执行剖分算法
		cx_generate(3);


		/*>>>>>.msh file write*****************************************************************************/
		cx_write_mshFormate(config_script_path);
		/*<<<<<*****************************************************************************/


		//结束
		//cx_clear();
		//cx_finalize();

		return 0; //0: succeed
}

	void oxGmsh::OutputOvvTable(std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>>& ovv_table, const std::string& str_outPath)
	{
		//输出ovv_table
		rapidjson::Document d;
		rapidjson::Document::AllocatorType& allocator = d.GetAllocator();
		d.SetObject();


		{
			rapidjson::Value array_val(rapidjson::kArrayType);


			for (int i = 0; i < ovv_table.size(); i++)
			{
				rapidjson::Value pair_val(rapidjson::kObjectType);

				//pair_first
				rapidjson::Value pair_first(rapidjson::kObjectType);
				{
					rapidjson::Value pair_first_first(rapidjson::kStringType);
					pair_first_first.SetString(std::to_string(ovv_table[i].first.first), allocator);

					rapidjson::Value pair_first_second(rapidjson::kNumberType);
					pair_first_second.SetInt(ovv_table[i].first.second);

					pair_first.AddMember(pair_first_first, pair_first_second, allocator);
				}
				pair_val.AddMember("pair_first", pair_first, allocator);

				//pair_second
				std::vector<std::pair<int, int>>& vec_second = ovv_table[i].second;
				rapidjson::Value pair_second(rapidjson::kArrayType);
				{
					for (int j = 0; j < vec_second.size(); j++)
					{
						rapidjson::Value pair_second_each(rapidjson::kObjectType);
						{
							rapidjson::Value pair_second_each_first(rapidjson::kStringType);
							pair_second_each_first.SetString(std::to_string(vec_second[j].first), allocator);

							rapidjson::Value pair_second_each_second(rapidjson::kNumberType);
							pair_second_each_second.SetInt(vec_second[j].second);

							pair_second_each.AddMember(pair_second_each_first, pair_second_each_second, allocator);
						}
						pair_second.PushBack(pair_second_each, allocator);
					}
				}
				pair_val.AddMember("pair_second", pair_second, allocator);

				array_val.PushBack(pair_val, allocator);
			}
			d.AddMember("ovv_table", array_val, allocator);
		}

		//输出json文件
		std::string str_gmsh_config = str_outPath + "/" + "ovv_table.json";
		std::ofstream ofs(str_gmsh_config);
		rapidjson::OStreamWrapper osw(ofs);

		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		d.Accept(writer);
	}

	void oxGmsh::OutputIdTagTable(std::vector<std::pair<int, int>>& solid_tags, std::vector<std::pair<int, int>> face_tags, std::vector<int>& ovv_ids, const std::string& str_outPath)
	{
		//输出id-tag对应表
		std::vector <std::pair<int, int>> threeDim_id_tag;
		for (int i = 0; i < solid_tags.size(); i++)
		{
			threeDim_id_tag.push_back(std::make_pair(ovv_ids[i],solid_tags[i].second));
		}

		std::vector <std::pair<int, int>> twoDim_id_tag;
		for (int i = 0; i < face_tags.size(); i++)
		{
			twoDim_id_tag.push_back(std::make_pair(ovv_ids[solid_tags.size() + i], face_tags[i].second));
		}


		rapidjson::Document d;
		rapidjson::Document::AllocatorType& allocator = d.GetAllocator();
		d.SetObject();


		{
			rapidjson::Value threeDim_val(rapidjson::kArrayType);
			//
			for (int i = 0; i < threeDim_id_tag.size(); i++)
			{
				rapidjson::Value id_val(rapidjson::kStringType);
				id_val.SetString(std::to_string(threeDim_id_tag[i].first), allocator);

				rapidjson::Value tag_val(rapidjson::kNumberType);
				tag_val.SetInt(threeDim_id_tag[i].second);

				rapidjson::Value obj_val(rapidjson::kObjectType);
				obj_val.AddMember(id_val, tag_val, allocator);
				//
				threeDim_val.PushBack(obj_val, allocator);
			}
			d.AddMember("3D", threeDim_val, allocator);
		}
		{
			rapidjson::Value twoDim_val(rapidjson::kArrayType);
			//
			for (int i = 0; i < twoDim_id_tag.size(); i++)
			{
				rapidjson::Value id_val(rapidjson::kStringType);
				id_val.SetString(std::to_string(twoDim_id_tag[i].first), allocator);

				rapidjson::Value tag_val(rapidjson::kNumberType);
				tag_val.SetInt(twoDim_id_tag[i].second);

				rapidjson::Value obj_val(rapidjson::kObjectType);
				obj_val.AddMember(id_val, tag_val, allocator);
				//
				twoDim_val.PushBack(obj_val, allocator);
			}
			d.AddMember("2D", twoDim_val, allocator);
		}

		//输出json文件
		std::string str_gmsh_config = str_outPath + "/" + "id_tag.json";
		std::ofstream ofs(str_gmsh_config);
		rapidjson::OStreamWrapper osw(ofs);

		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		d.Accept(writer);
	}

	void oxGmsh::OutputSolidTag2FaceTagsTable(std::vector<std::pair<int, int>>& solid_tags, 
		std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>>& ovv_table, 
		const std::string& str_outPath)
	{
		//获取fragment操作后的solid的tag列表
		std::set<int> temp_solid_tags;
		for (int i=0; i<solid_tags.size(); i++)
		{
			std::vector<std::pair<int, int>>& second_vecpair = ovv_table[i].second;
			for (auto& pair_each : second_vecpair)
			{
				temp_solid_tags.insert(pair_each.second);
			}
		}

		//查询每个solid tag对应的face tag list
		std::vector<std::pair<int, std::vector<int>>> temp_solidtag_facetags;
		for (auto& solid_tag : temp_solid_tags)
		{
			std::vector<int> face_tags;
			cx_getFaceTagsFromSolidTag(solid_tag, face_tags);
			temp_solidtag_facetags.push_back(std::make_pair(solid_tag, face_tags));
		}


		rapidjson::Document d;
		rapidjson::Document::AllocatorType& allocator = d.GetAllocator();
		d.SetObject();

		{
			rapidjson::Value array_val(rapidjson::kArrayType);
			for (auto& pair : temp_solidtag_facetags)
			{
				rapidjson::Value solidtag_val(rapidjson::kStringType);
				solidtag_val.SetString(std::to_string(pair.first), allocator);

				rapidjson::Value facearray_val(rapidjson::kArrayType);
				for (auto& facetag : pair.second)
				{
					rapidjson::Value tag_val(rapidjson::kNumberType);
					tag_val.SetInt(facetag);
					facearray_val.PushBack(tag_val, allocator);
				}

				rapidjson::Value obj_val(rapidjson::kObjectType);
				obj_val.AddMember(solidtag_val, facearray_val, allocator);

				array_val.PushBack(obj_val, allocator);
			}

			d.AddMember("solidtag_facetags", array_val, allocator);
		}


		//输出json文件
		std::string str_gmsh_config = str_outPath + "/" + "solidtag_facetags_table.json";
		std::ofstream ofs(str_gmsh_config);
		rapidjson::OStreamWrapper osw(ofs);

		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		d.Accept(writer);
	}

	void oxGmsh::OutputPartTable(std::vector<std::pair<int, std::string>>& vec_tag_solidName, std::map<int, std::vector<std::pair<std::string, std::vector<int>>>>& part_map_solidTag_subFaceGroups, const std::string& str_outPath)
	{
		rapidjson::Document d;
		rapidjson::Document::AllocatorType& allocator = d.GetAllocator();
		d.SetObject();

		{
			rapidjson::Value array_val(rapidjson::kArrayType);
			for (auto& pair : vec_tag_solidName)
			{
				rapidjson::Value tag_val(rapidjson::kStringType);
				tag_val.SetString(std::to_string(pair.first), allocator);

				rapidjson::Value name_val(rapidjson::kStringType);
				name_val.SetString(pair.second, allocator);

				rapidjson::Value pair_int_str(rapidjson::kObjectType);
				pair_int_str.AddMember(tag_val, name_val, allocator);
				array_val.PushBack(pair_int_str, allocator);
			}
			d.AddMember("part_info_name", array_val, allocator);
		}

		{
			rapidjson::Value array_val(rapidjson::kArrayType);
			for (auto& pair : part_map_solidTag_subFaceGroups)
			{
				rapidjson::Value arrayItem_val(rapidjson::kObjectType);

				rapidjson::Value solidtag_val(rapidjson::kStringType);
				solidtag_val.SetString(std::to_string(pair.first), allocator);

				rapidjson::Value vec_val(rapidjson::kArrayType);
				for (auto& vec_item : pair.second)
				{
					rapidjson::Value pair_val(rapidjson::kObjectType);

					//string
					rapidjson::Value str_val(rapidjson::kStringType);
					str_val.SetString(vec_item.first, allocator);

					rapidjson::Value vec_int_val(rapidjson::kArrayType);
					for (auto& item : vec_item.second)
					{
						vec_int_val.PushBack(item, allocator);
					}

					pair_val.AddMember(str_val, vec_int_val, allocator);
					vec_val.PushBack(pair_val, allocator);
				}

				
				arrayItem_val.AddMember(solidtag_val, vec_val, allocator);

				array_val.PushBack(arrayItem_val, allocator);
			}

			d.AddMember("part_info_table", array_val, allocator);
		}


		//输出json文件
		std::string str_gmsh_config = str_outPath + "/" + "part_info_table.json";
		std::ofstream ofs(str_gmsh_config);
		rapidjson::OStreamWrapper osw(ofs);

		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		d.Accept(writer);
	}

	int oxGmsh::FilterConnectedDomain(std::vector<int>& ids, std::vector<double>& seedPos)
	{
		//1. 查找交界面
		//std::vector<int> ids;
		//GetInterfaceIds(ids);


		//2. 查找physical Groups, fragment后的face tag
		std::vector<int> interface_tags;
		for (auto& id : ids)
		{
			std::vector<int> temp_tags;
			cx_getEntitiesForPhysicalGroup(2, id, temp_tags);
			interface_tags.insert(interface_tags.end(), temp_tags.begin(), temp_tags.end());
		}


		//3. 查找face-tag对应的occ-face
		std::vector<cxShape> interface_occfaces;
		for (auto& tag : interface_tags)
		{
			cxShape topoface;
			cx_getOccFaceFromTag(tag, topoface);
			interface_occfaces.push_back(topoface);
		}


		//4. 获取场景中的所有三维solids
		std::vector<std::pair<int, int>> vec_solidTags;
		std::vector<cxShape> vec_solidOccs;
		cx_getEntities(vec_solidTags, 3);
		for (auto& solid_tag : vec_solidTags)
		{
			cxShape toposolid;
			if (cx_getOccSolidFromTag(solid_tag.second, toposolid))
			{
				vec_solidOccs.push_back(toposolid);
			}
		}
		assert(vec_solidTags.size() == vec_solidOccs.size());


		//5. 查找interface的occ-face两侧的solid-tag对
		std::vector<std::pair<int, int>> solid_pairs;
		for (auto& occface : interface_occfaces)
		{
			std::pair<int, int> solidIdxPair;
			if (FindFaceSideSolids(occface, vec_solidOccs, solidIdxPair))
			{
				solid_pairs.push_back(std::make_pair(vec_solidTags[solidIdxPair.first].second, vec_solidTags[solidIdxPair.second].second));
			}
		}


		//6. 查找材料点所在的solid tag，并查找相关的连通域tag
		int material_cur_tag(-1);
		//double* pos = _region->tetraDivMethod().basicSize().position();
		for (int i = 0; i < vec_solidOccs.size(); i++)
		{
			if (IsPointInShape(qxGeom::qxPoint(seedPos[0], seedPos[1], seedPos[2]), vec_solidOccs[i]))
			{
				material_cur_tag = vec_solidTags[i].second;
			}
		}


		//7. 查找连通的所有solid tag
		std::set<int> set_result_regions{ material_cur_tag };
		int pre_cnt(-1);
		do
		{
			//记录上次循环的个数
			pre_cnt = set_result_regions.size();

			for (int i = 0; i < solid_pairs.size(); )
			{
				if ((set_result_regions.count(solid_pairs[i].first) != 0) || (set_result_regions.count(solid_pairs[i].second) != 0))
				{
					set_result_regions.insert(solid_pairs[i].first);
					set_result_regions.insert(solid_pairs[i].second);

					solid_pairs.erase(solid_pairs.begin() + i);
					continue;
				}
				i++;
			}
		} while (set_result_regions.size() != pre_cnt);


		//移除不在连通域中的3D-tag
		for (auto& pair : vec_solidTags)
		{
			if (set_result_regions.count(pair.second) == 0)
			{
				std::vector<std::pair<int, int> > dimTags{ pair };
				cx_removeEntities(dimTags);
			}
		}


		return 0;
	}

	void oxGmsh::SetMeshSize(rapidjson::Document& d)
	{
		if (d.HasMember("mesh_size_field"))
		{
			const rapidjson::Value& object_val = d["mesh_size_field"];
			for (rapidjson::Value::ConstMemberIterator itr = object_val.MemberBegin();
				itr != object_val.MemberEnd(); ++itr)
			{
				std::string str_name = itr->name.GetString();
				const rapidjson::Value& obj_val = itr->value;
				if (str_name == "Box")
				{
					double val_thickness(0.0), VIn(0.0), VOut(0.0), xMin(0.0), xMax(0.0), yMin(0.0), yMax(0.0), zMin(0.0), zMax(0.0);

					val_thickness = obj_val["val_thickness"].GetDouble();
					VIn = obj_val["VIn"].GetDouble();
					VOut = obj_val["VOut"].GetDouble();
					xMin = obj_val["xMin"].GetDouble();
					xMax = obj_val["xMax"].GetDouble();
					yMin = obj_val["yMin"].GetDouble();
					yMax = obj_val["yMax"].GetDouble();
					zMin = obj_val["zMin"].GetDouble();
					zMax = obj_val["zMax"].GetDouble();

					int field_tag = cx_field_add("Box");
					cx_field_setNumber(field_tag, "Thickness", val_thickness);
					cx_field_setNumber(field_tag, "VIn", VIn);
					cx_field_setNumber(field_tag, "VOut", VOut);
					cx_field_setNumber(field_tag, "xMin", xMin);
					cx_field_setNumber(field_tag, "xMax", xMax);
					cx_field_setNumber(field_tag, "yMin", yMin);
					cx_field_setNumber(field_tag, "yMax", yMax);
					cx_field_setNumber(field_tag, "zMin", zMin);
					cx_field_setNumber(field_tag, "zMax", zMax);

					cx_field_setAsBackgroundMesh(field_tag);
				}
				else if (str_name == "Ball")
				{
					double val_thickness(0.0), val_radius(0.0), VIn(0.0), VOut(0.0), XCenter(0.0), YCenter(0.0), ZCenter(0.0);

					val_thickness = obj_val["val_thickness"].GetDouble();
					val_radius = obj_val["val_radius"].GetDouble();
					VIn = obj_val["VIn"].GetDouble();
					VOut = obj_val["VOut"].GetDouble();
					XCenter = obj_val["XCenter"].GetDouble();
					YCenter = obj_val["YCenter"].GetDouble();
					ZCenter = obj_val["ZCenter"].GetDouble();

					int field_tag = cx_field_add("Ball");
					cx_field_setNumber(field_tag, "Thickness", val_thickness);
					cx_field_setNumber(field_tag, "Radius", val_radius);
					cx_field_setNumber(field_tag, "VIn", VIn);
					cx_field_setNumber(field_tag, "VOut", VOut);
					cx_field_setNumber(field_tag, "XCenter", XCenter);
					cx_field_setNumber(field_tag, "YCenter", YCenter);
					cx_field_setNumber(field_tag, "ZCenter", ZCenter);

					cx_field_setAsBackgroundMesh(field_tag);
				}
				else if (str_name == "Cylinder")
				{
					double val_radius(0.0), VIn(0.0), VOut(0.0), XCenter(0.0), YCenter(0.0), ZCenter(0.0), XAxis(0.0), YAxis(0.0), ZAxis(0.0);

					val_radius = obj_val["val_radius"].GetDouble();
					VIn = obj_val["VIn"].GetDouble();
					VOut = obj_val["VOut"].GetDouble();
					XCenter = obj_val["XCenter"].GetDouble();
					YCenter = obj_val["YCenter"].GetDouble();
					ZCenter = obj_val["ZCenter"].GetDouble();
					XAxis = obj_val["XAxis"].GetDouble();
					YAxis = obj_val["YAxis"].GetDouble();
					ZAxis = obj_val["ZAxis"].GetDouble();

					int field_tag = cx_field_add("Cylinder");
					cx_field_setNumber(field_tag, "Radius", val_radius);
					cx_field_setNumber(field_tag, "VIn", VIn);
					cx_field_setNumber(field_tag, "VOut", VOut);
					cx_field_setNumber(field_tag, "XCenter", XCenter);
					cx_field_setNumber(field_tag, "YCenter", YCenter);
					cx_field_setNumber(field_tag, "ZCenter", ZCenter);
					cx_field_setNumber(field_tag, "XAxis", XAxis);
					cx_field_setNumber(field_tag, "YAxis", YAxis);
					cx_field_setNumber(field_tag, "ZAxis", ZAxis);

					cx_field_setAsBackgroundMesh(field_tag);
				}

				m_option_meshSizeFromPoints = false;
				m_option_meshSizeExtendFromBoundary = false;
			}
		}


		if (d.HasMember("mesh_size_field_faceLine"))
		{
			const rapidjson::Value& object_val = d["mesh_size_field_faceLine"];

			//face or line
			std::vector<int> vec_faceIds;
			std::vector<double> vec_facetags;
			std::vector<int> vec_faceBelongSolidIds;
			std::vector<double> vec_edgetags;
			if (object_val.HasMember("SurfacesList"))
			{
				const rapidjson::Value& surface_val = object_val["SurfacesList"];
				for (rapidjson::Value::ConstValueIterator itr = surface_val.Begin();
					itr != surface_val.End(); ++itr)
				{
					vec_faceIds.push_back(itr->GetInt());
				}

				//转换tag
				for (auto& faceId : vec_faceIds)
				{
					std::vector<int> tags;
					cx_getEntitiesForPhysicalGroup(2, faceId, tags);
					for (auto& facetag : tags)
					{
						vec_facetags.push_back(facetag);
					}
				}
			}

			if (object_val.HasMember("CurvesList"))
			{
				const rapidjson::Value& surface_val = object_val["CurvesList"];
				for (rapidjson::Value::ConstValueIterator itr = surface_val.Begin();
					itr != surface_val.End(); ++itr)
				{
					vec_faceBelongSolidIds.push_back(itr->GetInt());
				}
				/*
				//转换tag
				for (auto& edge_shape : vec_faceBelongSolidIds)
				{
				int edge_tag(-1);
				if (cx_getModelTagFromEdge(&edge_shape.occshape(), edge_tag))
				{
				vec_edgetags.push_back(edge_tag);
				}
				}
				*/
			}

			//Threshold域
			std::vector<double> vec_threshold_vals;
			if (object_val.HasMember("Threshold"))
			{
				const rapidjson::Value& threshold_val = object_val["Threshold"];
				for (rapidjson::Value::ConstValueIterator itr = threshold_val.Begin();
					itr != threshold_val.End(); ++itr)
				{
					vec_threshold_vals.push_back(itr->GetDouble());
				}
			}


			//set field
			cx_field_add("Distance", 1);
			if (vec_facetags.size() != 0)
			{
				cx_field_setNumbers(1, "SurfacesList", vec_facetags);
			}
			if (vec_edgetags.size() != 0)
			{
				cx_field_setNumbers(1, "CurvesList", vec_edgetags);
			}
			cx_field_setNumber(1, "Sampling", 100);

			//threshold field
			cx_field_add("Threshold", 2);
			cx_field_setNumber(2, "InField", 1);
			cx_field_setNumber(2, "SizeMin", vec_threshold_vals[0]);
			cx_field_setNumber(2, "SizeMax", vec_threshold_vals[1]);
			cx_field_setNumber(2, "DistMin", vec_threshold_vals[2]);
			cx_field_setNumber(2, "DistMax", vec_threshold_vals[3]);

			cx_field_setAsBackgroundMesh(2);

			m_option_meshSizeFromPoints = false;
			m_option_meshSizeExtendFromBoundary = false;
		}
	}

	void oxGmsh::EmitLoggerInfoAfterAnchor(const std::string& str_find_anchor)
	{
		std::vector<std::string> vlog;
		cx_logger_get(vlog);
		auto iter_find_last = std::find(vlog.begin(), vlog.end(), str_find_anchor);
		if (iter_find_last != vlog.end())
		{
			iter_find_last++;
			std::vector<std::string> temp_vlog(iter_find_last, vlog.end());
			vlog.clear();
			vlog = temp_vlog;
		}
		//
		if (vlog.size() != 0)
		{
			std::string info;
			for (auto& str : vlog)
			{
				info += str;
				info += "\n";
			}
			//emit NotifyInfo(E_Info, UIUtitls::S2Q(info));
			std::cout << info << std::endl;
		}
	}

	void oxGmsh::EmitLoggerInfo()
	{
		std::vector<std::string> vlog;
		cx_logger_get(vlog);
		if (vlog.size() != 0)
		{
			std::string info;
			for (auto& str : vlog)
			{
				info += str;
				info += "\n";
			}
			//emit NotifyInfo(E_Info, UIUtitls::S2Q(info));
			std::cout << info << std::endl;
		}
	}

oxGmsh* oxGmsh::instance()
	{
		static oxGmsh gmsh_instance;
		return &gmsh_instance;
	}

	bool oxGmsh::cx_isInitialized()
	{
		return gmsh::isInitialized() == 1;
	}

	void oxGmsh::cx_initialize()
	{
		gmsh::initialize();
	}

	void oxGmsh::cx_reInitialize()
	{
		gmsh::reInitialize();
	}

	void oxGmsh::cx_importShapesNativePointer(std::vector<cxShape>& vec_occShapeNatives)
	{
		for (int i = 0; i < vec_occShapeNatives.size(); i++)
		{
			void* inner_shape = &(vec_occShapeNatives[i].occshape());
			gmsh::vectorpair temp_outDimTags;
			gmsh::model::occ::importShapesNativePointer(inner_shape, temp_outDimTags, true);
		}
		gmsh::model::occ::synchronize();
	}


	void oxGmsh::cx_fragment(std::vector<std::pair<int, int>>& solid_tags, std::vector<std::pair<int, int>>& face_tags, std::vector<int>& ovv_ids, std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>>& ovv_table)
	{
		//执行嵌入过程
		std::vector<std::pair<int, int> > outDimTags;
		std::vector<std::vector<std::pair<int, int>>> ovv;
		gmsh::model::occ::fragment(solid_tags, face_tags, outDimTags, ovv);
		gmsh::model::occ::synchronize();


		//赋值physical group
		for (int i = 0; i < ovv.size(); i++)
		{
			if (ovv[i].size() == 0)
				continue;

			std::vector<int> ovv_tags;
			for (auto& pair : ovv[i])
				ovv_tags.push_back(pair.second);

			int ovv_dim = ovv[i][0].first;
			gmsh::model::addPhysicalGroup(ovv_dim, ovv_tags, ovv_ids[i]);
		}

		//输出fragment前后的ovv对照表
		//std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>> ovv_table;
		for (int i=0; i<solid_tags.size(); i++)
		{
			ovv_table.push_back(std::make_pair(solid_tags[i], ovv[i]));
		}
		for (int i = 0; i < face_tags.size(); i++)
		{
			ovv_table.push_back(std::make_pair(face_tags[i], ovv[solid_tags.size() + i]));
		}
	}


	void oxGmsh::cx_generate(int dim)
	{
		//Option
		gmsh::option::setNumber("Mesh.Algorithm3D", m_option_algorithm3D);
		gmsh::option::setNumber("Mesh.Smoothing", m_option_smoothSteps);
		gmsh::option::setNumber("Mesh.MeshSizeFactor", m_option_meshSizeFactor);
		gmsh::option::setNumber("Mesh.MeshSizeMin", m_option_minElementSize);
		gmsh::option::setNumber("Mesh.MeshSizeMax", m_option_maxElementSize);
		//
		gmsh::option::setNumber("Mesh.MeshSizeExtendFromBoundary", m_option_meshSizeExtendFromBoundary);
		gmsh::option::setNumber("Mesh.Optimize", m_option_optimize);
		gmsh::option::setNumber("Mesh.OptimizeNetgen", m_option_optimizeNetgen);


		if (dim == 3)
		{
			gmsh::model::mesh::generate(3);
		}
		else if (dim == 2)
		{
			gmsh::model::mesh::generate(2);
		}
	}

	void oxGmsh::cx_write_mshFormate(std::string file_path)
	{
		//.msh文件渲染Mesh使用，不缩小
		std::string msh_file = file_path + "/Mesh.msh";
		gmsh::write(msh_file);
	}

	void oxGmsh::cx_write_meshFormate(std::string file_path)
	{
		//缩小Mesh于1000分之一（从mm到m单位）//.mesh文件缩小1000倍，给求解器使用
		std::vector<double> affineTransform{ 0.001, 0.0, 0.0, 0.0, \
			0.0, 0.001, 0.0, 0.0, \
			0.0, 0.0, 0.001, 0.0, \
			0.0, 0.0, 0.0, 1.0 };
		gmsh::model::mesh::affineTransform(affineTransform);

		std::string mesh_file = file_path + "\\Mesh.mesh";
		meshFormate_write(mesh_file);
	}

	void oxGmsh::cx_finalize()
	{
		gmsh::finalize();
	}

	void oxGmsh::cx_clear()
	{
		gmsh::clear();
	}

	void oxGmsh::cx_setMeshSize(const std::vector<std::pair<int, int> >& dimTags, const double size)
	{
		gmsh::model::mesh::setSize(dimTags, size);
	}

	int oxGmsh::cx_field_add(const std::string & fieldType, const int tag /*= -1*/)
	{
		return gmsh::model::mesh::field::add(fieldType, tag);
	}

	void oxGmsh::cx_field_setNumber(const int tag, const std::string & option, const double value)
	{
		gmsh::model::mesh::field::setNumber(tag, option, value);
	}

	void oxGmsh::cx_field_setNumbers(const int tag, const std::string & option, const std::vector<double> & value)
	{
		gmsh::model::mesh::field::setNumbers(tag, option, value);
	}

	void oxGmsh::cx_field_setAsBackgroundMesh(const int tag)
	{
		gmsh::model::mesh::field::setAsBackgroundMesh(tag);
	}

	void oxGmsh::cx_getEntities(std::vector<std::pair<int, int>>& dimTags, const int dim /*= -1*/)
	{
		gmsh::model::getEntities(dimTags, dim);
	}

	void oxGmsh::cx_getValue(const int dim, const int tag, const std::vector<double> & parametricCoord, std::vector<double> & coord)
	{
		gmsh::model::getValue(dim, tag, parametricCoord, coord);
	}

	void oxGmsh::cx_getAllEdges(std::vector<std::size_t> & edgeTags, std::vector<std::size_t> & edgeNodes)
	{
		gmsh::model::mesh::getAllEdges(edgeTags, edgeNodes);
	}

	void oxGmsh::cx_getNodes(std::vector<std::size_t>& nodeTags, std::vector<double>& coord, std::vector<double>& parametricCoord, const int dim /*= -1*/, const int tag /*= -1*/, const bool includeBoundary /*= false*/, const bool returnParametricCoord /*= true*/)
	{
		gmsh::model::mesh::getNodes(nodeTags, coord, parametricCoord, dim, tag, includeBoundary, returnParametricCoord);
	}

	void oxGmsh::cx_getElements(std::vector<int> & elementTypes, std::vector<std::vector<std::size_t> > & elementTags, std::vector<std::vector<std::size_t> > & nodeTags, const int dim /*= -1*/, const int tag /*= -1*/)
	{
		gmsh::model::mesh::getElements(elementTypes, elementTags, nodeTags, dim, tag);
	}

	void oxGmsh::cx_getBoundary(const std::vector<std::pair<int, int> > & dimTags, std::vector<std::pair<int, int> > & outDimTags, const bool combined /*= true*/, const bool oriented /*= true*/, const bool recursive /*= false*/)
	{
		gmsh::model::getBoundary(dimTags, outDimTags, combined, oriented, recursive);
	}

	bool oxGmsh::cx_getModelTagFromSolid(void* toposhape, int& tag)
	{
		return gmsh::model::occ::GetModelTagFromSolid(toposhape, tag);
	}

	bool oxGmsh::cx_getModelTagFromFace(void* toposhape, int& tag)
	{
		return gmsh::model::occ::GetModelTagFromFace(toposhape, tag);
	}

	bool oxGmsh::cx_getModelTagFromEdge(void* toposhape, int& tag)
	{
		return gmsh::model::occ::GetModelTagFromEdge(toposhape, tag);
	}

	bool oxGmsh::cx_getFaceTagsFromSolidTag(int tag, std::vector<int>& face_tags)
	{
		return gmsh::model::occ::GetFaceTagsFromSolidTag(tag, face_tags);
	}

	bool oxGmsh::cx_getOccSolidFromTag(int tag, cxShape& toposolid)
	{
		TopoDS_Solid occ_solid;
		bool btrue = gmsh::model::occ::GetModelSolidFromTag(tag, occ_solid);
		toposolid.occshape() = occ_solid;
		return btrue;
	}

	bool oxGmsh::cx_getOccFaceFromTag(int tag, cxShape& topoface)
	{
		TopoDS_Face occ_face;
		bool btrue = gmsh::model::occ::GetModelFaceFromTag(tag, occ_face);
		topoface.occshape() = occ_face;
		return btrue;
	}

	void oxGmsh::cx_addPhysicalGroups(const int dim, const std::vector<int> & tags, const int tag /*= -1*/, const std::string & name /*= ""*/)
	{
		gmsh::model::addPhysicalGroup(dim, tags, tag, name);
	}

	void oxGmsh::cx_getEntitiesForPhysicalGroup(const int dim, const int tag, std::vector<int> & tags)
	{
		gmsh::model::getEntitiesForPhysicalGroup(dim, tag, tags);
	}


	void oxGmsh::cx_logger_write(const std::string& message, const std::string& level /*= "info"*/)
	{
		gmsh::logger::write(message, level);
	}

	void oxGmsh::cx_logger_start()
	{
		gmsh::logger::start();
	}

	void oxGmsh::cx_logger_get(std::vector<std::string>& log)
	{
		gmsh::logger::get(log);
	}

	void oxGmsh::cx_logger_stop()
	{
		gmsh::logger::stop();
	}

	void oxGmsh::cx_removeEntities(const std::vector<std::pair<int, int>>& dimTags, const bool recursive /*= false*/)
	{
		gmsh::model::removeEntities(dimTags, recursive);
	}

	void oxGmsh::getNodes(std::pair<int, int>& dim_tag, std::map<int, std::vector<double>>& map_nodes)
	{
		std::vector<std::size_t> nodeTags;
		std::vector<double> coord;
		std::vector<double> parametricCoord;
		//cxGmsh::instance()->
		cx_getNodes(nodeTags, coord, parametricCoord, dim_tag.first, dim_tag.second, true, false);

		for (int i = 0; i < nodeTags.size(); i++)
		{
			std::vector<double> vec_coord { coord[3 * i + 0], coord[3 * i + 1], coord[3 * i + 2] };
			map_nodes.insert(std::make_pair(nodeTags[i], vec_coord));
		}
	}

	void oxGmsh::getTriFaces(std::pair<int, int>& dim_tag, std::vector<std::vector<int>>& vec_triFaces)
	{
		std::vector<std::pair<int, int>> vec_dim2_dim_tags;
		//如果dim==3，需要转换成所有的dim==2的面来获取三角片
		if (dim_tag.first == 3)
		{
			std::vector<std::pair<int, int>> vec_pair{ dim_tag };
			std::vector<std::pair<int, int>> outDimTags;
			oxGmsh::instance()->cx_getBoundary(vec_pair, outDimTags, false, false);
			vec_dim2_dim_tags = outDimTags;
		}
		else if (dim_tag.first == 2)
		{
			vec_dim2_dim_tags.push_back(dim_tag);
		}
		else
			return;

		for (auto& pair : vec_dim2_dim_tags)
		{
			std::vector<int> elementTypes;
			std::vector<std::vector<std::size_t>> elementTags;
			std::vector<std::vector<std::size_t>> element_nodeTags;
			oxGmsh::instance()->cx_getElements(elementTypes, elementTags, element_nodeTags, pair.first, pair.second);
			if (elementTypes.size() == 1 && elementTypes[0] == 2)
			{
				for (int i = 0; i < elementTags[0].size(); i++)
				{
					std::vector<int> tri_face{ (int)element_nodeTags[0][3 * i + 0], (int)element_nodeTags[0][3 * i + 1], (int)element_nodeTags[0][3 * i + 2] };
					vec_triFaces.push_back(tri_face);
				}
			}
		}
	}


	void oxGmsh::getTetCells(std::pair<int, int>& dim_tag, std::vector<TetCell>& vec_tetCells)
	{
		std::vector<int> elementTypes;
		std::vector<std::vector<std::size_t>> elementTags;
		std::vector<std::vector<std::size_t>> element_nodeTags;
		oxGmsh::instance()->cx_getElements(elementTypes, elementTags, element_nodeTags, dim_tag.first, dim_tag.second);
		if (elementTypes.size() == 1 && elementTypes[0] == 4)
		{
			for (int i = 0; i < elementTags[0].size(); i++)
			{
				TetCell tet_cell(element_nodeTags[0][4 * i + 0], element_nodeTags[0][4 * i + 1], element_nodeTags[0][4 * i + 2], element_nodeTags[0][4 * i + 3]);
				vec_tetCells.push_back(tet_cell);
			}
		}
	}

	/*
	lio::cxMesh* cxGmsh::CreateEntityMesh(int dim, nemo::neShapeLeaf* pleaf)
	{
		//Temp:2023-1-16 
		//结构破坏

		//nodes
//		std::map<int, qxGeom::qxPoint> map_nodes;
		std::pair<int, int> all_nodes(-1, -1);
		std::map<int, std::vector<double>> map_nodes;
		//getNodes(all_nodes, map_nodes);
		//
		std::vector<qxGeom::qxPoint> vec_nodes;
		auto iter_pnt_last = --map_nodes.end();
		int pnt_cnt = iter_pnt_last->first;
		for (int i = 0; i <= pnt_cnt; i++)
		{
			if (map_nodes.count(i) != 0)
			{
//				vec_nodes.push_back(map_nodes[i]);
			}
			else
			{
//				vec_nodes.push_back(qxGeom::qxPoint(0.0, 0.0, 0.0));
			}
		}


		//triFace
		std::vector<std::vector<int>> vec_triFaces;
		if (dim == 3)
		{
			for (auto& child : pleaf->childs())
			{
				std::vector<int> entity_tags;
				cx_getEntitiesForPhysicalGroup(2, child->id(), entity_tags);
				for (auto& tag : entity_tags)
				{
					std::pair<int, int> child_pair(2, tag);

					std::vector<std::vector<int>> temp_triFaces;
					//std::vector<std::vector<int>> vec_triFaces;
					getTriFaces(child_pair, temp_triFaces);
					vec_triFaces.insert(vec_triFaces.end(), temp_triFaces.begin(), temp_triFaces.end());
				}
			}
		}
		else if (dim == 2)
		{
			std::vector<int> entity_tags;
			cx_getEntitiesForPhysicalGroup(2, pleaf->id(), entity_tags);
			for (auto& tag : entity_tags)
			{
				std::pair<int, int> child_pair(2, tag);

				std::vector<std::vector<int>> temp_triFaces;
				getTriFaces(child_pair, temp_triFaces);
				vec_triFaces.insert(vec_triFaces.end(), temp_triFaces.begin(), temp_triFaces.end());
			}
		}


		//tetCell
		std::vector<lio::TetCell> vec_tetCells;
		if (dim == 3)
		{
			std::vector<int> entity_tags;
			cx_getEntitiesForPhysicalGroup(3, pleaf->id(), entity_tags);
			for (auto& tag : entity_tags)
			{
				std::pair<int, int> solid_pair(3, tag);

				std::vector<lio::TetCell> temp_tetCells;
				getTetCells(solid_pair, temp_tetCells);
				vec_tetCells.insert(vec_tetCells.end(), temp_tetCells.begin(), temp_tetCells.end());
			}
		}


		//设置当前构件的Mesh对象
		lio::cxMesh* pmesh = new lio::cxMesh();
		pmesh->nodes() = vec_nodes;
//Temp: 2023-1-16
//		pmesh->triFaces() = vec_triFaces;
//		pmesh->tetCells() = vec_tetCells;

		return pmesh;
	}
	*/

	/*
	void cxGmsh::SetSyntheticalEntitiesMeshSize(std::vector<nemo::neShapeLeaf*> vec_leafs)
	{
		//get all dim=0 points
		std::vector<std::pair<int, int>> point_dimTags;
		cx_getEntities(point_dimTags, 0);

		std::vector<qxGeom::qxPoint> vec_pnt_coords;
		for (auto& pnt_dimTag : point_dimTags)
		{
			std::vector<double> parametricCoord;
			std::vector<double> coord;
			cx_getValue(pnt_dimTag.first, pnt_dimTag.second, parametricCoord, coord);
			vec_pnt_coords.push_back(qxGeom::qxPoint(coord[0], coord[1], coord[2]));
		}
		assert(point_dimTags.size() == vec_pnt_coords.size());

		std::map<nemo::neShapeLeaf*, std::set<qxGeom::qxPoint, lio::PointSetCompare>> map_pleaf_cadPnts;
		for (auto& pleaf : vec_leafs)
		{
			std::set<qxGeom::qxPoint, lio::PointSetCompare> set_pnts;
			lio::GetShapeAsSetPoints(*pleaf->shape(), set_pnts);
			map_pleaf_cadPnts.insert(std::make_pair(pleaf, set_pnts));
		}

		//分配各个点的MeshSize: 
		//case <在一个cad模型上>:		使用该模型(pleaf)的meshsize参数;
		//case <在多个cad模型上>:		使用共享多个模型的meshsize平均值;
		//case <一个模型上也不在>:		使用默认值m_option_ElementSize;

		for (int i = 0; i < point_dimTags.size(); i++)
		{
			auto& pnt = vec_pnt_coords[i];

			double rslt_size(0.0);
			for (auto map_pair : map_pleaf_cadPnts)
			{
				if (map_pair.second.count(pnt) != 0)
				{
//					if (fabs(rslt_size - 0.0) < 1.0e-6)
//						rslt_size = map_pair.first->meshSize();
//					else
//						rslt_size = 0.5*(rslt_size + map_pair.first->meshSize());
				}
			}

			if (fabs(rslt_size - 0.0) < 1.0e-6)
			{
// 				if (fabs(m_option_ElementSize - 0.0) > 1.0e-6)
// 				{
// 					rslt_size = m_option_ElementSize;
// 				}
// 				else
// 					assert(false);
			}

			cx_setMeshSize({ point_dimTags[i] }, rslt_size);
		}
	}
	*/

	void oxGmsh::meshFormate_write(std::string file_path)
	{
		std::ofstream ofs;
		ofs.open(file_path, std::ios::out);

		//header
		ofs << "MeshVersionFormatted 2" << std::endl;
		ofs << "Dimension" << std::endl;
		ofs << "3" << std::endl;

		//Vertices
		std::map<int, qxGeom::qxPoint> map_tag_node;
		std::pair<int, int> all_nodes(-1, -1);
//		getNodes(all_nodes, map_tag_node);

		auto iter_last_pnt = --map_tag_node.end();
		int pnt_tag_cnt = iter_last_pnt->first;

		ofs << "Vertices" << std::endl;
		ofs << pnt_tag_cnt << std::endl;
		for (int i = 1; i <= pnt_tag_cnt; i++)
		{
			qxGeom::qxPoint pnt(0.0, 0.0, 0.0);
			if (map_tag_node.count(i) != 0)
				pnt = map_tag_node[i];

			ofs << setiosflags(ios::right) << setw(19) << pnt.x() << "      " \
				<< setiosflags(ios::right) << setw(19) << pnt.y() << "      " \
				<< setiosflags(ios::right) << setw(19) << pnt.z() << "      " \
				<< i << std::endl;
		}

		//Edges
		/*
		std::vector<std::size_t> edgeTags;
		std::vector<std::size_t> edgeNodes;
		gmsh::model::mesh::getAllEdges(edgeTags, edgeNodes);

		ofs << "Edges" << std::endl;
		ofs << edgeTags.size() << std::endl;
		for (int i = 0; i < edgeTags.size(); i++)
		{
		ofs << edgeNodes[2 * i + 0] << " " << edgeNodes[2 * i + 1] << i << std::endl;
		}
		*/

		//Triangles
		//gmsh::vectorpair dimTags_2d;
		//cx_getEntities(dimTags_2d, 2);

		std::map<int, std::vector<TriFace>> map_id_triFaces;
		for (auto iter = map_id_dimTags_2d.begin(); iter != map_id_dimTags_2d.end(); iter++)
		{
			std::vector<TriFace> temp_triFace_list;
			for (int j = 0; j < iter->second.size(); j++)
			{
				std::vector<TriFace> temp_triFace;
//				getTriFaces(iter->second[j], temp_triFace);
				temp_triFace_list.insert(temp_triFace_list.end(), temp_triFace.begin(), temp_triFace.end());
			}

			map_id_triFaces.insert(std::make_pair(iter->first, temp_triFace_list));
		}


		ofs << "Triangles" << std::endl;
		int triFaceCnt(0);
		for (auto& pair : map_id_triFaces)
			triFaceCnt += pair.second.size();
		ofs << triFaceCnt << std::endl;
		for (auto iter = map_id_triFaces.begin(); iter != map_id_triFaces.end(); iter++)
		{
			for (int j = 0; j < iter->second.size(); j++)
			{
				ofs << iter->second[j].v_vtk_idxs[0] << " " << iter->second[j].v_vtk_idxs[1] << " " << iter->second[j].v_vtk_idxs[2] << " " << iter->first << std::endl;
			}
		}


		//Tetrahedra
		gmsh::vectorpair dimTags_3d;
		cx_getEntities(dimTags_3d, 3);

		std::map<int, std::vector<TetCell>> map_id_tetCells;
		for (auto iter = map_id_dimTags_3d.begin(); iter != map_id_dimTags_3d.end(); iter++)
		{
			std::vector<TetCell> temp_tetCell_list;
			for (int j = 0; j < iter->second.size(); j++)
			{
				std::vector<TetCell> temp_tetCell;
				getTetCells(iter->second[j], temp_tetCell);

				temp_tetCell_list.insert(temp_tetCell_list.end(), temp_tetCell.begin(), temp_tetCell.end());
			}

			map_id_tetCells.insert(std::make_pair(iter->first, temp_tetCell_list));
		}

		ofs << "Tetrahedra" << std::endl;
		int tetCellCnt(0);
		for (auto& pair : map_id_tetCells)
			tetCellCnt += pair.second.size();
		ofs << tetCellCnt << std::endl;
		for (auto iter = map_id_tetCells.begin(); iter != map_id_tetCells.end(); iter++)
		{
			for (int j = 0; j < iter->second.size(); j++)
			{
				ofs << iter->second[j].v_node_idxs[0] << " " << iter->second[j].v_node_idxs[1] << " " << iter->second[j].v_node_idxs[2] << " " << iter->second[j].v_node_idxs[3] << " " << iter->first << std::endl;
			}
		}


		ofs << "End" << std::endl;
		ofs.close();
	}


	void oxGmsh::FilterIdTags3D_improve(std::vector<std::vector<std::pair<int, int>>>& filter_vv, int iter_begin, int iter_end)
	{
		for (int i = iter_begin; i < iter_end; i++)
		{
			for (int j = iter_begin; j < iter_end; j++)
			{
				if (i == j)
					continue;

				RemoveIdTagInMutual(filter_vv[i], filter_vv[j]);
			}
		}
	}

	void oxGmsh::RemoveIdTagInMutual(std::vector<std::pair<int, int>>& one_vector, std::vector<std::pair<int, int>>& other_vector)
	{
		for (int i = 0; i < one_vector.size(); i++)
		{
			for (int j = 0; j < other_vector.size(); j++)
			{
				if (one_vector[i].first == other_vector[j].first &&
					one_vector[i].second == other_vector[j].second)
				{
					if (one_vector.size() > other_vector.size())
					{
						one_vector.erase(one_vector.begin() + i);
						i--;
					}
					else
					{
						other_vector.erase(other_vector.begin() + j);
						j--;
					}
				}
			}
		}
	}

	void oxGmsh::FilterIdTags2D_improve(std::vector<std::vector<std::pair<int, int>>>& filter_vv, int iter_begin, int iter_end, std::vector<std::vector<std::pair<int, int>>>& remove_vv)
	{
		for (auto& remove_vector : remove_vv)
		{
			for (auto& remove_idtag : remove_vector)
			{
				for (int i = iter_begin; i < iter_end; i++)
				{
					RemoveIdTagInVector(filter_vv[i], remove_idtag);
				}
			}
		}
	}

	void oxGmsh::RemoveIdTagInVector(std::vector<std::pair<int, int>>& sorce_vector, std::pair<int, int>& remove_idtag)
	{
		for (int i = 0; i < sorce_vector.size(); i++)
		{
			if (sorce_vector[i].first == remove_idtag.first &&
				sorce_vector[i].second == remove_idtag.second)
			{
				sorce_vector.erase(sorce_vector.begin() + i);
				i--;
			}
		}
	}


	oxGmsh::oxGmsh(void)
	{
		gmsh::initialize();
	}

	oxGmsh::~oxGmsh(void)
	{
		gmsh::finalize();
	}
}