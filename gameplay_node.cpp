#include "gameplay_node.h"

#include <algorithm>
#include <iterator>

namespace {
constexpr auto _on_delayed_execution = "_on_delayed_execution";
}

Node *GameplayNode::_bind_find_child(const String &class_name) const {
	for (int i = 0, n = get_child_count(); i < n; i++) {
		if (auto child = get_child(i)) {
			if (child->is_class(class_name)) {
				return child;
			}
		}
	}

	return nullptr;
}

Array GameplayNode::_bind_find_all_children(const String &class_name) const {
	Array result;

	for (int i = 0, n = get_child_count(); i < n; i++) {
		if (auto child = get_child(i)) {
			if (child->is_class(class_name)) {
				result.push_back(child);
			}
		}
	}

	return result;
}

Array GameplayNode::_bind_find_all_children_multilevel(const String &class_name) const {
	Array result;
	Vector<Node *> nodes;
	nodes.push_back(const_cast<GameplayNode *>(this));

	for (int i = 0; i < nodes.size(); i++) {
		auto node = nodes[i];

		for (int j = 0, n = node->get_child_count(); j < n; j++) {
			if (auto child = node->get_child(j)) {
				nodes.push_back(child);

				if (child->is_class(class_name)) {
					result.push_back(child);
				}
			}
		}
	}

	return result;
}

Dictionary GameplayNode::serialise() const {
	Dictionary result;
	List<PropertyInfo> properties;
	get_property_list(&properties);

	for (auto &&it = properties.front(); it; it = it->next()) {
		auto &&property = it->get();
		result[property.name] = get(property.name);
	}

	return result;
}

void GameplayNode::deserialise(const Dictionary &data) {
	List<PropertyInfo> properties;
	get_property_list(&properties);

	for (auto &&it = properties.front(); it; it = it->next()) {
		auto &&property = it->get();

		if (data.has(property.name)) {
			set(property.name, data[property.name]);
		}
	}
}

void GameplayNode::_bind_methods() {
	BIND_VMETHOD(MethodInfo(_on_delayed_execution, PropertyInfo(Variant::OBJECT, "args")));
	ClassDB::bind_method(D_METHOD("find_child", "child_class"), &GameplayNode::_bind_find_child);
	ClassDB::bind_method(D_METHOD("find_all_children", "child_class"), &GameplayNode::_bind_find_all_children);
	ClassDB::bind_method(D_METHOD("find_all_children_multilevel", "child_class"), &GameplayNode::_bind_find_all_children_multilevel);
	ClassDB::bind_method(D_METHOD("serialise"), &GameplayNode::serialise);
	ClassDB::bind_method(D_METHOD("deserialise", "data"), &GameplayNode::deserialise);
}

Dictionary GameplayResource::serialise() const {
	Dictionary result;
	List<PropertyInfo> properties;
	get_property_list(&properties);

	for (auto &&it = properties.front(); it; it = it->next()) {
		auto &&property = it->get();
		result[property.name] = get(property.name);
	}

	return result;
}

void GameplayResource::deserialise(const Dictionary &data) {
	List<PropertyInfo> properties;
	get_property_list(&properties);

	for (auto &&it = properties.front(); it; it = it->next()) {
		auto &&property = it->get();

		if (data.has(property.name)) {
			set(property.name, data[property.name]);
		}
	}
}

void GameplayResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("serialise"), &GameplayResource::serialise);
	ClassDB::bind_method(D_METHOD("deserialise", "data"), &GameplayResource::deserialise);
}
