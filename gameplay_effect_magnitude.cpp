#include "gameplay_effect_magnitude.h"
#include "gameplay_ability.h"
#include "gameplay_ability_system.h"
#include "gameplay_attribute.h"
#include "gameplay_effect.h"
#include "gameplay_tags.h"

#include <array>
#include <iostream>

double GameplayEffectMagnitude::calculate_magnitude(const Node *, const Node *, const Ref<GameplayEffect> &, int64_t, double) {
	return 0;
}

void GameplayEffectMagnitude::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("calculate_magnitude", "source", "target", "effect", "level"), &GameplayEffectMagnitude::calculate_magnitude);
}

double ScalableFloat::calculate_magnitude(const Node *, const Node *, const Ref<GameplayEffect> &, int64_t, double level) {
	return curve.is_valid() ? value * curve->interpolate(level) : value;
}

void ScalableFloat::set_value(double value) {
	this->value = value;
}

double ScalableFloat::get_value() const {
	return value;
}

void ScalableFloat::set_curve(const Ref<Curve> &value) {
	curve = value;
}

Ref<Curve> ScalableFloat::get_curve() const {
	return curve;
}

void ScalableFloat::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_value", "value"), &ScalableFloat::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &ScalableFloat::get_value);
	ClassDB::bind_method(D_METHOD("set_curve", "value"), &ScalableFloat::set_curve);
	ClassDB::bind_method(D_METHOD("get_curve"), &ScalableFloat::get_curve);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "value"), "set_value", "get_value");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_curve", "get_curve");
}

double AttributeBasedFloat::calculate_magnitude(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) {
	const GameplayAbilitySystem *origin = nullptr;

	switch (attribute_origin) {
		case AttributeOrigin::Source: {
			origin = dynamic_cast<const GameplayAbilitySystem *>(source);
		} break;
		case AttributeOrigin::Target: {
			origin = dynamic_cast<const GameplayAbilitySystem *>(target);
		} break;
		default: {
			return 0.0;
		} break;
	}

	if (!origin) {
		return 0.0;
	}

	double attribute_value = 0;

	switch (attribute_calculation) {
		case AttributeCalculation::CurrentValue: {
			attribute_value = origin->get_current_attribute_value(backing_attribute);
		} break;
		case AttributeCalculation::BaseValue: {
			attribute_value = origin->get_base_attribute_value(backing_attribute);
		} break;
		case AttributeCalculation::DeltaValue: {
			auto attribute_data = origin->get_attribute_data(backing_attribute);
			attribute_value = attribute_data->get_current_value() - attribute_data->get_base_value();
		} break;
		default: {
			return 0.0;
		} break;
	}

	auto coefficient_magnitude = coefficient.is_valid() ? coefficient->calculate_magnitude(source, target, effect, level, normalised_level) : 1.0;
	auto pre_addition_magnitude = pre_multiply_addition.is_valid() ? pre_multiply_addition->calculate_magnitude(source, target, effect, level, normalised_level) : 0.0;
	auto post_addition_magnitude = post_multiply_addition.is_valid() ? post_multiply_addition->calculate_magnitude(source, target, effect, level, normalised_level) : 0.0;
	auto curve_value = attribute_curve.is_valid() ? attribute_curve->interpolate(level) : 1.0;

	return coefficient_magnitude * (pre_addition_magnitude + attribute_value * curve_value) + post_addition_magnitude;
}

void AttributeBasedFloat::set_coefficient(const Ref<ScalableFloat> &value) {
	coefficient = value;
}

Ref<ScalableFloat> AttributeBasedFloat::get_coefficient() const {
	return coefficient;
}

void AttributeBasedFloat::set_pre_multiply_addition(const Ref<ScalableFloat> &value) {
	pre_multiply_addition = value;
}

Ref<ScalableFloat> AttributeBasedFloat::get_pre_multiply_addition() const {
	return pre_multiply_addition;
}

void AttributeBasedFloat::set_post_multiply_addition(const Ref<ScalableFloat> &value) {
	post_multiply_addition = value;
}

Ref<ScalableFloat> AttributeBasedFloat::get_post_multiply_addition() const {
	return post_multiply_addition;
}

void AttributeBasedFloat::set_backing_attribute(const StringName &value) {
	backing_attribute = value;
}

StringName AttributeBasedFloat::get_backing_attribute() const {
	return backing_attribute;
}

void AttributeBasedFloat::set_attribute_curve(const Ref<Curve> &value) {
	attribute_curve = value;
}

Ref<Curve> AttributeBasedFloat::get_attribute_curve() const {
	return attribute_curve;
}

void AttributeBasedFloat::set_attribute_origin(AttributeOrigin::Type value) {
	attribute_origin = value;
}

AttributeOrigin::Type AttributeBasedFloat::get_attribute_origin() const {
	return attribute_origin;
}

void AttributeBasedFloat::set_attribute_calculation(AttributeCalculation::Type value) {
	attribute_calculation = value;
}

AttributeCalculation::Type AttributeBasedFloat::get_attribute_calculation() const {
	return attribute_calculation;
}

void AttributeBasedFloat::set_source_tag_filter(const Ref<GameplayTagContainer> &) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> AttributeBasedFloat::get_source_tag_filter() const {
	return source_tag_filter;
}

void AttributeBasedFloat::set_target_tag_filter(const Ref<GameplayTagContainer> &) {
	ERR_EXPLAIN("GameplayTagContainer are readonly properties.");
	ERR_FAIL();
}

Ref<GameplayTagContainer> AttributeBasedFloat::get_target_tag_filter() const {
	return target_tag_filter;
}

void AttributeBasedFloat::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_coefficient", "value"), &AttributeBasedFloat::set_coefficient);
	ClassDB::bind_method(D_METHOD("get_coefficient"), &AttributeBasedFloat::get_coefficient);
	ClassDB::bind_method(D_METHOD("set_pre_multiply_addition", "value"), &AttributeBasedFloat::set_pre_multiply_addition);
	ClassDB::bind_method(D_METHOD("get_pre_multiply_addition"), &AttributeBasedFloat::get_pre_multiply_addition);
	ClassDB::bind_method(D_METHOD("set_post_multiply_addition", "value"), &AttributeBasedFloat::set_post_multiply_addition);
	ClassDB::bind_method(D_METHOD("get_post_multiply_addition"), &AttributeBasedFloat::get_post_multiply_addition);
	ClassDB::bind_method(D_METHOD("set_backing_attribute", "value"), &AttributeBasedFloat::set_backing_attribute);
	ClassDB::bind_method(D_METHOD("get_backing_attribute"), &AttributeBasedFloat::get_backing_attribute);
	ClassDB::bind_method(D_METHOD("set_attribute_curve", "value"), &AttributeBasedFloat::set_attribute_curve);
	ClassDB::bind_method(D_METHOD("get_attribute_curve"), &AttributeBasedFloat::get_attribute_curve);
	ClassDB::bind_method(D_METHOD("set_attribute_origin", "value"), &AttributeBasedFloat::set_attribute_origin);
	ClassDB::bind_method(D_METHOD("get_attribute_origin"), &AttributeBasedFloat::get_attribute_origin);
	ClassDB::bind_method(D_METHOD("set_attribute_calculation", "value"), &AttributeBasedFloat::set_attribute_calculation);
	ClassDB::bind_method(D_METHOD("get_attribute_calculation"), &AttributeBasedFloat::get_attribute_calculation);
	ClassDB::bind_method(D_METHOD("set_source_tag_filter", "value"), &AttributeBasedFloat::set_source_tag_filter);
	ClassDB::bind_method(D_METHOD("get_source_tag_filter"), &AttributeBasedFloat::get_source_tag_filter);
	ClassDB::bind_method(D_METHOD("set_target_tag_filter", "value"), &AttributeBasedFloat::set_target_tag_filter);
	ClassDB::bind_method(D_METHOD("get_target_tag_filter"), &AttributeBasedFloat::get_target_tag_filter);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "coefficient", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_coefficient", "get_coefficient");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "pre_multiply_addition", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_pre_multiply_addition", "get_pre_multiply_addition");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "post_multiply_addition", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_post_multiply_addition", "get_post_multiply_addition");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "backing_attribute"), "set_backing_attribute", "get_backing_attribute");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attribute_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_attribute_curve", "get_attribute_curve");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "attribute_origin", PROPERTY_HINT_ENUM, "Source,Target"), "set_attribute_origin", "get_attribute_origin");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "attribute_calculation", PROPERTY_HINT_ENUM, "Current Value,Base Value,Delta Value"), "set_attribute_calculation", "get_attribute_calculation");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "source_tag_filter", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_source_tag_filter", "get_source_tag_filter");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "target_tag_filter", PROPERTY_HINT_RESOURCE_TYPE, "GameplayTagContainer"), "set_target_tag_filter", "get_target_tag_filter");
}

double CustomCalculatedFloat::calculate_magnitude(const Node *source, const Node *target, const Ref<GameplayEffect> &effect, int64_t level, double normalised_level) {
	if (script.is_null()) {
		script = GameplayPtr<ScriptInstance>(custom_calculation_script->instance_create(this));
	}
	if (script.is_valid()) {
		auto coefficient_magnitude = coefficient.is_valid() ? coefficient->calculate_magnitude(source, target, effect, level, normalised_level) : 1.0;
		auto pre_addition_magnitude = pre_multiply_addition.is_valid() ? pre_multiply_addition->calculate_magnitude(source, target, effect, level, normalised_level) : 0.0;
		auto post_addition_magnitude = post_multiply_addition.is_valid() ? post_multiply_addition->calculate_magnitude(source, target, effect, level, normalised_level) : 0.0;
		auto custom_magnitude = static_cast<double>(script->call("_execute", source, target, effect, level, normalised_level));

		return coefficient_magnitude * (pre_addition_magnitude + custom_magnitude) + post_addition_magnitude;
	} else {
		WARN_PRINTS("Could not instantiate custom magnitude calculation script: " + custom_calculation_script->get_path());
	}

	return 0.0;
}

void CustomCalculatedFloat::set_coefficient(const Ref<ScalableFloat> &value) {
	coefficient = value;
}

Ref<ScalableFloat> CustomCalculatedFloat::get_coefficient() const {
	return coefficient;
}

void CustomCalculatedFloat::set_pre_multiply_addition(const Ref<ScalableFloat> &value) {
	pre_multiply_addition = value;
}

Ref<ScalableFloat> CustomCalculatedFloat::get_pre_multiply_addition() const {
	return pre_multiply_addition;
}

void CustomCalculatedFloat::set_post_multiply_addition(const Ref<ScalableFloat> &value) {
	post_multiply_addition = value;
}

Ref<ScalableFloat> CustomCalculatedFloat::get_post_multiply_addition() const {
	return post_multiply_addition;
}

void CustomCalculatedFloat::set_calculation_script(const Ref<Script> &value) {
	custom_calculation_script = value;
}

Ref<Script> CustomCalculatedFloat::get_calculation_script() const {
	return custom_calculation_script;
}

void CustomMagnitudeCalculator::_bind_methods() {
	BIND_VMETHOD(MethodInfo(Variant::REAL, "_execute", PropertyInfo(Variant::OBJECT, "source"), PropertyInfo(Variant::OBJECT, "target"), PropertyInfo(Variant::OBJECT, "effect"), PropertyInfo(Variant::INT, "level"), PropertyInfo(Variant::REAL, "normalised_level")));
}

void CustomCalculatedFloat::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("set_coefficient", "value"), &CustomCalculatedFloat::set_coefficient);
	ClassDB::bind_method(D_METHOD("get_coefficient"), &CustomCalculatedFloat::get_coefficient);
	ClassDB::bind_method(D_METHOD("set_pre_multiply_addition", "value"), &CustomCalculatedFloat::set_pre_multiply_addition);
	ClassDB::bind_method(D_METHOD("get_pre_multiply_addition"), &CustomCalculatedFloat::get_pre_multiply_addition);
	ClassDB::bind_method(D_METHOD("set_post_multiply_addition", "value"), &CustomCalculatedFloat::set_post_multiply_addition);
	ClassDB::bind_method(D_METHOD("get_post_multiply_addition"), &CustomCalculatedFloat::get_post_multiply_addition);
	ClassDB::bind_method(D_METHOD("set_calculation_script", "value"), &CustomCalculatedFloat::set_calculation_script);
	ClassDB::bind_method(D_METHOD("get_calculation_script"), &CustomCalculatedFloat::get_calculation_script);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "coefficient", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_coefficient", "get_coefficient");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "pre_multiply_addition", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_pre_multiply_addition", "get_pre_multiply_addition");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "post_multiply_addition", PROPERTY_HINT_RESOURCE_TYPE, "ScalableFloat"), "set_post_multiply_addition", "get_post_multiply_addition");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "custom_calculation_script", PROPERTY_HINT_RESOURCE_TYPE, "Script"), "set_calculation_script", "get_calculation_script");
}
