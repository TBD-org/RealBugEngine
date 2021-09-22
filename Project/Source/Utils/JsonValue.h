#pragma once

#include "rapidjson/document.h"

#include <string>

class JsonValue {
public:
	JsonValue(rapidjson::Document& document, rapidjson::Value& value);

	// Size of the array. Returns 0 if the value is not an array.
	size_t Size() const;

	// Object/array access (Creates new members if they don't exist)
	template<typename T> JsonValue operator[](T* key) const;
	JsonValue operator[](unsigned index) const;

	// Assignment operators to modify the value
	void operator=(bool x);
	void operator=(int x);
	void operator=(unsigned x);
	void operator=(long long x);
	void operator=(unsigned long long x);
	void operator=(float x);
	void operator=(double x);
	void operator=(const char* x);

	// Conversion operators for easy access
	operator bool() const;
	operator int() const;
	operator unsigned() const;
	operator long long() const;
	operator unsigned long long() const;
	operator float() const;
	operator double() const;
	operator std::string() const;

private:
	rapidjson::Document& document;
	rapidjson::Value& value;
};

// Template is necessary to disambiguate with 'operator[](int index)'
template<typename T>
inline JsonValue JsonValue::operator[](T* key) const {
	if (!value.IsObject()) {
		value.SetObject();
	}

	if (!value.HasMember(key)) {
		value.AddMember(rapidjson::StringRef(key), rapidjson::Value(), document.GetAllocator());
	}

	return JsonValue(document, value[key]);
}