#pragma once
#include "cxShape.h"
#include "oxTetCell.h"
namespace oxm {

	class oxGmsh
	{

	/*脚本config方式调用*/
	public:
		bool ExeConfigScript(const std::string& config_script_path);
		
		void OutputOvvTable(std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>>& ovv_table, const std::string& str_outPath);
		void OutputIdTagTable(std::vector<std::pair<int, int>>& solid_tags, std::vector<std::pair<int, int>> face_tags, std::vector<int>& ovv_ids, const std::string& str_outPath);
		void OutputSolidTag2FaceTagsTable(std::vector<std::pair<int, int>>& solid_tags, std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>>& ovv_table, const std::string& str_outPath);
		void OutputPartTable(std::vector<std::pair<int, std::string>>& vec_tag_solidName, std::map<int, std::vector<std::pair<std::string, std::vector<int>>>>& part_map_solidTag_subFaceGroups, const std::string& str_outPath);

		int FilterConnectedDomain(std::vector<int>& ids, std::vector<double>& seedPos);
		void SetMeshSize(rapidjson::Document& d);

		void EmitLoggerInfoAfterAnchor(const std::string& str_find_anchor);
		void EmitLoggerInfo();

	public:
		static  oxGmsh* instance();
	public:

		/**///procedure
		bool cx_isInitialized();

		void cx_initialize();

		void cx_reInitialize();

		
		void cx_importShapesNativePointer(std::vector<cxShape>& vec_occShapeNatives);

		void cx_fragment(std::vector<std::pair<int, int>>& solid_tags, 
			std::vector<std::pair<int, int>>& face_tags, 
			std::vector<int>& ovv_ids, 
			std::vector<std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>>& ovv_table);
		
		void cx_generate(int dim);

		void cx_write_mshFormate(std::string file_path);
		void cx_write_meshFormate(std::string file_path);

		void cx_finalize();

		void cx_clear();
		/**/


		/**///option operation
		void cx_setMeshSize(const std::vector<std::pair<int, int> >& dimTags, const double size);
		/**/


		/**///field
		int cx_field_add(const std::string & fieldType, const int tag = -1);

		void cx_field_setNumber(const int tag, const std::string & option, const double value);

		void cx_field_setNumbers(const int tag, const std::string & option, const std::vector<double> & value);

		void cx_field_setAsBackgroundMesh(const int tag);
		/**/


		/**///get info
		void cx_getEntities(std::vector<std::pair<int, int>>& dimTags,
			const int dim = -1);

		void cx_getValue(const int dim,
			const int tag,
			const std::vector<double> & parametricCoord,
			std::vector<double> & coord);

		void cx_getAllEdges(std::vector<std::size_t> & edgeTags,
			std::vector<std::size_t> & edgeNodes);

		void cx_getNodes(std::vector<std::size_t>& nodeTags,
			std::vector<double>& coord,
			std::vector<double>& parametricCoord,
			const int dim = -1,
			const int tag = -1,
			const bool includeBoundary = false,
			const bool returnParametricCoord = true);

		void cx_getElements(std::vector<int> & elementTypes,
			std::vector<std::vector<std::size_t> > & elementTags,
			std::vector<std::vector<std::size_t> > & nodeTags,
			const int dim = -1,
			const int tag = -1);

		void cx_getBoundary(const std::vector<std::pair<int, int> > & dimTags,
							std::vector<std::pair<int, int> > & outDimTags,
							const bool combined = true,
							const bool oriented = true,
							const bool recursive = false);

		bool cx_getModelTagFromSolid(void* toposhape, int& tag);

		bool cx_getModelTagFromFace(void* toposhape, int& tag);

		bool cx_getModelTagFromEdge(void* toposhape, int& tag);

		bool cx_getFaceTagsFromSolidTag(int tag, std::vector<int>& face_tags);

		bool cx_getOccSolidFromTag(int tag, cxShape& toposolid);

		bool cx_getOccFaceFromTag(int tag, cxShape& topoface);
		/**/


		/**///physical group
		void cx_addPhysicalGroups(const int dim, const std::vector<int> & tags, const int tag = -1, const std::string & name = "");

		void cx_getEntitiesForPhysicalGroup(const int dim, const int tag, std::vector<int> & tags);


		/**///logger
		void cx_logger_write(const std::string& message, const std::string& level = "info");
		
		void cx_logger_start();

		void cx_logger_get(std::vector<std::string>& log);

		void cx_logger_stop();


		/**///remove entity
		void cx_removeEntities(const std::vector<std::pair<int, int> >& dimTags, const bool recursive = false);


	public:
		//获取dim_tag实体对应的node点列表
		void getNodes(std::pair<int, int>& dim_tag, std::map<int, std::vector<double>>& map_nodes);

		//获取dim_tag实体对应的triangle列表
		void getTriFaces(std::pair<int, int>& dim_tag, std::vector<std::vector<int>>& vec_triFaces);

		//获取dim_tag实体对应的Cell列表
		void getTetCells(std::pair<int, int>& dim_tag, std::vector<TetCell>& vec_tetCells);

		//lio::cxMesh* CreateEntityMesh(int dim, nemo::neShapeLeaf* pleaf);

		//set mesh size
		//void SetSyntheticalEntitiesMeshSize(std::vector<nemo::neShapeLeaf*> vec_leafs);


		//.mesh formate write(自定义输出)
		void meshFormate_write(std::string file_path);


		//改进的3DFilter算法(在filter_vv的[iter_begin, iter_end]范围内，相互排除重复的<id,tag>对)
		void FilterIdTags3D_improve(std::vector<std::vector<std::pair<int, int>>>& filter_vv, int iter_begin, int iter_end);
		void RemoveIdTagInMutual(std::vector<std::pair<int, int>>& one_vector, std::vector<std::pair<int, int>>& other_vector);
		//改进的2DFilter算法(在filter_vv的[iter_begin, iter_end]范围内排除remove_vv中的所有<id,tag>对)
		void FilterIdTags2D_improve(std::vector<std::vector<std::pair<int, int>>>& filter_vv, int iter_begin, int iter_end, std::vector<std::vector<std::pair<int, int>>>& remove_vv);
		void RemoveIdTagInVector(std::vector<std::pair<int, int>>& sorce_vector, std::pair<int, int>& remove_idtag);



	private:
		oxGmsh(void);
		~oxGmsh(void);

	public:
		int		m_option_algorithm3D = 1;				//1:Delaunay[默认]    4: Frontal	  10: HXT    7: MMG3D    3: Initial mesh only
		int     m_option_smoothSteps = 1;				//光顺次数
		double  m_option_meshSizeFactor = 1.0;			//网格尺寸因子
		double	m_option_minElementSize = 0.0;			//最小网格尺寸值
		double	m_option_maxElementSize = 1.0e22;		//最大网格尺寸值
		//
		bool    m_option_meshSizeFromPoints = true;				//使用几何点上的meshSize值
		bool	m_option_meshSizeExtendFromBoundary = true;		//从边界扩展网格尺寸
		bool	m_option_optimize = true;						//优化四面体质量
		bool	m_option_optimizeNetgen = false;				//使用Netgen优化四面体质量

		//Append: 后续参数添加
		int		m_option_SubdivisionAlgorithm = 1;		//1:四面体 2:六面体


		std::map<int, std::vector<std::pair<int, int>>> map_id_dimTags_3d;
		std::map<int, std::vector<std::pair<int, int>>> map_id_dimTags_2d;
	};

}

