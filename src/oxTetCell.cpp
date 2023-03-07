#include "stdafx.h"
#include "Include/oxTetCell.h"


namespace oxm
{


	rapidjson::Value TetCell::toJson(rapidjson::Document& doc)
	{
		return json::toArray_rapid(v_node_idxs, 4, doc);
	}

	void TetCell::fromJson(const rapidjson::Value& root, int version)
	{
		json::fromArray_rapid_int(root, v_node_idxs);
	}

	rapidjson::Value TriFace::toJson(rapidjson::Document& doc)
	{
		RAPIDJSON_NAMESPACE::Value root(RAPIDJSON_NAMESPACE::kObjectType);
		root.AddMember("idx", json::toArray_rapid(v_vtk_idxs, 3, doc), doc.GetAllocator());
		root.AddMember("left_cell_idx", left_cell_idx, doc.GetAllocator());
		root.AddMember("right_cell_idx", right_cell_idx, doc.GetAllocator());
		return root;
	}

	void TriFace::fromJson(const rapidjson::Value& root, int version)
	{
		json::fromArray_rapid_int(root["idx"], v_vtk_idxs);
		left_cell_idx = root["left_cell_idx"].GetInt();
		right_cell_idx = root["right_cell_idx"].GetInt();
	}

}