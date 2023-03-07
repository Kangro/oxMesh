#pragma once


#include "TopoDS_Shape.hxx"


class TopoDS_Shape;
namespace oxm
{

	//OCC类“TopoDS_Shape”的包装类
	class cxShape //: public qxCom::qxObject, public qxCom::qxIdName
	{
		//qxCopyMacro(cxShape, qxCom::qxObject)
	public:
		cxShape();
		cxShape(TopoDS_Shape shape);
		virtual ~cxShape();
		virtual void Copy(const cxShape& src);
		virtual bool Compare(const cxShape& src);
		virtual rapidjson::Value toJson(rapidjson::Document& doc);
		virtual void fromJson(const rapidjson::Value& root, int version);
		virtual void clear();
		virtual const std::string typeName() const { return "shape"; }

	public:		
// 		//获取包围盒X/Y/Z轴尺寸值
// 		void GetBoundingBoxSize(double& xsize, double& ysize, double& zsize);
// 
// 		//获取模型在投影方向上的尺寸值
// 		void GetProjectedSize(const qxGeom::qxVector& project_vec, double& zsize);
// 
// 		//计算形心坐标
// 		void CalcCentroid(qxGeom::qxPoint& center_pnt);
// 
// 		//创建包围盒体
// 		void CreateBndBoxShape(const qxGeom::qxPoint& minCorner, const qxGeom::qxPoint& maxCorner);

		//计算体积
		double CalcVolume();

		//判断是否为空
		bool IsNull();

	public:
		const TopoDS_Shape&		occshape() const { return _occshape; }
		TopoDS_Shape&			occshape() { return _occshape; }

		int						shapeType() { return (int)_occshape.ShapeType(); }
		std::string&			name() { return _name; }

	private:
		TopoDS_Shape				_occshape;
		std::string					_name;
	};

}
