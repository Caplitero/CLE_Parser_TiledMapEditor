"use strict";
/// <reference types="@mapeditor/tiled-api" />
class PropertyMember {
    constructor() {
        this.name = "";
        this.type = "";
        this.value = "";
    }
}
class PropertyType {
    constructor() {
        this.color = "";
        this.drawFill = true;
        this.id = 0;
        this.members = [];
        this.name = "";
        this.type = "";
        this.useAs = [];
        this.values = [];
    }
}
class ProjectFiles {
    constructor() {
        this.automappingRulesFile = [];
        this.commands = [];
        this.compatibilityVersion = 0;
        this.extensionsPath = "";
        this.folders = [];
        this.properties = [];
        this.propertyTypes = [];
    }
}
const OBJ_Type = {
    Map_Data: "0",
    Class_type: "1",
    Enum_type: "2",
    Tile_SET: "3",
    Group_Layer: "4",
    Tile_Layer: "5",
    Object_Layer: "6",
    Object_Body: "7",
    Image_Layer: "8",
};
function sanitizeString(input) {
    return input.toString().replace(/ /g, "_");
}
function return_name_class(object) {
    const name = sanitizeString(object.name || "0");
    const classname = sanitizeString(object.className || "0");
    return [name, classname].join(" ");
}
function return_properties(object) {
    const customProperties = object.properties();
    const buffer = [Object.keys(customProperties).length.toString()];
    for (const key in customProperties) {
        let property = `${customProperties[key]}`;
        if (property == "")
            property = "0";
        buffer.push(sanitizeString(key), sanitizeString(property));
    }
    return buffer.join(" ");
}
function load_TILESETS(map, file) {
    for (const TILESET of map.usedTilesets()) {
        const buffer = [
            OBJ_Type.Tile_SET,
            TILESET.tileWidth,
            TILESET.tileHeight,
            TILESET.tileCount,
            sanitizeString(TILESET.name),
            sanitizeString(TILESET.image),
        ];
        file.writeLine(buffer.join(" "));
    }
}
function load_TileLayer(layer, file) {
    const buffer = [
        OBJ_Type.Tile_Layer,
        layer.width.toString(),
        layer.height.toString(),
        return_name_class(layer),
        return_properties(layer),
    ];
    for (let i = 0; i < layer.height; i++) {
        for (let j = 0; j < layer.width; j++) {
            const tile = layer.tileAt(j, i);
            buffer.push(tile ? tile.id.toString() : "0");
        }
    }
    file.writeLine(buffer.join(" "));
}
function load_ObjectLayer(layer, file) {
    const ObjectLayerData = [
        OBJ_Type.Object_Layer,
        return_name_class(layer),
        return_properties(layer),
        layer.objectCount.toString()
    ];
    file.writeLine(ObjectLayerData.join(" "));
    for (let i = 0; i < layer.objectCount; i++) {
        const object = layer.objectAt(i);
        const buffer = [
            object.x.toString(),
            object.y.toString(),
            return_name_class(object),
            return_properties(object),
        ];
        file.writeLine(buffer.join(" "));
    }
}
function load_ImageLayer(layer, file) {
    let image = layer.imageSource;
    if (image == "")
        image = "0";
    const buffer = [
        OBJ_Type.Image_Layer,
        return_name_class(layer),
        sanitizeString(image),
        return_properties(layer)
    ];
    file.writeLine(buffer.join(" "));
}
function load_ClassType(data) {
    const line = [
        OBJ_Type.Class_type,
        sanitizeString(data.name),
        data.members.length.toString(),
    ];
    for (let i = 0; i < data.members.length; i++) {
        const member = data.members[i];
        const key = sanitizeString(member.name);
        let value = sanitizeString(member.value);
        if (key === "") {
            throw new Error(`Error: View -> Custom Types Editor -> ${data.name} : Property must have a name`);
        }
        if (value === "") {
            value = "0";
        }
        line.push(key, value);
    }
    return line.join(" ");
}
function load_EnumType(data) {
    const line = [
        OBJ_Type.Enum_type,
        sanitizeString(data.name),
        data.values.length.toString(),
    ];
    for (let i = 0; i < data.values.length; i++) {
        const member = sanitizeString(data.values[i]);
        if (member == "")
            throw new Error(`Error: View -> Custom Types Editor -> ${data.name} : Enum member must have a name`);
        line.push(member);
    }
    return line.join(" ");
}
function load_CustomTypes(data) {
    const buffer = [];
    for (const Type in data.propertyTypes) {
        const Properties = data.propertyTypes[Type];
        const name = sanitizeString(Properties.name);
        if (name === "") {
            throw new Error("Error: View -> Custom Types Editor: Class has no name");
        }
        if (Properties.type === "class") {
            buffer.push(load_ClassType(Properties));
        }
        if (Properties.type === "enum") {
            buffer.push(load_EnumType(Properties));
        }
    }
    return buffer.join("\n");
}
function load_GroupLayer(layer, file) {
    const buffer = [
        OBJ_Type.Group_Layer,
        return_name_class(layer),
        return_properties(layer),
        layer.layerCount.toString()
    ];
    file.writeLine(buffer.join(" "));
    load_Layers(layer, file);
}
function get_all_parents(layer) {
    let parents_list = [];
    while (layer.parentLayer) {
        parents_list.push(layer.parentLayer.name);
        layer = layer.parentLayer;
    }
    return parents_list.reverse().join("->");
}
function load_Layers(layer, file) {
    for (let i = 0; i < layer.layerCount; i++) {
        const Layer = layer.layerAt(i);
        if (Layer.name.replace(/ /g, "") == "") {
            let HER = [];
            let root = Layer;
            throw new Error(`Layers->${get_all_parents(Layer)}: Layers cannot be left unnamed.`);
        }
        if (Layer.isTileLayer) {
            load_TileLayer(Layer, file);
        }
        else if (Layer.isObjectLayer) {
            load_ObjectLayer(Layer, file);
        }
        else if (Layer.isImageLayer) {
            load_ImageLayer(Layer, file);
        }
        else if (Layer.isGroupLayer) {
            load_GroupLayer(Layer, file);
        }
    }
}
tiled.registerMapFormat("LilEditor", {
    name: "Cpp Little Editor",
    extension: "cle",
    write: (map, fileName) => {
        const file = new TextFile(fileName, TextFile.WriteOnly);
        const project_file = tiled.project.fileName;
        try {
            const tiled_project = new TextFile(project_file, TextFile.ReadOnly);
            const jsonObject = JSON.parse(tiled_project.readAll());
            tiled_project.commit();
            try {
                const NewClasses = load_CustomTypes(jsonObject);
                file.writeLine(NewClasses);
            }
            catch (error) {
                file.commit();
                return error.message;
            }
            load_TILESETS(map, file);
            let buffer = [OBJ_Type.Map_Data, "Root Project_Root 0", map.layerCount.toString()];
            file.writeLine(buffer.join(" "));
            load_Layers(map, file);
        }
        catch (error) {
            file.commit();
            return error.message;
        }
        file.commit();
        return "";
    },
});
