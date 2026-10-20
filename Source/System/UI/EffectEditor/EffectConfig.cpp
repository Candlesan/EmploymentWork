#include "EffectConfig.h"
#include <fstream>
#include <windows.h>


int EffectConfig::GenerateNodeId() const
{
    int maxId = 0;
    for (const auto& node : nodes)
    {
        if (node.id > maxId) maxId = node.id;
    }
    return maxId + 1;
}

int EffectConfig::FindNodeIndex(int id) const
{
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

bool EffectConfig::IsDescendant(int candidateId, int ancestorId) const
{
    int index = FindNodeIndex(candidateId);

    // ノード数より多く上にたどったらループしているので打ち切る（壊れたデータ対策）
    for (size_t step = 0; index != -1 && step < nodes.size(); ++step)
    {
        int parentId = nodes[index].parentId;
        if (parentId == ancestorId) return true;
        index = FindNodeIndex(parentId);
    }
    return false;
}

// 保存
void EffectConfig::Save(const std::string& filePath)
{
    using json = nlohmann::json;
    json j;
    j["effectName"] = effectName;
    j["nodes"] = json::array();

    for (auto& node : nodes)
    {
        json n;

        // 基本情報
        n["id"] = node.id;
        n["parentId"] = node.parentId;

        n["name"] = node.name;
        n["type"] = static_cast<int>(node.type); // RenderType
        n["visible"] = node.visible;
        n["blendMode"] = node.blendMode;

        // 時間・ループ
        n["emitCount"] = node.emitCount;
        n["lifeTime"] = node.lifeTime;
        n["delayTime"] = node.delayTime;
        n["duration"] = node.duration;
        n["spawnInterval"] = node.spawnInterval;
        n["isLoop"] = node.isLoop;

        // フェードイン
        n["IsFadeIn"] = node.IsFadeIn;
        n["fadeInTime"] = node.fadeInTime;
        n["fadeInStartSpeed"] = node.fadeInStartSpeed;
        n["fadeInEndSpeed"] = node.fadeInEndSpeed;

        // フェードアウト
        n["IsFadeOut"] = node.IsFadeOut;
        n["fadeOutTime"] = node.fadeOutTime;
        n["fadeOutStartSpeed"] = node.fadeOutStartSpeed;
        n["fadeOutEndSpeed"] = node.fadeOutEndSpeed;

        // UV
        n["uvScrollSpeed"] = { node.uvScrollSpeed.x, node.uvScrollSpeed.y };
        n["noiseUvOffset"] = { node.noiseUvOffset.x, node.noiseUvOffset.y };

        // トランスフォーム (位置/回転/拡大)
        n["position"] = { node.position.x, node.position.y, node.position.z };
        n["positionRange"] = { node.positionRange.x, node.positionRange.y, node.positionRange.z };
        n["positionVelocity"] = { node.positionVelocity.x, node.positionVelocity.y, node.positionVelocity.z };
        n["positionVelocityRange"] = { node.positionVelocityRange.x, node.positionVelocityRange.y, node.positionVelocityRange.z };
        n["positionAcceleration"] = { node.positionAcceleration.x, node.positionAcceleration.y, node.positionAcceleration.z };
        n["positionAccelerationRange"] = { node.positionAccelerationRange.x, node.positionAccelerationRange.y, node.positionAccelerationRange.z };

        n["rotation"] = { node.rotation.x, node.rotation.y, node.rotation.z };
        n["rotationRange"] = { node.rotationRange.x, node.rotationRange.y, node.rotationRange.z };
        n["rotationVelocity"] = { node.rotationVelocity.x, node.rotationVelocity.y, node.rotationVelocity.z };
        n["rotationVelocityRange"] = { node.rotationVelocityRange.x, node.rotationVelocityRange.y, node.rotationVelocityRange.z };
        n["rotationAcceleration"] = { node.rotationAcceleration.x, node.rotationAcceleration.y, node.rotationAcceleration.z };
        n["rotationAccelerationRange"] = { node.rotationAccelerationRange.x, node.rotationAccelerationRange.y, node.rotationAccelerationRange.z };

        n["scale"] = { node.scale.x, node.scale.y, node.scale.z };
        n["scaleRange"] = { node.scaleRange.x, node.scaleRange.y, node.scaleRange.z };
        n["scaleVelocity"] = { node.scaleVelocity.x, node.scaleVelocity.y, node.scaleVelocity.z };
        n["scaleVelocityRange"] = { node.scaleVelocityRange.x, node.scaleVelocityRange.y, node.scaleVelocityRange.z };
        n["scaleAcceleration"] = { node.scaleAcceleration.x, node.scaleAcceleration.y, node.scaleAcceleration.z };
        n["scaleAccelerationRange"] = { node.scaleAccelerationRange.x, node.scaleAccelerationRange.y, node.scaleAccelerationRange.z };

        // 単一イージング（拡大縮小）
        n["scaleMode"] = static_cast<int>(node.scaleMode);
        n["scaleUniformStart"] = node.scaleUniformStart;
        n["scaleUniformStartRandom"] = node.scaleUniformStartRandom;
        n["scaleUniformEnd"] = node.scaleUniformEnd;
        n["scaleUniformEndRandom"] = node.scaleUniformEndRandom;
        n["scaleUniformStartSpeed"] = node.scaleUniformStartSpeed;
        n["scaleUniformEndSpeed"] = node.scaleUniformEndSpeed;


        n["color"] = { node.color.x, node.color.y, node.color.z, node.color.w };
        n["colorMode"] = static_cast<int>(node.colorMode);
        n["colorMin"] = { node.colorMin.x, node.colorMin.y, node.colorMin.z, node.colorMin.w };
        n["colorMax"] = { node.colorMax.x, node.colorMax.y, node.colorMax.z, node.colorMax.w };
        n["colorStartMin"] = { node.colorStartMin.x, node.colorStartMin.y, node.colorStartMin.z, node.colorStartMin.w };
        n["colorStartMax"] = { node.colorStartMax.x, node.colorStartMax.y, node.colorStartMax.z, node.colorStartMax.w };
        n["colorEndMin"] = { node.colorEndMin.x, node.colorEndMin.y, node.colorEndMin.z, node.colorEndMin.w };
        n["colorEndMax"] = { node.colorEndMax.x, node.colorEndMax.y, node.colorEndMax.z, node.colorEndMax.w };
        n["colorStartSpeed"] = node.colorStartSpeed;
        n["colorEndSpeed"] = node.colorEndSpeed;

        // 固有パラメータ郡
        // [Particle用]
        n["texturePath"] = node.texturePath;
        n["splitCount"] = { node.splitCount[0], node.splitCount[1] };
        n["spriteType"] = node.spriteType;
        n["particleFollowMode"] = node.particleFollowMode;
        n["billboardType"] = node.billboardType;
        n["stretchScale"] = node.stretchScale;

        // [Mesh用]
        n["meshPath"] = node.meshPath;
        n["noisePath"] = node.noisePath;
        n["distortionPower"] = node.distortionPower;

        // [Trail用]
        n["width"] = node.width;
        n["widthStart"] = node.widthStart;
        n["widthMiddle"] = node.widthMiddle;
        n["widthEnd"] = node.widthEnd;
        n["trailLength"] = node.trailLength;

        // [Ring用]
        n["ringDivision"] = node.ringDivision;
        n["ringOuter"] = { node.ringOuter.x, node.ringOuter.x };
        n["ringInner"] = { node.ringInner.x, node.ringInner.y };
        n["ringStartAngle"] = node.ringStartAngle;
        n["ringEndAngle"] = node.ringEndAngle;
        n["ringInnerColor"] = { node.ringInnerColor.x, node.ringInnerColor.y, node.ringInnerColor.z, node.ringInnerColor.w };
        n["ringCenterColor"] = { node.ringCenterColor.x, node.ringCenterColor.y, node.ringCenterColor.z, node.ringCenterColor.w };
        n["ringOuterColor"] = { node.ringOuterColor.x, node.ringOuterColor.y, node.ringOuterColor.z, node.ringOuterColor.w };

        j["nodes"].push_back(n);
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
                NodeConfig node;

                // ノード基本情報

                node.id = nodeJson.value("id", 0);
                node.parentId = nodeJson.value("parentId", -1);

                node.name = nodeJson.value("name", "Node");
                node.type = static_cast<RenderType>(nodeJson.value("type", 0));
                node.visible = nodeJson.value("visible", true);

                node.texturePath = nodeJson.value("texturePath", "");
                node.noisePath = nodeJson.value("noisePath", "");
                node.spriteType =  nodeJson.value("spriteType", 0);

                if (nodeJson.find("splitCount") != nodeJson.end()) {
                    node.splitCount[0] = nodeJson["splitCount"][0];
                    node.splitCount[1] = nodeJson["splitCount"][1];
                }

                node.emitCount = nodeJson.value("emitCount", 1);
                node.lifeTime  = nodeJson.value("lifeTime", 1.0f);
                node.isLoop    = nodeJson.value("isLoop", true);
                node.spawnInterval = nodeJson.value("spawnInterval", 0.1f);
                node.duration =  nodeJson.value("duration", 1.0f);
                node.delayTime = nodeJson.value("delayTime", 0.0f);
                node.blendMode = nodeJson.value("blendMode", 0.0f);

                // フェードイン
                node.IsFadeIn = nodeJson.value("IsFadeIn", false);
                node.fadeInTime = nodeJson.value("fadeInTime", 0.0f);
                node.fadeInStartSpeed = nodeJson.value("fadeInStartSpeed", 3);
                node.fadeInEndSpeed = nodeJson.value("fadeInEndSpeed", 3);

                // フェードアウト
                node.IsFadeOut = nodeJson.value("IsFadeOut", false);
                node.fadeOutTime = nodeJson.value("fadeOutTime", 0.0f);
                node.fadeOutStartSpeed = nodeJson.value("fadeOutStartSpeed", 3);
                node.fadeOutEndSpeed = nodeJson.value("fadeOutEndSpeed", 3);

                // UVスクロール
                if (nodeJson.find("uvScrollSpeed") != nodeJson.end()) node.uvScrollSpeed = { nodeJson["uvScrollSpeed"][0], nodeJson["uvScrollSpeed"][1] };
                if (nodeJson.find("noiseUvOffset") != nodeJson.end()) node.noiseUvOffset = { nodeJson["noiseUvOffset"][0], nodeJson["noiseUvOffset"][1] };

                // 位置
                if (nodeJson.find("position") != nodeJson.end()) node.position = { nodeJson["position"][0], nodeJson["position"][1], nodeJson["position"][2] };
                if (nodeJson.find("positionRange") != nodeJson.end()) node.positionRange = { nodeJson["positionRange"][0], nodeJson["positionRange"][1], nodeJson["positionRange"][2] };
                if (nodeJson.find("positionVelocity") != nodeJson.end()) node.positionVelocity = { nodeJson["positionVelocity"][0], nodeJson["positionVelocity"][1], nodeJson["positionVelocity"][2] };
                if (nodeJson.find("positionVelocityRange") != nodeJson.end()) node.positionVelocityRange = { nodeJson["positionVelocityRange"][0], nodeJson["positionVelocityRange"][1], nodeJson["positionVelocityRange"][2] };
                if (nodeJson.find("positionAcceleration") != nodeJson.end()) node.positionAcceleration = { nodeJson["positionAcceleration"][0], nodeJson["positionAcceleration"][1], nodeJson["positionAcceleration"][2] };
                if (nodeJson.find("positionAccelerationRange") != nodeJson.end()) node.positionAccelerationRange = { nodeJson["positionAccelerationRange"][0], nodeJson["positionAccelerationRange"][1], nodeJson["positionAccelerationRange"][2] };

                // 回転
                if (nodeJson.find("rotation") != nodeJson.end()) node.rotation = { nodeJson["rotation"][0], nodeJson["rotation"][1], nodeJson["rotation"][2] };
                if (nodeJson.find("rotationRange") != nodeJson.end()) node.rotationRange = { nodeJson["rotationRange"][0], nodeJson["rotationRange"][1], nodeJson["rotationRange"][2] };
                if (nodeJson.find("rotationVelocity") != nodeJson.end()) node.rotationVelocity = { nodeJson["rotationVelocity"][0], nodeJson["rotationVelocity"][1], nodeJson["rotationVelocity"][2] };
                if (nodeJson.find("rotationVelocityRange") != nodeJson.end()) node.rotationVelocityRange = { nodeJson["rotationVelocityRange"][0], nodeJson["rotationVelocityRange"][1], nodeJson["rotationVelocityRange"][2] };
                if (nodeJson.find("rotationAcceleration") != nodeJson.end()) node.rotationAcceleration = { nodeJson["rotationAcceleration"][0], nodeJson["rotationAcceleration"][1], nodeJson["rotationAcceleration"][2] };
                if (nodeJson.find("rotationAccelerationRange") != nodeJson.end()) node.rotationAccelerationRange = { nodeJson["rotationAccelerationRange"][0], nodeJson["rotationAccelerationRange"][1], nodeJson["rotationAccelerationRange"][2] };

                // 拡大率
                if (nodeJson.find("scale") != nodeJson.end()) node.scale = { nodeJson["scale"][0], nodeJson["scale"][1], nodeJson["scale"][2] };
                if (nodeJson.find("scaleRange") != nodeJson.end()) node.scaleRange = { nodeJson["scaleRange"][0], nodeJson["scaleRange"][1], nodeJson["scaleRange"][2] };
                if (nodeJson.find("scaleVelocity") != nodeJson.end()) node.scaleVelocity = { nodeJson["scaleVelocity"][0], nodeJson["scaleVelocity"][1], nodeJson["scaleVelocity"][2] };
                if (nodeJson.find("scaleVelocityRange") != nodeJson.end()) node.scaleVelocityRange = { nodeJson["scaleVelocityRange"][0], nodeJson["scaleVelocityRange"][1], nodeJson["scaleVelocityRange"][2] };
                if (nodeJson.find("scaleAcceleration") != nodeJson.end()) node.scaleAcceleration = { nodeJson["scaleAcceleration"][0], nodeJson["scaleAcceleration"][1], nodeJson["scaleAcceleration"][2] };
                if (nodeJson.find("scaleAccelerationRange") != nodeJson.end()) node.scaleAccelerationRange = { nodeJson["scaleAccelerationRange"][0], nodeJson["scaleAccelerationRange"][1], nodeJson["scaleAccelerationRange"][2] };

                // 単一イージング
                node.scaleMode = static_cast<ScaleMode>(nodeJson.value("scaleMode", static_cast<int>(ScaleMode::Fixed)));
                node.scaleUniformStart = nodeJson.value("scaleUniformStart", 1.0f);
                node.scaleUniformStartRandom = nodeJson.value("scaleUniformStartRandom", 1.0f);
                node.scaleUniformEnd = nodeJson.value("scaleUniformEnd", 1.0f);
                node.scaleUniformEndRandom = nodeJson.value("scaleUniformEndRandom", 1.0f);
                node.scaleUniformStartSpeed = nodeJson.value("scaleUniformStartSpeed", 1.0f);
                node.scaleUniformEndSpeed = nodeJson.value("scaleUniformEndSpeed", 1.0f);

                // 色
                if (nodeJson.find("color") != nodeJson.end()) node.color = { nodeJson["color"][0], nodeJson["color"][1], nodeJson["color"][2], nodeJson["color"][3] };
                node.colorMode = static_cast<ColorMode>(nodeJson.value("colorMode", static_cast<int>(ColorMode::Fixed)));
                if (nodeJson.find("colorMin") != nodeJson.end()) node.colorMin = { nodeJson["colorMin"][0], nodeJson["colorMin"][1], nodeJson["colorMin"][2], nodeJson["colorMin"][3] };
                if (nodeJson.find("colorMax") != nodeJson.end()) node.colorMax = { nodeJson["colorMax"][0], nodeJson["colorMax"][1], nodeJson["colorMax"][2], nodeJson["colorMax"][3] };
                if (nodeJson.find("colorStartMin") != nodeJson.end()) node.colorStartMin = { nodeJson["colorStartMin"][0], nodeJson["colorStartMin"][1], nodeJson["colorStartMin"][2], nodeJson["colorStartMin"][3] };
                if (nodeJson.find("colorStartMax") != nodeJson.end()) node.colorStartMax = { nodeJson["colorStartMax"][0], nodeJson["colorStartMax"][1], nodeJson["colorStartMax"][2], nodeJson["colorStartMax"][3] };
                if (nodeJson.find("colorEndMin") != nodeJson.end()) node.colorEndMin = { nodeJson["colorEndMin"][0], nodeJson["colorEndMin"][1], nodeJson["colorEndMin"][2], nodeJson["colorEndMin"][3] };
                if (nodeJson.find("colorEndMax") != nodeJson.end()) node.colorEndMax = { nodeJson["colorEndMax"][0], nodeJson["colorEndMax"][1], nodeJson["colorEndMax"][2], nodeJson["colorEndMax"][3] };
                node.colorStartSpeed = nodeJson.value("colorStartSpeed", 0.0f);
                node.colorEndSpeed = nodeJson.value("colorEndSpeed", 0.0f);

                // [Particle用]
                node.particleFollowMode = nodeJson.value("particleFollowMode", 0);
                if (nodeJson.find("billboardType") != nodeJson.end())
                    node.billboardType = nodeJson["billboardType"];
                else
                    node.billboardType = nodeJson.value("isBillboard", true) ? 1 : 0;
                node.stretchScale = nodeJson.value("stretchScale", 0.1f);

                // [Mesh用]
                node.meshPath = nodeJson.value("meshPath", "");
                node.distortionPower = nodeJson.value("distortionPower", 0.0f);

                // [Trail用]
                node.width = nodeJson.value("width", 1.0f);
                node.widthStart = nodeJson.value("widthStart", 1.0f);
                node.widthMiddle = nodeJson.value("widthMiddle", 1.0f);
                node.widthEnd = nodeJson.value("widthEnd", 0.0f);
                node.trailLength = nodeJson.value("trailLength", 0.3f);

                // [Ring用]
                node.ringDivision = nodeJson.value("ringDivision", 32); // 最低で3個にしておく多分3角形になるはず
                if (nodeJson.find("ringInner") != nodeJson.end()) node.ringInner = { nodeJson["ringInner"][0], nodeJson["ringInner"][1] };
                if (nodeJson.find("ringOuter") != nodeJson.end()) node.ringOuter = { nodeJson["ringOuter"][0], nodeJson["ringOuter"][1] };
                node.ringStartAngle = nodeJson.value("ringStartAngle  ", 0.0f);
                node.ringEndAngle = nodeJson.value("ringEndAngle    ", 360.0f);
                if (nodeJson.find("ringOuterColor") != nodeJson.end()) node.ringOuterColor = { nodeJson["ringOuterColor"][0], nodeJson["ringOuterColor"][1], nodeJson["ringOuterColor"][2], nodeJson["ringOuterColor"][3] };
                if (nodeJson.find("ringCenterColor") != nodeJson.end()) node.ringCenterColor = { nodeJson["ringCenterColor"][0], nodeJson["ringCenterColor"][1], nodeJson["ringCenterColor"][2], nodeJson["ringCenterColor"][3] };
                if (nodeJson.find("ringInnerColor") != nodeJson.end()) node.ringInnerColor = { nodeJson["ringInnerColor"][0], nodeJson["ringInnerColor"][1], nodeJson["ringInnerColor"][2], nodeJson["ringInnerColor"][3] };



                // 読み込んだノードを追加
                nodes.push_back(node);
            }
        }

        // ノードが１つも無い時既定のノードを入れる
        if (nodes.empty())
        {
            nodes.push_back(NodeConfig{});
        }

        // id が無い・重複している古いデータに id を振り直す
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            bool duplicated = false;
            for (size_t j = 0; j < i; ++j)
            {
                if (nodes[j].id == nodes[i].id) { duplicated = true; break; }
            }
            if (nodes[i].id <= 0 || duplicated)
            {
                nodes[i].id = GenerateNodeId();
            }
        }

        // 存在しない親を指していたら、親なしに戻す
        for (auto& node : nodes)
        {
            if (node.parentId != -1 && FindNodeIndex(node.parentId) == -1)
            {
                node.parentId = -1;
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