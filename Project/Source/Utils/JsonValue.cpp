#include "JsonValue.h"

#include "Utils/Leaks.h"

JsonValue::JsonValue(rapidjson::Document& document_, rapidjson::Value& value_)
	: document(document_)
	, value(value_) {}

size_t JsonValue::Size() const {
	return value.IsArray() ? value.Size() : 0;
}

JsonValue JsonValue::operator[](unsigned index) const {
	if (!value.IsArray()) {
		value.SetArray();
	}

	while (index >= value.Size()) {
		value.PushBack(rapidjson::Value(), document.GetAllocator());
	}

	return JsonValue(document, value[index]);
}

void JsonValue::operator=(bool x) {
	value = x;
}

void JsonValue::operator=(int x) {
	value = x;
}

void JsonValue::operator=(unsigned x) {
	value = x;
}

void JsonValue::operator=(long long x) {
	value = x;
}

void JsonValue::operator=(unsigned long long x) {
	value = x;
}

void JsonValue::operator=(float x) {
	value = x;
}

void JsonValue::operator=(double x) {
	value = x;
}

void JsonValue::operator=(const char* x) {
	value.SetString(x, document.GetAllocator());
}

JsonValue::operator bool() const {
	return value.IsBool() ? value.GetBool() : false;
}

JsonValue::operator int() const {
	return value.IsInt() ? value.GetInt() : 0;
}

JsonValue::operator unsigned() const {
	return value.IsUint() ? value.GetUint() : 0;
}

JsonValue::operator long long() const {
	return value.IsInt64() ? value.GetInt64() : 0;
}

JsonValue::operator unsigned long long() const {
	return value.IsUint64() ? value.GetUint64() : 0;
}

JsonValue::operator float() const {
	return value.IsDouble() ? value.GetFloat() : 0;
}

JsonValue::operator double() const {
	return value.IsDouble() ? value.GetDouble() : 0;
}

JsonValue::operator std::string() const {
	return value.IsString() ? value.GetString() : "";
}
