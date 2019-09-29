#include "gameplay_attribute.h"

void GameplayAttributeData::reset_to_base() {
	current_value = base_value;
}

void GameplayAttributeData::set_base_value(double value) {
	base_value = value;
}

double GameplayAttributeData::get_base_value() const {
	return base_value;
}

void GameplayAttributeData::set_current_value(double value) {
	current_value = value;
}

double GameplayAttributeData::get_current_value() const {
	return current_value;
}

void GameplayAttributeData::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("reset_to_base"), &GameplayAttributeData::reset_to_base);
	ClassDB::bind_method(D_METHOD("set_base_value", "value"), &GameplayAttributeData::set_base_value);
	ClassDB::bind_method(D_METHOD("get_base_value"), &GameplayAttributeData::get_base_value);
	ClassDB::bind_method(D_METHOD("set_current_value", "value"), &GameplayAttributeData::set_current_value);
	ClassDB::bind_method(D_METHOD("get_current_value"), &GameplayAttributeData::get_current_value);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "base_value"), "set_base_value", "get_base_value");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "current_value"), "set_current_value", "get_current_value");
}

void GameplayAttribute::set_attribute_name(const StringName &value) {
	attribute_name = value;
}

StringName GameplayAttribute::get_attribute_name() const {
	return attribute_name;
}

void GameplayAttribute::set_attribute_data(const Ref<GameplayAttributeData> &value) {
	attribute_data = value;
}

Ref<GameplayAttributeData> GameplayAttribute::get_attribute_data() const {
	return attribute_data;
}

void GameplayAttribute::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_attribute_name", "value"), &GameplayAttribute::set_attribute_name);
	ClassDB::bind_method(D_METHOD("get_attribute_name"), &GameplayAttribute::get_attribute_name);
	ClassDB::bind_method(D_METHOD("set_attribute_data", "value"), &GameplayAttribute::set_attribute_data);
	ClassDB::bind_method(D_METHOD("get_attribute_data"), &GameplayAttribute::get_attribute_data);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "attribute_name"), "set_attribute_name", "get_attribute_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attribute_data", PROPERTY_HINT_RESOURCE_TYPE, "GameplayAttributeData"), "set_attribute_data", "get_attribute_data");
}

bool GameplayAttributeSet::has_attribute(const StringName &name) const {
	return attributes.has(name);
}

void GameplayAttributeSet::add_attribute(const StringName &name, double base_value) {
	ERR_FAIL_COND(has_attribute(name));
	auto attribute = make_reference<GameplayAttribute>();
	auto attribute_data = make_reference<GameplayAttributeData>();
	attribute_data->set_base_value(base_value);
	attribute_data->set_current_value(base_value);
	attribute->set_attribute_name(name);
	attribute->set_attribute_data(attribute_data);
	attributes[name] = attribute;
}

void GameplayAttributeSet::update_attribute(const StringName &name, double base_value, bool reset_current_value /*= true*/) {
	if (attributes.has(name)) {
		auto attribute = get_attribute(name);
		attribute->get_attribute_data()->set_base_value(base_value);

		if (reset_current_value) {
			attribute->get_attribute_data()->reset_to_base();
		}
	}
}

void GameplayAttributeSet::remove_attribute(const StringName &name) {
	attributes.erase(name);
}

Array GameplayAttributeSet::get_attributes() const {
	return attributes.values();
}

Ref<GameplayAttribute> GameplayAttributeSet::get_attribute(const StringName &name) const {
	return attributes[name];
}

Ref<GameplayAttributeData> GameplayAttributeSet::get_attribute_data(const StringName &name) const {
	return get_attribute(name)->get_attribute_data();
}

void GameplayAttributeSet::set_attribute_set_name(const StringName &value) {
	attribute_set_name = value;
}

StringName GameplayAttributeSet::get_attribute_set_name() const {
	return attribute_set_name;
}

void GameplayAttributeSet::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("has_attribute", "name"), &GameplayAttributeSet::has_attribute);
	ClassDB::bind_method(D_METHOD("add_attribute", "name", "base_value"), &GameplayAttributeSet::add_attribute);
	ClassDB::bind_method(D_METHOD("update_attribute", "name", "base_value", "reset_to_default"), &GameplayAttributeSet::update_attribute);
	ClassDB::bind_method(D_METHOD("remove_attribute", "name"), &GameplayAttributeSet::remove_attribute);
	ClassDB::bind_method(D_METHOD("get_attributes"), &GameplayAttributeSet::get_attributes);
	ClassDB::bind_method(D_METHOD("get_attribute", "name"), &GameplayAttributeSet::get_attribute);
	ClassDB::bind_method(D_METHOD("set_attribute_set_name", "value"), &GameplayAttributeSet::set_attribute_set_name);
	ClassDB::bind_method(D_METHOD("get_attribute_set_name"), &GameplayAttributeSet::get_attribute_set_name);

	BIND_VMETHOD(MethodInfo("_pre_effect_execution", PropertyInfo(Variant::OBJECT, "effect"), PropertyInfo(Variant::OBJECT, "attribute"), PropertyInfo(Variant::REAL, "magnitude")));
	BIND_VMETHOD(MethodInfo("_post_effect_execution", PropertyInfo(Variant::OBJECT, "effect"), PropertyInfo(Variant::OBJECT, "attribute"), PropertyInfo(Variant::REAL, "magnitude")));
	BIND_VMETHOD(MethodInfo("_pre_attribute_base_change", PropertyInfo(Variant::OBJECT, "attribute"), PropertyInfo(Variant::REAL, "magnitude")));
	BIND_VMETHOD(MethodInfo("_pre_attribute_change", PropertyInfo(Variant::OBJECT, "attribute"), PropertyInfo(Variant::REAL, "magnitude")));

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "attribute_set_name"), "set_attribute_set_name", "get_attribute_set_name");
}
