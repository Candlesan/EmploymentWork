#include "EffectConfig.h"
#include <fstream>
#include <windows.h>


// 保存
void EffectConfig::Save(const std::string& filePath)
{
	using json = nlohmann::json;
	json j;

    // 全体の名前
    j["effectName"] = effectName;

    // ノードリスト
    j["nodes"] = json::array();

    for (auto& node : nodes)
    {
        json nodeJson;

        // ノード基本情報
        nodeJson["name"] = node.name;
        nodeJson["type"] = static_cast<int>(node.type);
        nodeJson["visible"] = node.visible;

        // --- パーティクル設定 ---
        nodeJson["particle"]["texturePath"] = node.particle.texturePath;
        nodeJson["particle"]["spriteType"] = node.particle.spriteType;
        nodeJson["particle"]["splitCount"] = { node.particle.splitCount[0], node.particle.splitCount[1] };
        nodeJson["particle"]["emitCount"] = node.particle.emitCount;
        nodeJson["particle"]["lifeTime"] = node.particle.lifeTime;
        nodeJson["particle"]["delayTime"] = node.particle.delayTime;

        nodeJson["particle"]["isLoop"] = node.particle.isLoop;
        nodeJson["particle"]["isBillboard"] = node.particle.isBillboard;
        nodeJson["particle"]["spawnInterval"] = node.particle.spawnInterval;
        nodeJson["particle"]["duration"] = node.particle.duration;
        nodeJson["particle"]["uvScrollSpeed"] = { node.particle.uvScrollSpeed.x, node.particle.uvScrollSpeed.y };

        // 位置
        nodeJson["particle"]["position"] = { node.particle.position.x, node.particle.position.y, node.particle.position.z };
        nodeJson["particle"]["positionRange"] = { node.particle.positionRange.x, node.particle.positionRange.y, node.particle.positionRange.z };
        nodeJson["particle"]["positionVelocity"] = { node.particle.positionVelocity.x, node.particle.positionVelocity.y, node.particle.positionVelocity.z };
        nodeJson["particle"]["positionVelocityRange"] = { node.particle.positionVelocityRange.x, node.particle.positionVelocityRange.y, node.particle.positionVelocityRange.z };
        nodeJson["particle"]["positionAcceleration"] = { node.particle.positionAcceleration.x, node.particle.positionAcceleration.y, node.particle.positionAcceleration.z };
        nodeJson["particle"]["positionAccelerationRange"] = { node.particle.positionAccelerationRange.x, node.particle.positionAccelerationRange.y, node.particle.positionAccelerationRange.z };

        // 回転
        nodeJson["particle"]["rotation"] = { node.particle.rotation.x, node.particle.rotation.y, node.particle.rotation.z };
        nodeJson["particle"]["rotationRange"] = { node.particle.rotationRange.x, node.particle.rotationRange.y, node.particle.rotationRange.z };
        nodeJson["particle"]["rotationVelocity"] = { node.particle.rotationVelocity.x, node.particle.rotationVelocity.y, node.particle.rotationVelocity.z };
        nodeJson["particle"]["rotationVelocityRange"] = { node.particle.rotationVelocityRange.x, node.particle.rotationVelocityRange.y, node.particle.rotationVelocityRange.z };
        nodeJson["particle"]["rotationAcceleration"] = { node.particle.rotationAcceleration.x, node.particle.rotationAcceleration.y, node.particle.rotationAcceleration.z };
        nodeJson["particle"]["rotationAccelerationRange"] = { node.particle.rotationAccelerationRange.x, node.particle.rotationAccelerationRange.y, node.particle.rotationAccelerationRange.z };

        // 拡大
        nodeJson["particle"]["scale"] = { node.particle.scale.x, node.particle.scale.y };
        nodeJson["particle"]["scaleRange"] = { node.particle.scaleRange.x, node.particle.scaleRange.y };
        nodeJson["particle"]["scaleVelocity"] = { node.particle.scaleVelocity.x, node.particle.scaleVelocity.y, node.particle.scaleVelocity.z };
        nodeJson["particle"]["scaleVelocityRange"] = { node.particle.scaleVelocityRange.x, node.particle.scaleVelocityRange.y, node.particle.scaleVelocityRange.z };
        nodeJson["particle"]["scaleAcceleration"] = { node.particle.scaleAcceleration.x, node.particle.scaleAcceleration.y, node.particle.scaleAcceleration.z };
        nodeJson["particle"]["scaleAccelerationRange"] = { node.particle.scaleAccelerationRange.x, node.particle.scaleAccelerationRange.y, node.particle.scaleAccelerationRange.z };

        // 色
        nodeJson["particle"]["color"] = { node.particle.color.x, node.particle.color.y, node.particle.color.z, node.particle.color.w };

        // --- メッシュVFX設定 ---
        nodeJson["meshVfx"]["meshPath"] = node.meshVfx.meshPath;
        nodeJson["meshVfx"]["useMesh"] = node.meshVfx.useMesh;
        nodeJson["meshVfx"]["scale"] = { node.meshVfx.scale.x, node.meshVfx.scale.y };
        nodeJson["meshVfx"]["rotationSpeed"] = { node.meshVfx.rotationSpeed.x, node.meshVfx.rotationSpeed.y, node.meshVfx.rotationSpeed.z };
        nodeJson["meshVfx"]["color"] = { node.meshVfx.color.x, node.meshVfx.color.y, node.meshVfx.color.z, node.meshVfx.color.w };
        nodeJson["meshVfx"]["lifeTime"] = node.meshVfx.lifeTime;
        nodeJson["meshVfx"]["delayTime"] = node.meshVfx.delayTime;

        // --- トレイル設定 ---
        nodeJson["trail"]["texturePath"] = node.trail.texturePath;
        nodeJson["trail"]["width"] = node.trail.width;
        nodeJson["trail"]["color"] = { node.trail.color.x, node.trail.color.y, node.trail.color.z, node.trail.color.w };
        nodeJson["trail"]["lifeTime"] = node.trail.lifeTime;
        nodeJson["trail"]["delayTime"] = node.trail.delayTime;

        // 完成したノードデータ1個分を全体の「nodes」配列に追加
        j["nodes"].push_back(nodeJson);
    }


    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << j.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
    }
}

// 読み込み
void EffectConfig::Load(const std::string& filePath)
{
    using json = nlohmann::json;
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    json j;

    try
    {
        file >> j;
        effectName = j.value("effectName", "NewEffect");

        // 既存のノードを一度全部消してリセット！
        nodes.clear();

        // nodes 配列が存在する場合、全ノードを読み込む
        if (j.find("nodes") != j.end() && j["nodes"].is_array())
        {
            for (const auto& nodeJson : j["nodes"])
            {
                EffectNode node;

                // ノード基本情報
                node.name = nodeJson.value("name", "Node");
                node.type = static_cast<EffectNodeType>(nodeJson.value("type", 0));
                node.visible = nodeJson.value("visible", true);

                // --- パーティクル ---
                if (nodeJson.find("particle") != nodeJson.end())
                {
                    const auto& p = nodeJson["particle"];
                    node.particle.texturePath = p.value("texturePath", "");
                    node.particle.spriteType = p.value("spriteType", 0);

                    if (p.find("splitCount") != p.end()) {
                        node.particle.splitCount[0] = p["splitCount"][0];
                        node.particle.splitCount[1] = p["splitCount"][1];
                    }

                    node.particle.emitCount = p.value("emitCount", 1);
                    node.particle.lifeTime = p.value("lifeTime", 1.0f);
                    node.particle.isLoop = p.value("isLoop", true);
                    node.particle.isBillboard = p.value("isBillboard", true);
                    node.particle.spawnInterval = p.value("spawnInterval", 0.1f);
                    node.particle.duration = p.value("duration", 1.0f);
                    node.particle.delayTime = p.value("delayTime", 0.0f);
                    node.particle.spawnTimer = 0.0f;
                    if (p.find("uvScrollSpeed") != p.end()) node.particle.uvScrollSpeed = { p["uvScrollSpeed"][0], p["uvScrollSpeed"][1] };

                    // 位置
                    if (p.find("position") != p.end()) node.particle.position = { p["position"][0], p["position"][1], p["position"][2] };
                    if (p.find("positionRange") != p.end()) node.particle.positionRange = { p["positionRange"][0], p["positionRange"][1], p["positionRange"][2] };
                    if (p.find("positionVelocity") != p.end()) node.particle.positionVelocity = { p["positionVelocity"][0], p["positionVelocity"][1], p["positionVelocity"][2] };
                    if (p.find("positionVelocityRange") != p.end()) node.particle.positionVelocityRange = { p["positionVelocityRange"][0], p["positionVelocityRange"][1], p["positionVelocityRange"][2] };
                    if (p.find("positionAcceleration") != p.end()) node.particle.positionAcceleration = { p["positionAcceleration"][0], p["positionAcceleration"][1], p["positionAcceleration"][2] };
                    if (p.find("positionAccelerationRange") != p.end()) node.particle.positionAccelerationRange = { p["positionAccelerationRange"][0], p["positionAccelerationRange"][1], p["positionAccelerationRange"][2] };

                    // 回転
                    if (p.find("rotation") != p.end()) node.particle.rotation = { p["rotation"][0], p["rotation"][1], p["rotation"][2] };
                    if (p.find("rotationRange") != p.end()) node.particle.rotationRange = { p["rotationRange"][0], p["rotationRange"][1], p["rotationRange"][2] };
                    if (p.find("rotationVelocity") != p.end()) node.particle.rotationVelocity = { p["rotationVelocity"][0], p["rotationVelocity"][1], p["rotationVelocity"][2] };
                    if (p.find("rotationVelocityRange") != p.end()) node.particle.rotationVelocityRange = { p["rotationVelocityRange"][0], p["rotationVelocityRange"][1], p["rotationVelocityRange"][2] };
                    if (p.find("rotationAcceleration") != p.end()) node.particle.rotationAcceleration = { p["rotationAcceleration"][0], p["rotationAcceleration"][1], p["rotationAcceleration"][2] };
                    if (p.find("rotationAccelerationRange") != p.end()) node.particle.rotationAccelerationRange = { p["rotationAccelerationRange"][0], p["rotationAccelerationRange"][1], p["rotationAccelerationRange"][2] };

                    // 拡大
                    if (p.find("scale") != p.end()) node.particle.scale = { p["scale"][0], p["scale"][1] };
                    if (p.find("scaleRange") != p.end()) node.particle.scaleRange = { p["scaleRange"][0], p["scaleRange"][1] };
                    if (p.find("scaleVelocity") != p.end()) node.particle.scaleVelocity = { p["scaleVelocity"][0], p["scaleVelocity"][1], p["scaleVelocity"][2] };
                    if (p.find("scaleVelocityRange") != p.end()) node.particle.scaleVelocityRange = { p["scaleVelocityRange"][0], p["scaleVelocityRange"][1], p["scaleVelocityRange"][2] };
                    if (p.find("scaleAcceleration") != p.end()) node.particle.scaleAcceleration = { p["scaleAcceleration"][0], p["scaleAcceleration"][1], p["scaleAcceleration"][2] };
                    if (p.find("scaleAccelerationRange") != p.end()) node.particle.scaleAccelerationRange = { p["scaleAccelerationRange"][0], p["scaleAccelerationRange"][1], p["scaleAccelerationRange"][2] };

                    // 色
                    if (p.find("color") != p.end()) node.particle.color = { p["color"][0], p["color"][1], p["color"][2], p["color"][3] };
                }

                // --- メッシュVFX ---
                if (nodeJson.find("meshVfx") != nodeJson.end())
                {
                    const auto& m = nodeJson["meshVfx"];
                    node.meshVfx.meshPath = m.value("meshPath", "");
                    node.meshVfx.useMesh = m.value("useMesh", false);
                    node.meshVfx.lifeTime = m.value("lifeTime", 1.0f);
                    node.meshVfx.delayTime = m.value("delayTime", 0.0f);

                    if (m.find("color") != m.end()) node.meshVfx.color = { m["color"][0], m["color"][1], m["color"][2], m["color"][3] };
                    if (m.find("scale") != m.end()) node.meshVfx.scale = { m["scale"][0], m["scale"][1] };
                    if (m.find("rotationSpeed") != m.end()) node.meshVfx.rotationSpeed = { m["rotationSpeed"][0], m["rotationSpeed"][1], m["rotationSpeed"][2] };
                }

                // --- トレイル ---
                if (nodeJson.find("trail") != nodeJson.end())
                {
                    const auto& t = nodeJson["trail"];
                    node.trail.texturePath = t.value("texturePath", "Data/Sprite/Trail/T_TrailMask_09.png");
                    node.trail.width = t.value("width", 1.0f);
                    node.trail.lifeTime = t.value("lifeTime", 1.0f);
                    node.trail.delayTime = t.value("delayTime", 0.0f);

                    if (t.find("color") != t.end()) node.trail.color = { t["color"][0], t["color"][1], t["color"][2], t["color"][3] };
                }

                // 読み込んだノードを追加
                nodes.push_back(node);
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        OutputDebugStringA("JSON Load Error: ");
        OutputDebugStringA(e.what());
        OutputDebugStringA("\n");
        *this = EffectConfig();
    }
}