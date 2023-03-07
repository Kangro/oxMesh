#pragma once

namespace json {
	static void fromArray_rapid_int(const rapidjson::Value &root, int* v)
	{
		const auto& ary = root.GetArray();
		int n = ary.Size();
		for (int i = 0; i < n; i++)
		{
			v[i] = ary[i].GetInt();
		}
	}
	template<class T>
	rapidjson::Value toArray_rapid(const T * value, int n, rapidjson::Document &doc)
	{
		rapidjson::Value array1(rapidjson::kArrayType);
		for (int i = 0; i < n; i++)
		{
			// 			rapidjson::Value int_object(rapidjson::kObjectType);
			// 			int_object.SetInt(i);
			array1.PushBack(rapidjson::Value(value[i]).Move(), doc.GetAllocator());
		}
		return array1;
	}
	static void fromArray_rapid_double(const rapidjson::Value &root, double* v)
	{
		const auto& ary = root.GetArray();
		int n = ary.Size();
		for (int i = 0; i < n; i++)
		{
			v[i] = ary[i].GetDouble();
		}
	}
}

