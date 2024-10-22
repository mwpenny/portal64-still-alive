#include "RenderMode.h"

#include <map>

#define CREATE_RENDER_MODE_ENTRY(val)  {std::string(#val), val}

std::map<std::string, int> gRenderModeFlags = {
    CREATE_RENDER_MODE_ENTRY(AA_EN),
    CREATE_RENDER_MODE_ENTRY(Z_CMP),
    CREATE_RENDER_MODE_ENTRY(Z_UPD),
    CREATE_RENDER_MODE_ENTRY(IM_RD),
    CREATE_RENDER_MODE_ENTRY(CLR_ON_CVG),

    CREATE_RENDER_MODE_ENTRY(CVG_X_ALPHA),
    CREATE_RENDER_MODE_ENTRY(ALPHA_CVG_SEL),
    CREATE_RENDER_MODE_ENTRY(FORCE_BL),
};

std::map<std::string, int> gCVG_DST_VALUES =  {
    CREATE_RENDER_MODE_ENTRY(CVG_DST_CLAMP),
    CREATE_RENDER_MODE_ENTRY(CVG_DST_WRAP),
    CREATE_RENDER_MODE_ENTRY(CVG_DST_FULL),
    CREATE_RENDER_MODE_ENTRY(CVG_DST_SAVE),
};

std::map<std::string, int> gZMODE_VALUES =  {
    CREATE_RENDER_MODE_ENTRY(ZMODE_OPA),
    CREATE_RENDER_MODE_ENTRY(ZMODE_INTER),
    CREATE_RENDER_MODE_ENTRY(ZMODE_XLU),
    CREATE_RENDER_MODE_ENTRY(ZMODE_DEC),
};

void renderModeExtractFlags(int flags, std::vector<std::string>& output) {
    for (auto& pair : gRenderModeFlags) {
        if (flags & pair.second) {
            output.push_back(pair.first);
        }
    }

    for (auto& pair : gCVG_DST_VALUES) {
        if ((flags & CVG_DST_MASK) == pair.second) {
            output.push_back(pair.first);
            break;
        }
    }

    for (auto& pair : gZMODE_VALUES) {
        if ((flags & ZMODE_MASK) == pair.second) {
            output.push_back(pair.first);
            break;
        }
    }
} 

bool renderModeGetFlagValue(const std::string& name, int& output) {
    auto it = gRenderModeFlags.find(name);

    if (it != gRenderModeFlags.end()) {
        output = it->second;
        return true;
    }

    it = gCVG_DST_VALUES.find(name);

    if (it != gCVG_DST_VALUES.end()) {
        output = it->second;
        return true;
    }

    it = gZMODE_VALUES.find(name);

    if (it != gZMODE_VALUES.end()) {
        output = it->second;
        return true;
    }

    return false;
}

std::string gBlendModeNames[4][4] = {
    {std::string("G_BL_CLR_IN"), std::string("G_BL_CLR_MEM"), std::string("G_BL_CLR_BL"), std::string("G_BL_CLR_FOG")},
    {std::string("G_BL_A_IN"), std::string("G_BL_A_FOG"), std::string("G_BL_A_SHADE"), std::string("G_BL_0")},
    {std::string("G_BL_CLR_IN"), std::string("G_BL_CLR_MEM"), std::string("G_BL_CLR_BL"), std::string("G_BL_CLR_FOG")},
    {std::string("G_BL_1MA"), std::string("G_BL_A_MEM"), std::string("G_BL_1"), std::string("G_BL_0")},
};

int gBlendModeShift[4] = {30, 26, 22, 18};

bool renderModeGetBlendModeValue(const std::string& name, int index, int& output) {
    if (index < 0 || index >= 4) {
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        if (gBlendModeNames[index][i] == name) {
            output = i;
            return true;
        }
    }

    return false;
}

const std::string& renderModeGetBlendModeName(int blendMode, int index) {
    if (index < 0) {
        index = 0;
    }

    if (index >= 4) {
        index = 3;
    }

    int dataIndex = (blendMode >> gBlendModeShift[index]) & 0x3;
    return gBlendModeNames[index][dataIndex];
}