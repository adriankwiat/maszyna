#include "stdafx.h"
#include "launcher/vehicle_picker.h"
#include "renderer.h"

ui::vehiclepicker_panel::vehiclepicker_panel()
    : ui_panel(STR("Select vehicle"), false)
{
	bank.scan_textures();
}

void ui::vehiclepicker_panel::render()
{
	if (!is_open)
		return;

	static std::map<vehicle_type, std::string> type_names =
	{
	    { vehicle_type::electric_loco, STRN("Electric locos") },
	    { vehicle_type::diesel_loco, STRN("Diesel locos") },
	    { vehicle_type::steam_loco, STRN("Steam locos") },
	    { vehicle_type::railcar, STRN("Railcars") },
	    { vehicle_type::emu, STRN("EMU") },
	    { vehicle_type::utility, STRN("Utility") },
	    { vehicle_type::draisine, STRN("Draisines") },
	    { vehicle_type::tram, STRN("Trams") },
	    { vehicle_type::carriage, STRN("Carriages") },
	    { vehicle_type::truck, STRN("Trucks") },
	    { vehicle_type::bus, STRN("Buses") },
	    { vehicle_type::car, STRN("Cars") },
	    { vehicle_type::man, STRN("People") },
	    { vehicle_type::animal, STRN("Animals") },
	    { vehicle_type::unknown, STRN("Unknown") }
	};

	if (ImGui::Begin(m_name.c_str(), &is_open)) {
		ImGui::Columns(3);

		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

		if (ImGui::BeginChild("box1")) {
			for (auto const &e : type_names) {
				deferred_image *image = nullptr;
				auto it = bank.category_icons.find(e.first);
				if (it != bank.category_icons.end())
					image = &it->second;

				if (selectable_image(Translations.lookup_s(e.second).c_str(), e.first == selected_type, image))
					selected_type = e.first;
			}
		}
		ImGui::EndChild();
		ImGui::NextColumn();

		ImGui::TextUnformatted(STR_C("Group by: "));
		ImGui::SameLine();
		if (ImGui::RadioButton(STR_C("type"), !display_by_groups))
			display_by_groups = false;
		ImGui::SameLine();
		if (ImGui::RadioButton(STR_C("texture group"), display_by_groups))
			display_by_groups = true;

		std::vector<const skin_set*> skinset_list;

		if (display_by_groups) {
			std::vector<const std::string*> model_list;

			for (auto const &kv : bank.group_map) {
				const std::string &group = kv.first;

				bool model_added = false;
				bool can_break = false;
				bool map_sel_eq = (selected_group && group == *selected_group);

				for (auto const &vehicle : kv.second) {
					if (vehicle->type != selected_type)
						continue;

					for (auto const &skinset : vehicle->matching_skinsets) {
						bool map_group_eq = (skinset->group == group);
						bool sel_group_eq = (selected_group && skinset->group == *selected_group);

						if (!model_added && map_group_eq) {
							model_list.push_back(&group);
							model_added = true;
						}

						if (map_sel_eq) {
							if (sel_group_eq)
								skinset_list.push_back(skinset.get());
						}
						else if (model_added) {
							can_break = true;
							break;
						}
					}

					if (can_break)
						break;
				}
			}

			if (ImGui::BeginChild("box2")) {
				ImGuiListClipper clipper(model_list.size());
				while (clipper.Step())
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						auto group = model_list[i];

						deferred_image *image = nullptr;
						auto it = bank.group_icons.find(*group);
						if (it != bank.group_icons.end())
							image = &it->second;

						if (selectable_image(group->c_str(), group == selected_group, image))
							selected_group = group;
					}
			}
		}
		else {
			std::vector<std::shared_ptr<const vehicle_desc>> model_list;

			for (auto const &v : bank.vehicles) {
				auto desc = v.second;

				if (selected_type == desc->type && desc->matching_skinsets.size() > 0)
					model_list.push_back(desc);

				if (selected_vehicle == desc && selected_type == desc->type) {
					for (auto &skin : desc->matching_skinsets)
						skinset_list.push_back(skin.get());
				}
			}

			if (ImGui::BeginChild("box2")) {
				ImGuiListClipper clipper(model_list.size());
				while (clipper.Step())
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						auto &desc = model_list[i];

						const deferred_image *image = nullptr;
						for (auto const &skinset : desc->matching_skinsets) {
							if (skinset->mini) {
								image = &skinset->mini;
								break;
							}
						}

						std::string label = desc->path.stem().string();
						if (selectable_image(label.c_str(), desc == selected_vehicle, image))
							selected_vehicle = desc;
					}
			}
		}

		ImGui::EndChild();
		ImGui::NextColumn();

		if (ImGui::BeginChild("box3")) {
			ImGuiListClipper clipper(skinset_list.size());
			while (clipper.Step())
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					auto skin = skinset_list[i];

					//std::string label = skin->skins[0].stem().string();
					std::string label = skin->skin;
					if (selectable_image(label.c_str(), skin == selected_skinset, &skin->mini, true))
						selected_skinset = skin;
				}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();
	}
	ImGui::End();
}

bool ui::vehiclepicker_panel::selectable_image(const char *desc, bool selected, const deferred_image* image, bool pickable)
{
	bool ret = ImGui::Selectable(desc, selected, 0, ImVec2(0, 30));
	if (pickable)
		ImGui::SetItemAllowOverlap();

	if (!image)
		return ret;

	GLuint tex = image->get();
	if (tex != -1) {
		glm::ivec2 size = image->size();
		float width = 30.0f / size.y * size.x;
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
		ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

		if (pickable) {
			ImGui::PushID((void*)image);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
			ImGui::InvisibleButton(desc, ImVec2(width, 30));

			if (ImGui::BeginDragDropSource()) {
				ImGui::Image(reinterpret_cast<void*>(tex), ImVec2(width, 30), ImVec2(0, 1), ImVec2(1, 0));

				ImGui::SetDragDropPayload("skin", "aaaa", 5);
				ImGui::EndDragDropSource();
			}

			ImGui::PopID();
		}
	}

	return ret;
}
