/*
 * EMS-ESP - https://github.com/emsesp/EMS-ESP
 * Copyright 2020-2023  Paul Derbyshire
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emsesp.h"

namespace emsesp {

using namespace std::placeholders; // for `_1` etc

WebCustomEntityService::WebCustomEntityService(AsyncWebServer * server, FS * fs, SecurityManager * securityManager)
    : _httpEndpoint(WebCustomEntity::read,
                    WebCustomEntity::update,
                    this,
                    server,
                    EMSESP_CUSTOMENTITY_SERVICE_PATH,
                    securityManager,
                    AuthenticationPredicates::IS_AUTHENTICATED)
    , _fsPersistence(WebCustomEntity::read, WebCustomEntity::update, this, fs, EMSESP_CUSTOMENTITY_FILE, FS_BUFFER_SIZE) {
}

// load the settings when the service starts
void WebCustomEntityService::begin() {
    _fsPersistence.readFromFS();
    EMSESP::logger().info("Starting Custom entity service");
    Mqtt::subscribe(EMSdevice::DeviceType::CUSTOM, "custom/#", nullptr); // use empty function callback
}

// this creates the entity file, saving it to the FS
// and also calls when the Entity web page is refreshed
void WebCustomEntity::read(WebCustomEntity & webEntity, JsonObject & root) {
    JsonArray entity  = root.createNestedArray("entities");
    uint8_t   counter = 0;
    for (const CustomEntityItem & entityItem : webEntity.customEntityItems) {
        JsonObject ei    = entity.createNestedObject();
        ei["id"]         = counter++; // id is only used to render the table and must be unique
        ei["device_id"]  = entityItem.device_id;
        ei["type_id"]    = entityItem.type_id;
        ei["offset"]     = entityItem.offset;
        ei["factor"]     = entityItem.factor;
        ei["name"]       = entityItem.name;
        ei["uom"]        = entityItem.uom;
        ei["value_type"] = entityItem.value_type;
        ei["writeable"]  = entityItem.writeable;
        EMSESP::webCustomEntityService.render_value(ei, entityItem, true, true);
    }
}

// call on initialization and also when the Entity web page is updated
// this loads the data into the internal class
StateUpdateResult WebCustomEntity::update(JsonObject & root, WebCustomEntity & webCustomEntity) {
#ifdef EMSESP_STANDALONE
    // invoke some fake data for testing
    // clang-format off
    /* prettier-ignore */
    const char * json =
        "{\"entities\": [{\"id\":0,\"device_id\":8,\"type_id\":24,\"offset\":0,\"factor\":1,\"name\":\"boiler_flowtemp\",\"uom\":1,\"value_type\":1,\"writeable\":true}]}";
    // clang-format on
    StaticJsonDocument<500> doc;
    deserializeJson(doc, json);
    root = doc.as<JsonObject>();
    Serial.println(COLOR_BRIGHT_MAGENTA);
    Serial.print(" Using fake custom entity file: ");
    serializeJson(root, Serial);
    Serial.println(COLOR_RESET);
#endif

    for (CustomEntityItem & entityItem : webCustomEntity.customEntityItems) {
        Command::erase_command(EMSdevice::DeviceType::CUSTOM, entityItem.name.c_str());
    }
    webCustomEntity.customEntityItems.clear();
    EMSESP::webCustomEntityService.ha_reset();

    if (root["entities"].is<JsonArray>()) {
        for (const JsonObject ei : root["entities"].as<JsonArray>()) {
            auto entityItem       = CustomEntityItem();
            entityItem.device_id  = ei["device_id"]; // send as numeric, will be converted to string in web
            entityItem.type_id    = ei["type_id"];
            entityItem.offset     = ei["offset"];
            entityItem.factor     = ei["factor"];
            entityItem.name       = ei["name"].as<std::string>();
            entityItem.uom        = ei["uom"];
            entityItem.value_type = ei["value_type"];
            entityItem.writeable  = ei["writeable"];

            if (entityItem.value_type == DeviceValueType::BOOL) {
                entityItem.value = EMS_VALUE_DEFAULT_BOOL;
            } else if (entityItem.value_type == DeviceValueType::INT) {
                entityItem.value = EMS_VALUE_DEFAULT_INT;
            } else if (entityItem.value_type == DeviceValueType::UINT) {
                entityItem.value = EMS_VALUE_DEFAULT_UINT;
            } else if (entityItem.value_type == DeviceValueType::SHORT) {
                entityItem.value = EMS_VALUE_DEFAULT_SHORT;
            } else if (entityItem.value_type == DeviceValueType::USHORT) {
                entityItem.value = EMS_VALUE_DEFAULT_USHORT;
            } else { // if (entityItem.value_type == DeviceValueType::ULONG || entityItem.value_type == DeviceValueType::TIME) {
                entityItem.value = EMS_VALUE_DEFAULT_ULONG;
            }
            if (entityItem.factor == 0) {
                entityItem.factor = 1;
            }
            webCustomEntity.customEntityItems.push_back(entityItem); // add to list
            if (entityItem.writeable) {
                Command::add(
                    EMSdevice::DeviceType::CUSTOM,
                    webCustomEntity.customEntityItems.back().name.c_str(),
                    [webCustomEntity](const char * value, const int8_t id) {
                        return EMSESP::webCustomEntityService.command_setvalue(value, webCustomEntity.customEntityItems.back().name);
                    },
                    FL_(entity_cmd),
                    CommandFlag::ADMIN_ONLY);
            }
        }
    }
    return StateUpdateResult::CHANGED;
}

// set value by api command
bool WebCustomEntityService::command_setvalue(const char * value, const std::string name) {
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    for (CustomEntityItem & entityItem : *customEntityItems) {
        if (Helpers::toLower(entityItem.name) == Helpers::toLower(name)) {
            if (entityItem.value_type == DeviceValueType::STRING) {
                char telegram[84];
                strlcpy(telegram, value, sizeof(telegram));
                uint8_t data[EMS_MAX_TELEGRAM_LENGTH];
                uint8_t count = 0;
                char *  p     = strtok(telegram, " ,"); // delimiter
                while (p != nullptr) {
                    data[count++] = (uint8_t)strtol(p, 0, 16);
                    p             = strtok(nullptr, " ,");
                }
                if (count == 0) {
                    return false;
                }
                EMSESP::send_write_request(entityItem.type_id, entityItem.device_id, entityItem.offset, data, count, 0);
            } else if (entityItem.value_type == DeviceValueType::BOOL) {
                bool v;
                if (!Helpers::value2bool(value, v)) {
                    return false;
                }
                EMSESP::send_write_request(entityItem.type_id, entityItem.device_id, entityItem.offset, v ? 0xFF : 0, 0);
            } else {
                float f;
                if (!Helpers::value2float(value, f)) {
                    return false;
                }
                int v = f / entityItem.factor;
                if (entityItem.value_type == DeviceValueType::UINT || entityItem.value_type == DeviceValueType::INT) {
                    EMSESP::send_write_request(entityItem.type_id, entityItem.device_id, entityItem.offset, v, 0);
                } else if (entityItem.value_type == DeviceValueType::USHORT || entityItem.value_type == DeviceValueType::SHORT) {
                    uint8_t v1[2] = {(uint8_t)(v >> 8), (uint8_t)(v & 0xFF)};
                    EMSESP::send_write_request(entityItem.type_id, entityItem.device_id, entityItem.offset, v1, 2, 0);
                } else {
                    uint8_t v1[3] = {(uint8_t)(v >> 16), (uint8_t)((v & 0xFF00) >> 8), (uint8_t)(v & 0xFF)};
                    EMSESP::send_write_request(entityItem.type_id, entityItem.device_id, entityItem.offset, v1, 3, 0);
                }
            }

            publish_single(entityItem);
            if (EMSESP::mqtt_.get_publish_onchange(0)) {
                publish();
            }
            return true;
        }
    }
    return false;
}

// output of a single value
// if add_uom is true it will add the UOM string to the value
void WebCustomEntityService::render_value(JsonObject & output, CustomEntityItem entity, const bool useVal, const bool web, const bool add_uom) {
    char        payload[12];
    std::string name = useVal ? "value" : entity.name;
    switch (entity.value_type) {
    case DeviceValueType::BOOL:
        if ((uint8_t)entity.value != EMS_VALUE_BOOL_NOTSET) {
            if (web) {
                output[name] = Helpers::render_boolean(payload, (uint8_t)entity.value, true);
            } else if (EMSESP::system_.bool_format() == BOOL_FORMAT_TRUEFALSE) {
                output[name] = (uint8_t)entity.value ? true : false;
            } else if (EMSESP::system_.bool_format() == BOOL_FORMAT_10) {
                output[name] = (uint8_t)entity.value ? 1 : 0;
            } else {
                output[name] = Helpers::render_boolean(payload, (uint8_t)entity.value);
            }
        }
        break;
    case DeviceValueType::INT:
        if ((int8_t)entity.value != EMS_VALUE_INT_NOTSET) {
            std::string v = Helpers::render_value(payload, entity.factor * (int8_t)entity.value, 2);
            output[name]  = add_uom ? serialized(v + ' ' + EMSdevice::uom_to_string(entity.uom)) : serialized(v);
        }
        break;
    case DeviceValueType::UINT:
        if ((uint8_t)entity.value != EMS_VALUE_UINT_NOTSET) {
            std::string v = Helpers::render_value(payload, entity.factor * (uint8_t)entity.value, 2);
            output[name]  = add_uom ? serialized(v + ' ' + EMSdevice::uom_to_string(entity.uom)) : serialized(v);
        }
        break;
    case DeviceValueType::SHORT:
        if ((int16_t)entity.value != EMS_VALUE_SHORT_NOTSET) {
            std::string v = Helpers::render_value(payload, entity.factor * (int16_t)entity.value, 2);
            output[name]  = add_uom ? serialized(v + ' ' + EMSdevice::uom_to_string(entity.uom)) : serialized(v);
        }
        break;
    case DeviceValueType::USHORT:
        if ((uint16_t)entity.value != EMS_VALUE_USHORT_NOTSET) {
            std::string v = Helpers::render_value(payload, entity.factor * (uint16_t)entity.value, 2);
            output[name]  = add_uom ? serialized(v + ' ' + EMSdevice::uom_to_string(entity.uom)) : serialized(v);
        }
        break;
    case DeviceValueType::ULONG:
    case DeviceValueType::TIME:
        if (entity.value != EMS_VALUE_ULONG_NOTSET) {
            std::string v = Helpers::render_value(payload, entity.factor * entity.value, 2);
            output[name]  = add_uom ? serialized(v + ' ' + EMSdevice::uom_to_string(entity.uom)) : serialized(v);
        }
        break;
    case DeviceValueType::STRING:
        if (entity.data.length() > 0) {
            output[name] = entity.data;
        }
        break;
    default:
        // EMSESP::logger().warning("unknown value type");
        break;
    }
}

// display all custom entities
// adding each one, with UOM to a json object string
void WebCustomEntityService::show_values(JsonObject & output) {
    for (const CustomEntityItem & entity : *customEntityItems) {
        render_value(output, entity, false, false, true); // with add_uom
    }
}


// process json output for info/commands and value_info
bool WebCustomEntityService::get_value_info(JsonObject & output, const char * cmd) {
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    if (Helpers::toLower(cmd) == F_(commands)) {
        output[F_(info)]     = Helpers::translated_word(FL_(info_cmd));
        output[F_(commands)] = Helpers::translated_word(FL_(commands_cmd));
        for (const auto & entity : *customEntityItems) {
            output[entity.name] = "custom entitiy";
        }
        return true;
    }

    // if no entries, return empty json
    // https://github.com/emsesp/EMS-ESP32/issues/1297
    if (customEntityItems->size() == 0) {
        return true;
    }

    if (strlen(cmd) == 0 || Helpers::toLower(cmd) == F_(values) || Helpers::toLower(cmd) == F_(info)) {
        // list all names
        for (const CustomEntityItem & entity : *customEntityItems) {
            render_value(output, entity);
        }
        return (output.size() != 0);
    }

    char command_s[30];
    strlcpy(command_s, cmd, sizeof(command_s));
    char * attribute_s = nullptr;
    // check specific attribute to fetch instead of the complete record
    char * breakp = strchr(command_s, '/');
    if (breakp) {
        *breakp     = '\0';
        attribute_s = breakp + 1;
    }

    for (const auto & entity : *customEntityItems) {
        if (Helpers::toLower(entity.name) == Helpers::toLower(command_s)) {
            output["name"] = entity.name;
            if (entity.uom > 0) {
                output["uom"] = EMSdevice::uom_to_string(entity.uom);
            }
            output["type"]      = entity.value_type == DeviceValueType::BOOL ? "boolean" : entity.value_type == DeviceValueType::STRING ? "string" : F_(number);
            output["readable"]  = true;
            output["writeable"] = entity.writeable;
            output["visible"]   = true;
            output["device_id"] = Helpers::hextoa(entity.device_id);
            output["type_id"]   = Helpers::hextoa(entity.type_id);
            output["offset"]    = entity.offset;
            if (entity.value_type != DeviceValueType::BOOL && entity.value_type != DeviceValueType::STRING) {
                output["factor"] = entity.factor;
            } else if (entity.value_type == DeviceValueType::STRING) {
                output["bytes"] = (uint8_t)entity.factor;
            }
            render_value(output, entity, true);
            if (attribute_s) {
                if (output.containsKey(attribute_s)) {
                    JsonVariant data = output[attribute_s];
                    output.clear();
                    output["api_data"] = data;
                    return true;
                } else {
                    char error[100];
                    snprintf(error, sizeof(error), "cannot find attribute %s in entity %s", attribute_s, command_s);
                    output.clear();
                    output["message"] = error;
                    return false;
                }
            }
        }

        if (output.size()) {
            return true;
        }
    }

    output["message"] = "unknown command";
    return false;
}

// publish single value
void WebCustomEntityService::publish_single(const CustomEntityItem & entity) {
    if (!Mqtt::enabled() || !Mqtt::publish_single()) {
        return;
    }
    char topic[Mqtt::MQTT_TOPIC_MAX_SIZE];
    if (Mqtt::publish_single2cmd()) {
        snprintf(topic, sizeof(topic), "%s/%s", "custom", entity.name.c_str());
    } else {
        snprintf(topic, sizeof(topic), "%s/%s", "custom_data", entity.name.c_str());
    }
    StaticJsonDocument<EMSESP_JSON_SIZE_SMALL> doc;
    JsonObject                                 output = doc.to<JsonObject>();
    render_value(output, entity, true);
    Mqtt::queue_publish(topic, output["value"].as<std::string>());
}

// publish to Mqtt
void WebCustomEntityService::publish(const bool force) {
    if (force) {
        ha_registered_ = false;
    }
    if (!Mqtt::enabled()) {
        return;
    }
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    if (customEntityItems->size() == 0) {
        return;
    }
    if (Mqtt::publish_single() && force) {
        for (const CustomEntityItem & entityItem : *customEntityItems) {
            publish_single(entityItem);
        }
    }

    DynamicJsonDocument doc(EMSESP_JSON_SIZE_XLARGE);
    JsonObject          output     = doc.to<JsonObject>();
    bool                ha_created = ha_registered_;
    for (const CustomEntityItem & entityItem : *customEntityItems) {
        render_value(output, entityItem);
        // create HA config
        if (Mqtt::ha_enabled() && !ha_registered_) {
            StaticJsonDocument<EMSESP_JSON_SIZE_MEDIUM> config;
            char                                        stat_t[50];
            snprintf(stat_t, sizeof(stat_t), "%s/custom_data", Mqtt::base().c_str());
            config["stat_t"] = stat_t;

            char val_obj[50];
            char val_cond[65];
            snprintf(val_obj, sizeof(val_obj), "value_json['%s']", entityItem.name.c_str());
            snprintf(val_cond, sizeof(val_cond), "%s is defined", val_obj);
            config["val_tpl"] = (std::string) "{{" + val_obj + " if " + val_cond + "}}";

            char uniq_s[70];
            snprintf(uniq_s, sizeof(uniq_s), "custom_%s", entityItem.name.c_str());

            config["obj_id"]  = uniq_s;
            config["uniq_id"] = uniq_s; // same as object_id
            config["name"]    = entityItem.name.c_str();

            char topic[Mqtt::MQTT_TOPIC_MAX_SIZE];
            if (entityItem.writeable) {
                if (entityItem.value_type == DeviceValueType::BOOL) {
                    snprintf(topic, sizeof(topic), "switch/%s/custom_%s/config", Mqtt::basename().c_str(), entityItem.name.c_str());
                } else if (entityItem.value_type == DeviceValueType::STRING) {
                    snprintf(topic, sizeof(topic), "sensor/%s/custom_%s/config", Mqtt::basename().c_str(), entityItem.name.c_str());
                } else if (Mqtt::discovery_type() == Mqtt::discoveryType::HOMEASSISTANT) {
                    snprintf(topic, sizeof(topic), "number/%s/custom_%s/config", Mqtt::basename().c_str(), entityItem.name.c_str());
                } else {
                    snprintf(topic, sizeof(topic), "sensor/%s/custom_%s/config", Mqtt::basename().c_str(), entityItem.name.c_str());
                }
                char command_topic[Mqtt::MQTT_TOPIC_MAX_SIZE];
                snprintf(command_topic, sizeof(command_topic), "%s/custom/%s", Mqtt::basename().c_str(), entityItem.name.c_str());
                config["cmd_t"] = command_topic;
            } else {
                if (entityItem.value_type == DeviceValueType::BOOL) {
                    snprintf(topic, sizeof(topic), "binary_sensor/%s/custom_%s/config", Mqtt::basename().c_str(), entityItem.name.c_str());
                } else {
                    snprintf(topic, sizeof(topic), "sensor/%s/custom_%s/config", Mqtt::basename().c_str(), entityItem.name.c_str());
                }
            }
            if (entityItem.value_type == DeviceValueType::BOOL) {
                // applies to both Binary Sensor (read only) and a Switch (for a command)
                if (EMSESP::system_.bool_format() == BOOL_FORMAT_TRUEFALSE) {
                    config["pl_on"]  = true;
                    config["pl_off"] = false;
                } else if (EMSESP::system_.bool_format() == BOOL_FORMAT_10) {
                    config["pl_on"]  = 1;
                    config["pl_off"] = 0;
                } else {
                    char result[12];
                    config["pl_on"]  = Helpers::render_boolean(result, true);
                    config["pl_off"] = Helpers::render_boolean(result, false);
                }
            }
            JsonObject dev = config.createNestedObject("dev");
            dev["name"]    = Mqtt::basename();
            JsonArray ids  = dev.createNestedArray("ids");
            ids.add(Mqtt::basename());

            // add "availability" section
            Mqtt::add_avty_to_doc(stat_t, config.as<JsonObject>(), val_cond);
            ha_created |= Mqtt::queue_ha(topic, config.as<JsonObject>());
        }
    }
    ha_registered_ = ha_created;
    if (output.size() > 0) {
        Mqtt::queue_publish("custom_data", output);
    }
    // EMSESP::logger().debug("publish %d custom entities", output.size());
}

// count only entities with valid value or command to show in dashboard
uint8_t WebCustomEntityService::count_entities() {
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    if (customEntityItems->size() == 0) {
        return 0;
    }

    DynamicJsonDocument doc(EMSESP_JSON_SIZE_XLARGE);
    JsonObject          output = doc.to<JsonObject>();
    uint8_t             count  = 0;
    for (const CustomEntityItem & entity : *customEntityItems) {
        render_value(output, entity);
        count += (output.containsKey(entity.name) || entity.writeable) ? 1 : 0;
    }
    return count;
}

uint8_t WebCustomEntityService::has_commands() {
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    uint8_t count = 0;
    for (const CustomEntityItem & entity : *customEntityItems) {
        count += entity.writeable ? 1 : 0;
    }
    return count;
}

// send to dashboard, msgpack don't like serialized, use number
void WebCustomEntityService::generate_value_web(JsonObject & output) {
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });

    output["label"] = (std::string) "Custom Entities";
    JsonArray data  = output.createNestedArray("data");
    uint8_t   index = 0;
    for (const CustomEntityItem & entity : *customEntityItems) {
        JsonObject obj = data.createNestedObject(); // create the object, we know there is a value
        obj["id"]      = "00" + entity.name;
        obj["u"]       = entity.uom;
        if (entity.writeable) {
            obj["c"] = entity.name;
            if (entity.value_type != DeviceValueType::BOOL && entity.value_type != DeviceValueType::STRING) {
                char s[10];
                obj["s"] = Helpers::render_value(s, entity.factor, 1);
            }
        }

        switch (entity.value_type) {
        case DeviceValueType::BOOL: {
            char s[12];
            obj["v"]    = Helpers::render_boolean(s, (uint8_t)entity.value, true);
            JsonArray l = obj.createNestedArray("l");
            l.add(Helpers::render_boolean(s, false, true));
            l.add(Helpers::render_boolean(s, true, true));
            break;
        }
        case DeviceValueType::INT:
            if ((int8_t)entity.value != EMS_VALUE_INT_NOTSET) {
                obj["v"] = Helpers::transformNumFloat(entity.factor * (int8_t)entity.value, 0);
            }
            break;
        case DeviceValueType::UINT:
            if ((uint8_t)entity.value != EMS_VALUE_UINT_NOTSET) {
                obj["v"] = Helpers::transformNumFloat(entity.factor * (uint8_t)entity.value, 0);
            }
            break;
        case DeviceValueType::SHORT:
            if ((int16_t)entity.value != EMS_VALUE_SHORT_NOTSET) {
                obj["v"] = Helpers::transformNumFloat(entity.factor * (int16_t)entity.value, 0);
            }
            break;
        case DeviceValueType::USHORT:
            if ((uint16_t)entity.value != EMS_VALUE_USHORT_NOTSET) {
                obj["v"] = Helpers::transformNumFloat(entity.factor * (uint16_t)entity.value, 0);
            }
            break;
        case DeviceValueType::ULONG:
        case DeviceValueType::TIME:
            if (entity.value != EMS_VALUE_ULONG_NOTSET) {
                obj["v"] = Helpers::transformNumFloat(entity.factor * entity.value, 0);
            }
            break;
        case DeviceValueType::STRING:
            if (entity.data.length() > 0) {
                obj["v"] = entity.data;
            }
            break;
        default:
            break;
        }
        // show only entities with value or command
        if (!obj.containsKey("v") && !obj.containsKey("c")) {
            data.remove(index);
        } else {
            index++;
        }
    }
}

// fetch telegram, called from emsesp::fetch
void WebCustomEntityService::fetch() {
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    const uint8_t len[] = {1, 1, 1, 2, 2, 3, 3};
    for (auto & entity : *customEntityItems) {
        EMSESP::send_read_request(entity.type_id,
                                  entity.device_id,
                                  entity.offset,
                                  entity.value_type == DeviceValueType::STRING ? (uint8_t)entity.factor : len[entity.value_type]);
    }
    // EMSESP::logger().debug("fetch custom entities");
}

// called on process telegram, read from telegram
bool WebCustomEntityService::get_value(std::shared_ptr<const Telegram> telegram) {
    bool has_change = false;
    EMSESP::webCustomEntityService.read([&](WebCustomEntity & webEntity) { customEntityItems = &webEntity.customEntityItems; });
    // read-length of BOOL, INT, UINT, SHORT, USHORT, ULONG, TIME
    const uint8_t len[] = {1, 1, 1, 2, 2, 3, 3};
    for (auto & entity : *customEntityItems) {
        if (entity.value_type == DeviceValueType::STRING && telegram->type_id == entity.type_id && telegram->src == entity.device_id
            && telegram->offset == entity.offset) {
            auto data = Helpers::data_to_hex(telegram->message_data, telegram->message_length);
            if (entity.data != data) {
                entity.data = data;
                if (Mqtt::publish_single()) {
                    publish_single(entity);
                } else if (EMSESP::mqtt_.get_publish_onchange(0)) {
                    has_change = true;
                }
            }
        }
        if (entity.value_type != DeviceValueType::STRING && telegram->type_id == entity.type_id && telegram->src == entity.device_id
            && telegram->offset <= entity.offset && (telegram->offset + telegram->message_length) >= (entity.offset + len[entity.value_type])) {
            uint32_t value = 0;
            for (uint8_t i = 0; i < len[entity.value_type]; i++) {
                value = (value << 8) + telegram->message_data[i + entity.offset - telegram->offset];
            }
            if (value != entity.value) {
                entity.value = value;
                if (Mqtt::publish_single()) {
                    publish_single(entity);
                } else if (EMSESP::mqtt_.get_publish_onchange(0)) {
                    has_change = true;
                }
            }
            // EMSESP::logger().debug("custom entity %s received with value %d", entity.name.c_str(), (int)entity.val);
        }
    }
    if (has_change) {
        publish();
        return true;
    }
    return false;
}

} // namespace emsesp
