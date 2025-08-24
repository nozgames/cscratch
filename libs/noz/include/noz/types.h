//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef u16 type_t;

constexpr type_t TYPE_INVALID = -1;
constexpr type_t TYPE_UNKNOWN = -1000;

// @object
constexpr type_t TYPE_STREAM = -900;
constexpr type_t TYPE_LIST = -901;
constexpr type_t TYPE_MAP = -902;
constexpr type_t TYPE_PROPS = -903;
constexpr type_t TYPE_MESH_BUILDER = -904;

// @asset
constexpr type_t TYPE_MATERIAL = -800;
constexpr type_t TYPE_SHADER = -801;
constexpr type_t TYPE_FONT = -802;
constexpr type_t TYPE_MESH = -803;
constexpr type_t TYPE_SOUND = -804;
constexpr type_t TYPE_TEXTURE = -805;
constexpr type_t TYPE_STYLESHEET = -806;

// @scene
constexpr type_t TYPE_ENTITY = -700;
constexpr type_t TYPE_CAMERA = -701;

