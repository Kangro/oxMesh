#include "stdafx.h"
#include "Include/qxPoint.h"

#include "setDebugNew.h"
namespace qxGeom
{
	qxPoint::qxPoint(void)
	{
		Set(0.0, 0.0, 0.0);
	}

	qxPoint::qxPoint(double x, double y, double z)
	{
		Set(x, y, z);
	}

	qxPoint::qxPoint(const double* val)
	{
		Set(val[0], val[1], val[2]);
	}

	qxPoint::~qxPoint(void)
	{
	}

	rapidjson::Value qxPoint::toJson(rapidjson::Document& doc)
	{
		RAPIDJSON_NAMESPACE::Value root(RAPIDJSON_NAMESPACE::kObjectType);
		root.AddMember("coord", json::toArray_rapid(Value(), 3, doc), doc.GetAllocator());

		return root;
	}

	void qxPoint::fromJson(const rapidjson::Value& root, int version)
	{
		json::fromArray_rapid_double(root["coord"], m_coord);
	}

	qxPoint& qxPoint::operator =(const qxPoint& point)
	{
		if (this != &point)
		{
			for (int i = 0; i < 3; i++)
			{
				m_coord[i] = point[i];
			}
		}

		return *this;
	}

	double qxPoint::operator [](int idx) const
	{
		assert(idx >= 0 && idx <= 2);
		return m_coord[idx];
	}

	double& qxPoint::operator [](int idx)
	{
		assert(idx >= 0 && idx <= 2);
		return m_coord[idx];
	}

	qxPoint& qxPoint::operator +=(const qxPoint& point)
	{
		Set(m_coord[0] + point.m_coord[0],
			m_coord[1] + point.m_coord[1],
			m_coord[2] + point.m_coord[2]);

		return *this;
	}

// 	qxPoint& qxPoint::operator +=(const qxVector& vec)
// 	{
// 		Set(m_coord[0] + vec.x(),
// 			m_coord[1] + vec.y(),
// 			m_coord[2] + vec.z());
// 
// 		return *this;
// 	}

	qxPoint& qxPoint::operator -=(const qxPoint& point)
	{
		Set(m_coord[0] - point.m_coord[0],
			m_coord[1] - point.m_coord[1],
			m_coord[2] - point.m_coord[2]);

		return *this;
	}

// 	qxPoint& qxPoint::operator -=(const qxVector& vec)
// 	{
// 		Set(m_coord[0] - vec.x(),
// 			m_coord[1] - vec.y(),
// 			m_coord[2] - vec.z());
// 
// 		return *this;
// 	}

	qxPoint& qxPoint::operator /=(double f)
	{
		Set(m_coord[0] / f, m_coord[1] / f, m_coord[2] / f);
		return *this;
	}

	qxPoint& qxPoint::operator *=(double f)
	{
		Set(m_coord[0] * f, m_coord[1] * f, m_coord[2] * f);
		return *this;
	}
	
	void qxPoint::Set(double x, double y, double z)
	{
		m_coord[0] = x;
		m_coord[1] = y;
		m_coord[2] = z;
	}

	double qxPoint::DistanceToPoint(const qxPoint& point) const
	{
		double dist2 = (m_coord[0] - point[0]) * (m_coord[0] - point[0])
			+ (m_coord[1] - point[1]) * (m_coord[1] - point[1])
			+ (m_coord[2] - point[2]) * (m_coord[2] - point[2]);

		return sqrt(dist2);
	}

	double qxPoint::SquareDistanceToPoint(const qxPoint& point) const
	{
		double temp = DistanceToPoint(point);

		return temp * temp;
	}

}