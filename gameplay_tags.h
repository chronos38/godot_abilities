#pragma once

#include "gameplay_node.h"

#include <core/resource.h>
#include <core/variant.h>

class GAMEPLAY_ABILITIES_API GameplayTagContainer : public GameplayResource {
	GDCLASS(GameplayTagContainer, GameplayResource);
	OBJ_CATEGORY("GameplayAbilities");

	friend String *begin(Ref<GameplayTagContainer> &tags);
	friend const String *begin(const Ref<GameplayTagContainer> &tags);
	friend const String *cbegin(const Ref<GameplayTagContainer> &tags);
	friend String *end(Ref<GameplayTagContainer> &tags);
	friend const String *end(const Ref<GameplayTagContainer> &tags);
	friend const String *cend(const Ref<GameplayTagContainer> &tags);

public:
	bool has_tag(const String &tag) const;
	bool has_all(const Ref<GameplayTagContainer> &tags) const;
	bool has_any(const Ref<GameplayTagContainer> &tags) const;
	bool has_none(const Ref<GameplayTagContainer> &tags) const;

	void set_tag(int index, const String &value);
	const String &get_tag(int index) const;
	int size() const;
	bool empty() const;

	void append(const String &tag);
	void append_tags(const Ref<GameplayTagContainer> &tags);
	void append_array(const PoolStringArray &array);
	void remove(const String &tag);
	void remove_tags(const Ref<GameplayTagContainer> &tags);
	void remove_array(const PoolStringArray &array);

	void set_tags(const PoolStringArray &value);
	const PoolStringArray &get_tags() const;

	String get_tag_list();

	String &operator[](int index);
	const String &operator[](int index) const;

private:
	PoolStringArray tags;

	static void _bind_methods();
};

String *begin(Ref<GameplayTagContainer> &tags);
const String *begin(const Ref<GameplayTagContainer> &tags);
const String *cbegin(const Ref<GameplayTagContainer> &tags);
String *end(Ref<GameplayTagContainer> &tags);
const String *end(const Ref<GameplayTagContainer> &tags);
const String *cend(const Ref<GameplayTagContainer> &tags);
