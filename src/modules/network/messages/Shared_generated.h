// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_SHARED_NETWORK_MESSAGES_H_
#define FLATBUFFERS_GENERATED_SHARED_NETWORK_MESSAGES_H_

#include "flatbuffers/flatbuffers.h"


namespace network {
namespace messages {

struct Vec3;
struct Vec2;
struct IVec2;

enum NpcType {
  NpcType_NONE = 0,
  NpcType_BEGIN_ANIMAL = 1,
  NpcType_ANIMAL_WOLF = 2,
  NpcType_ANIMAL_RABBIT = 3,
  NpcType_MAX_ANIMAL = 4,
  NpcType_BEGIN_CHARACTERS = 5,
  NpcType_BLACKSMITH = 6,
  NpcType_MAX_CHARACTERS = 7,
  NpcType_MAX = 8
};

inline const char **EnumNamesNpcType() {
  static const char *names[] = { "NONE", "BEGIN_ANIMAL", "ANIMAL_WOLF", "ANIMAL_RABBIT", "MAX_ANIMAL", "BEGIN_CHARACTERS", "BLACKSMITH", "MAX_CHARACTERS", "MAX", nullptr };
  return names;
}

inline const char *EnumNameNpcType(NpcType e) { return EnumNamesNpcType()[e]; }

enum NpcEffectType {
  NpcEffectType_HUMAN_CONTROLLED = 0,
  NpcEffectType_AUTONOMOUS = 1
};

inline const char **EnumNamesNpcEffectType() {
  static const char *names[] = { "HUMAN_CONTROLLED", "AUTONOMOUS", nullptr };
  return names;
}

inline const char *EnumNameNpcEffectType(NpcEffectType e) { return EnumNamesNpcEffectType()[e]; }

MANUALLY_ALIGNED_STRUCT(4) Vec3 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;
  float z_;

 public:
  Vec3(float x, float y, float z)
    : x_(flatbuffers::EndianScalar(x)), y_(flatbuffers::EndianScalar(y)), z_(flatbuffers::EndianScalar(z)) { }

  float x() const { return flatbuffers::EndianScalar(x_); }
  float y() const { return flatbuffers::EndianScalar(y_); }
  float z() const { return flatbuffers::EndianScalar(z_); }
};
STRUCT_END(Vec3, 12);

MANUALLY_ALIGNED_STRUCT(4) Vec2 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;

 public:
  Vec2(float x, float y)
    : x_(flatbuffers::EndianScalar(x)), y_(flatbuffers::EndianScalar(y)) { }

  float x() const { return flatbuffers::EndianScalar(x_); }
  float y() const { return flatbuffers::EndianScalar(y_); }
};
STRUCT_END(Vec2, 8);

MANUALLY_ALIGNED_STRUCT(4) IVec2 FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t x_;
  int32_t y_;

 public:
  IVec2(int32_t x, int32_t y)
    : x_(flatbuffers::EndianScalar(x)), y_(flatbuffers::EndianScalar(y)) { }

  int32_t x() const { return flatbuffers::EndianScalar(x_); }
  int32_t y() const { return flatbuffers::EndianScalar(y_); }
};
STRUCT_END(IVec2, 8);

}  // namespace messages
}  // namespace network

#endif  // FLATBUFFERS_GENERATED_SHARED_NETWORK_MESSAGES_H_
