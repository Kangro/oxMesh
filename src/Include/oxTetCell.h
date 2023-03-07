#pragma once
namespace oxm
{
	class TetCell
	{
	public:
		TetCell() {};
		TetCell(int id0, int id1, int id2, int id3) {
			v_node_idxs[0] = id0;
			v_node_idxs[1] = id1;
			v_node_idxs[2] = id2;
			v_node_idxs[3] = id3;
		}
		~TetCell() {};

		rapidjson::Value toJson(rapidjson::Document& doc);
		void fromJson(const rapidjson::Value& root, int version);

	public:
		int v_node_idxs[4];
	};

	class TriFace
	{
	public:
		TriFace() {};
		TriFace(int id0, int id1, int id2) {
			v_vtk_idxs[0] = id0;
			v_vtk_idxs[1] = id1;
			v_vtk_idxs[2] = id2;
		}
		~TriFace() {};

		rapidjson::Value toJson(rapidjson::Document& doc);
		void fromJson(const rapidjson::Value& root, int version);

	public:
		int v_vtk_idxs[3];
		int left_cell_idx{ -1 };
		int right_cell_idx{ -1 };
	};
}