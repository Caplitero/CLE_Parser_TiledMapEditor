#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace CLE {

	enum Data_Type
	{
		Map_Data,
		Class_type,
		Enum_type,
		Tile_SET,
		Group_Layer,
		Tile_Layer,
		Object_Layer,
		Object_Body,
		Image_Layer,
	};

	class _Data_Storage {
		std::string data;
	public:
		_Data_Storage() = default;
		_Data_Storage(std::string input) : data(input) {}
		int toInt() const { return std::stoi(data); }
		float toFloat() const { return std::stof(data); }
		std::string toString() const { return data; }

		friend std::ostream& operator<<(std::ostream& output, const _Data_Storage& obj) {
			output << obj.data;
			return output;
		}
		friend std::istream& operator>>(std::istream& input, _Data_Storage& obj) {
			input >> obj.data;
			return input;
		}
	};

	void load_prop(std::unordered_map<std::string, _Data_Storage>& data, std::istream& input)
	{
		int nr_prop;
		input >> nr_prop;
		data.clear();
		for (int i = 1; i <= nr_prop; i++)
		{
			std::string NAME, DATA;
			input >> NAME >> DATA;
			data.emplace(NAME, DATA);
		}
	}

	struct _Custom_Enum_Type {
		std::string NAME;
		std::vector<std::string> Enums;

		friend std::istream& operator>>(std::istream& input, _Custom_Enum_Type& obj) {
			int children = 0;
			input >> obj.NAME >> children;
			obj.Enums.resize(children);
			for (int i = 0; i < children; i++)
				input >> obj.Enums[i];
			return input;
		}
	};

	struct _Custom_Class_Type {
		std::string NAME;
		std::unordered_map<std::string, _Data_Storage> Properties;

		friend std::istream& operator>>(std::istream& input, _Custom_Class_Type& obj) {
			input >> obj.NAME;
			load_prop(obj.Properties, input);
			return input;
		}
	};

	struct TILED_structure {
		std::string CLASS = "\0";
		std::string NAME = "\0";
		uint16_t TYPE = 0;
		std::unordered_map<std::string, _Data_Storage> Properties;
		std::vector<std::shared_ptr<TILED_structure>> Children;
	};

	struct Tile_Class : TILED_structure {
		unsigned int width = 0;
		unsigned int height = 0;
		std::vector<uint16_t> Tile_Map;

		friend std::istream& operator>>(std::istream& input, Tile_Class& obj) {
			input >> obj.width >> obj.height >> obj.NAME >> obj.CLASS;
			load_prop(obj.Properties, input);
			obj.Tile_Map.resize(obj.width * obj.height);
			for (uint16_t& tile : obj.Tile_Map)
				input >> tile;
			return input;
		}
	};

	struct Object_Base : TILED_structure {
		float x = 0;
		float y = 0;

		friend std::istream& operator>>(std::istream& input, Object_Base& obj) {
			input >> obj.x >> obj.y >> obj.NAME >> obj.CLASS;
			load_prop(obj.Properties, input);
			return input;
		}
	};

	struct _Object_Layer : TILED_structure {
		std::vector<Object_Base> objects;

		friend std::istream& operator>>(std::istream& input, _Object_Layer& obj) {
			input >> obj.NAME >> obj.CLASS;
			load_prop(obj.Properties, input);
			int children = 0;
			input >> children;
			obj.objects.resize(children);
			for (Object_Base& new_child : obj.objects) {
				input >> new_child;
				new_child.TYPE = Object_Body;
			}
			return input;
		}
	};

	struct _Tileset_Class {
		std::string IMAGE_PATH;
		std::string NAME;
		int tile_count = 0;
		int tile_width = 0;
		int tile_height = 0;

		friend std::istream& operator>>(std::istream& input, _Tileset_Class& obj) {
			input >> obj.tile_width >> obj.tile_height >> obj.tile_count >> obj.NAME >> obj.IMAGE_PATH;
			return input;
		}
	};

	struct _Image_Layer_Class : TILED_structure {
		std::string pathName = "\0";

		friend std::istream& operator>>(std::istream& input, _Image_Layer_Class& obj) {
			input >> obj.NAME >> obj.CLASS >> obj.pathName;
			load_prop(obj.Properties, input);
			return input;
		}
	};

	std::shared_ptr<TILED_structure> _Load_Tiled_Structure(std::istream& input);

	struct _Group_Layer : TILED_structure {
		friend std::istream& operator>>(std::istream& input, _Group_Layer& obj) {
			int children = 0;
			input >> obj.NAME >> obj.CLASS;
			load_prop(obj.Properties, input);
			input >> children;
			obj.Children.resize(children);
			for (std::shared_ptr<TILED_structure>& child : obj.Children) {
				child = _Load_Tiled_Structure(input);
			}
			return input;
		}
	};

	std::shared_ptr<TILED_structure> _Load_Tiled_Structure(std::istream& input)
	{
		int Type = 0;
		input >> Type;

		switch (Type) {
		case Tile_Layer: {
			auto tiled_layer = std::make_shared<Tile_Class>();
			input >> *tiled_layer;
			tiled_layer->TYPE = Type;
			return tiled_layer;
		}
		case Object_Layer: {
			auto object_layer = std::make_shared<_Object_Layer>();
			input >> *object_layer;
			object_layer->TYPE = Type;
			return object_layer;
		}
		case Image_Layer: {
			auto image_layer = std::make_shared<_Image_Layer_Class>();
			input >> *image_layer;
			image_layer->TYPE = Type;
			return image_layer;
		}
		case Group_Layer: {
			auto group_layer = std::make_shared<_Group_Layer>();
			input >> *group_layer;
			group_layer->TYPE = Type;
			return group_layer;
		}
		default:
			return nullptr;
		}
	}

	struct TME_MapFormat {
		_Group_Layer Project_root;

		void load(const std::string& MapPath)
		{
			std::ifstream in(MapPath, std::ios::binary);
			if (!in.is_open()) {
				// Handle file open error
				return;
			}

			int file_data;
			while (in >> file_data) {
				if (file_data == Map_Data)
					break;
				else if (file_data == Enum_type) {
					_Custom_Enum_Type ENUM;
					in >> ENUM;
				}
				else if (file_data == Class_type) {
					_Custom_Class_Type Ctype;
					in >> Ctype;
				}
				else if (file_data == Tile_SET) {
					_Tileset_Class SET;
					in >> SET;
				}
			}

			in >> Project_root;

			in.close();
		}
	};
}
