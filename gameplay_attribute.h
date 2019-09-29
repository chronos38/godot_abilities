#pragma once

#include "gameplay_node.h"

#include <core/array.h>
#include <core/hash_map.h>
#include <core/io/resource_loader.h>
#include <core/pool_vector.h>
#include <core/resource.h>
#include <core/variant.h>
#include <core/vector.h>

class GameplayEffect;
class GameplayAbilitySystem;

class GAMEPLAY_ABILITIES_API GameplayAttributeData : public GameplayResource {
	GDCLASS(GameplayAttributeData, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayAttributeData() = default;

	void reset_to_base();

	void set_base_value(double value);
	double get_base_value() const;
	void set_current_value(double value);
	double get_current_value() const;

private:
	double base_value = 0;
	double current_value = 0;

	static void _bind_methods();
};

class GAMEPLAY_ABILITIES_API GameplayAttribute : public GameplayResource {
	GDCLASS(GameplayAttribute, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayAttribute() = default;

	void set_attribute_name(const StringName &value);
	StringName get_attribute_name() const;
	void set_attribute_data(const Ref<GameplayAttributeData> &value);
	Ref<GameplayAttributeData> get_attribute_data() const;

private:
	StringName attribute_name;
	Ref<GameplayAttributeData> attribute_data;

	static void _bind_methods();
};

class GAMEPLAY_ABILITIES_API GameplayAttributeSet : public GameplayResource {
	GDCLASS(GameplayAttributeSet, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

public:
	virtual ~GameplayAttributeSet() = default;

	bool has_attribute(const StringName &name) const;
	void add_attribute(const StringName &name, double base_value);
	void update_attribute(const StringName &name, double base_value, bool reset_current_value = true);
	void remove_attribute(const StringName &name);

	Array get_attributes() const;
	Ref<GameplayAttribute> get_attribute(const StringName &name) const;
	Ref<GameplayAttributeData> get_attribute_data(const StringName &name) const;

	void set_attribute_set_name(const StringName &value);
	StringName get_attribute_set_name() const;

private:
	StringName attribute_set_name;
	Dictionary attributes;

	static void _bind_methods();
};
