#pragma once

//#include "qxVector2.h"

namespace qxGeom
{
	class qxPoint
	{
	public:
		qxPoint(void);
		qxPoint(double x, double y, double z);
		qxPoint(const double* val);
		virtual ~qxPoint(void);
		//operator cxVector() const { return cxVector(m_coord[0], m_coord[1], m_coord[2]); }

		virtual rapidjson::Value toJson(rapidjson::Document& doc);
		virtual void fromJson(const rapidjson::Value& root, int version);

	public:
		qxPoint& operator =(const qxPoint& point);
		double operator [](int idx) const;
		double& operator [](int idx);
		qxPoint& operator +=(const qxPoint& point);
//		qxPoint& operator +=(const qxVector& vec);
		qxPoint& operator -=(const qxPoint& point);
//		qxPoint& operator -=(const qxVector& vec);
		qxPoint& operator *= (double f);
		qxPoint& operator /=(double f);

	public:
		const double& x() const { return m_coord[0]; }
		double& x() { return m_coord[0]; }
		const double& y() const { return m_coord[1]; }
		double& y() { return m_coord[1]; }
		const double& z() const { return m_coord[2]; }
		double& z() { return m_coord[2]; }
		double*	Value() { return m_coord; };
		const double*	Value() const { return m_coord; };
	public:
		void Set(double x, double y, double z);
		double DistanceToPoint(const qxPoint& point) const;
		double SquareDistanceToPoint(const qxPoint& point) const;

	public:
	public:
// 		friend qxVector operator -(const qxPoint& p1, const qxPoint& p2)
// 		{
// 			return qxVector(p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]);
// 		}

// 		friend qxPoint operator +(const qxPoint& p, const qxVector& v)
// 		{
// 			return qxPoint(p[0] + v[0], p[1] + v[1], p[2] + v[2]);
// 		}
// 
// 		friend qxPoint operator -(const qxPoint& p, const qxVector& v)
// 		{
// 			return qxPoint(p[0] - v[0], p[1] - v[1], p[2] - v[2]);
// 		}

		friend qxPoint operator *(double r, const qxPoint& point)
		{
			return qxPoint(r*point[0], r*point[1], r*point[2]);
		}

//  		friend qxPoint operator *(const qxGeom::qxMatrix4x4& mat, const qxPoint& point)
//  		{
// 			return qxPoint( mat.GetElement(0, 0) * point[0] + mat.GetElement(0, 1) * point[1] + mat.GetElement(0, 2) * point[2] + mat.GetElement(0, 3),
// 							mat.GetElement(1, 0) * point[0] + mat.GetElement(1, 1) * point[1] + mat.GetElement(1, 2) * point[2] + mat.GetElement(1, 3),
// 							mat.GetElement(2, 0) * point[0] + mat.GetElement(2, 1) * point[1] + mat.GetElement(2, 2) * point[2] + mat.GetElement(2, 3));
//  		}

		friend bool operator ==(const qxPoint& point1, const qxPoint& point2)
		{
			double dist = point1.DistanceToPoint(point2);
			return fabs(dist) < 1.0e-4;
		}

	private:
		double m_coord[3];
	};

	class PointCompare
	{
	public:
		PointCompare(double dEps = 1.0e-7) : m_tol(dEps) {}

		bool operator()(const qxPoint& p0, const qxPoint& p1) const
		{
			for (int i = 0; i < 3; i++)
			{
				if (fabs(p0[i] - p1[i]) > m_tol)
				{
					return false;
				}				
			}
			return true;
		}

	private:
		double m_tol;	// tolerance
	};

}
