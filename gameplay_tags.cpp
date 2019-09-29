#include "gameplay_tags.h"

#include <algorithm>

bool GameplayTagContainer::has_tag(const String &tag) const {
	if (tag.empty()) {
		return true;
	}

	for (auto &&owned_tag : tags) {
		if (owned_tag.matchn(tag)) {
			return true;
		}
	}

	return false;
}

bool GameplayTagContainer::has_all(const Ref<GameplayTagContainer> &tags) const {
	return std::all_of(begin(tags), end(tags), [this](const String &tag) {
		return has_tag(tag);
	});
}

bool GameplayTagContainer::has_any(const Ref<GameplayTagContainer> &tags) const {
	return std::any_of(begin(tags), end(tags), [this](const String &tag) {
		return has_tag(tag);
	});
}

bool GameplayTagContainer::has_none(const Ref<GameplayTagContainer> &tags) const {
	return std::none_of(begin(tags), end(tags), [this](const String &tag) {
		return has_tag(tag);
	});
}

void GameplayTagContainer::set_tag(int index, const String &value) {
	tags.set(index, value);
}

const String &GameplayTagContainer::get_tag(int index) const {
	return tags.read()[index];
}

int GameplayTagContainer::size() const {
	return tags.size();
}

bool GameplayTagContainer::empty() const {
	return tags.size() <= 0;
}

void GameplayTagContainer::append(const String &tag) {
	if (!has_tag(tag)) {
		tags.push_back(tag);
	}
}

void GameplayTagContainer::append_tags(const Ref<GameplayTagContainer> &tags) {
	this->tags.append_array(tags->tags);
}

void GameplayTagContainer::append_array(const PoolStringArray &array) {
	tags.append_array(array);
}

void GameplayTagContainer::remove(const String &tag) {
	for (int i = tags.size() - 1; i >= 0; i--) {
		if (tags[i].matchn(tag)) {
			tags.remove(i);
		}
	}
}

void GameplayTagContainer::remove_tags(const Ref<GameplayTagContainer> &tags) {
	for (auto &&tag : tags) {
		remove(tag);
	}
}

void GameplayTagContainer::remove_array(const PoolStringArray &array) {
	for (int i = 0, n = array.size(); i < n; i++) {
		remove(array[i]);
	}
}

void GameplayTagContainer::set_tags(const PoolStringArray &value) {
	tags = value;
}

const PoolStringArray &GameplayTagContainer::get_tags() const {
	return tags;
}

String GameplayTagContainer::get_tag_list() {
	return tags.join(",");
}

String &GameplayTagContainer::operator[](int index) {
	return tags.write()[index];
}

const String &GameplayTagContainer::operator[](int index) const {
	return tags.read()[index];
}

void GameplayTagContainer::_bind_methods() {
	/** Methods */
	ClassDB::bind_method(D_METHOD("has_tag", "tag"), &GameplayTagContainer::has_tag);
	ClassDB::bind_method(D_METHOD("has_all", "tags"), &GameplayTagContainer::has_all);
	ClassDB::bind_method(D_METHOD("has_any", "tags"), &GameplayTagContainer::has_any);
	ClassDB::bind_method(D_METHOD("has_none", "tags"), &GameplayTagContainer::has_none);
	ClassDB::bind_method(D_METHOD("set_tag", "index", "tag"), &GameplayTagContainer::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag", "index"), &GameplayTagContainer::get_tag);
	ClassDB::bind_method(D_METHOD("size"), &GameplayTagContainer::size);
	ClassDB::bind_method(D_METHOD("empty"), &GameplayTagContainer::empty);
	ClassDB::bind_method(D_METHOD("append", "tag"), &GameplayTagContainer::append);
	ClassDB::bind_method(D_METHOD("append_tags", "tag"), &GameplayTagContainer::append_tags);
	ClassDB::bind_method(D_METHOD("append_array", "tag"), &GameplayTagContainer::append_array);
	ClassDB::bind_method(D_METHOD("remove", "tag"), &GameplayTagContainer::remove);
	ClassDB::bind_method(D_METHOD("remove_tags", "tag"), &GameplayTagContainer::remove_tags);
	ClassDB::bind_method(D_METHOD("remove_array", "tag"), &GameplayTagContainer::remove_array);
	ClassDB::bind_method(D_METHOD("set_tags", "value"), &GameplayTagContainer::set_tags);
	ClassDB::bind_method(D_METHOD("get_tags"), &GameplayTagContainer::get_tags);
	ClassDB::bind_method(D_METHOD("get_tag_list"), &GameplayTagContainer::get_tag_list);

	/** Properties */
	ADD_PROPERTY(PropertyInfo(Variant::POOL_STRING_ARRAY, "tags"), "set_tags", "get_tags");
}

String *begin(Ref<GameplayTagContainer> &tags) {
	return begin(tags->tags);
}

const String *begin(const Ref<GameplayTagContainer> &tags) {
	return begin(tags->tags);
}

const String *cbegin(const Ref<GameplayTagContainer> &tags) {
	return cbegin(tags->tags);
}

String *end(Ref<GameplayTagContainer> &tags) {
	return end(tags->tags);
}

const String *end(const Ref<GameplayTagContainer> &tags) {
	return end(tags->tags);
}

const String *cend(const Ref<GameplayTagContainer> &tags) {
	return cend(tags->tags);
}
