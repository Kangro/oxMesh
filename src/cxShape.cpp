#include "stdafx.h"
#include "Include/cxShape.h"
//#include "Include/occTopology.h"
//#include "Include/cxLineSeg.h"
//#include "Include/magObjectEnum.h"
//#include "Include/occBooleanFormula.h"
#include <occ/inc/BRepTools.hxx>
#include <occ/inc/BRep_Builder.hxx>

#include "setDebugNew.h"
//#include "Include/occUtils.h"

namespace oxm
{

	cxShape::cxShape()
		//: qxCom::qxObject(lio::q_Shape), qxCom::qxIdName()
	{
	}

	cxShape::cxShape(TopoDS_Shape shape)
		//: qxCom::qxObject(lio::q_Shape), qxCom::qxIdName()
	{
		_occshape = shape;
	}

	cxShape::~cxShape()
	{
		clear();
	}

	void cxShape::clear()
	{
	}

	void cxShape::Copy(const cxShape& src)
	{
		if (this != &src) 
		{
			//qxIdName::Copy(src);
			_occshape = src.occshape();
		}
	}

	bool cxShape::Compare(const cxShape & src)
	{
		//if(!qxIdName::Compare(src)) return false;
		if (_occshape != src._occshape) return false;
		return true;
	}

	rapidjson::Value cxShape::toJson(rapidjson::Document& doc)
	{
		RAPIDJSON_NAMESPACE::Value root(RAPIDJSON_NAMESPACE::kObjectType);
		//auto root = qxCom::qxIdName::toJson(doc);
		//root.AddMember("type", shapeType(), doc.GetAllocator());
		std::stringstream om;
		BRepTools::Write(_occshape, om);
		root.AddMember("source", om.str(), doc.GetAllocator());
		root.AddMember("name", _name, doc.GetAllocator());
		return root;
	}

	void cxShape::fromJson(const rapidjson::Value& root, int version)
	{
		//qxCom::qxIdName::fromJson(root, version);
		std::string s = root["source"].GetString();
		std::stringstream im(s);
		TopoDS_Shape C;
		BRep_Builder B;
		BRepTools::Read(C, im, B);
		_occshape = C;
		_name = root["name"].GetString();
	}


// 	void cxShape::GetBoundingBoxSize(double& xsize, double& ysize, double& zsize)
// 	{
// 		std::vector<double> vec_size;
// 		qxGeom::qxPoint center;
// 		CalcBndBoxSize(_occshape, center, vec_size);
// 		if (vec_size.size() != 3)
// 		{
// 			return;
// 		}
// 		xsize = vec_size[0];
// 		ysize = vec_size[1];
// 		zsize = vec_size[2];
// 	}

// 	void cxShape::GetProjectedSize(const qxGeom::qxVector& project_vec, double& zsize)
// 	{
// 		qxGeom::qxVector vec_temp = project_vec;
// 		CalcProjectSize(_occshape, vec_temp, zsize);
// 	}


// 	void cxShape::CalcCentroid(qxGeom::qxPoint& center_pnt)
// 	{
// 		CalcShapeCenter(_occshape, center_pnt);
// 	}

// 	void cxShape::CreateBndBoxShape(const qxGeom::qxPoint& minCorner, const qxGeom::qxPoint& maxCorner)
// 	{
// 		MakeBoundboxShape(minCorner, maxCorner, _occshape);
// 	}
// 
// 	double cxShape::CalcVolume()
// 	{
// 		return lio::CalcVolume(_occshape);
// 	}

	bool cxShape::IsNull()
	{
		return _occshape.IsNull();
	}

}