/* Copyright Institute of Sound and Vibration Research - All rights reserved */

#ifndef VISR_OBJECTMODEL_AUDIO_OBJECT_HPP_INCLUDED
#define VISR_OBJECTMODEL_AUDIO_OBJECT_HPP_INCLUDED

#include "object_type.hpp"

#include <cstdint>
#include <memory>

namespace visr
{
namespace objectmodel
{

// move to header object_id.hpp later
using ObjectId = unsigned int;

using GroupId = unsigned int;

/**
 * Type use for level (gain, volume) settings, linear scale
 */
using LevelType = float;

/**
 *
 */
class AudioObject
{
public:
  explicit AudioObject( ObjectId id );

  virtual ~AudioObject() = 0;

  virtual ObjectTypeId type() const = 0;

  ObjectId id() const { return mObjectId; }

  GroupId groupId() const { return mGroupId; }

  void setObjectId( ObjectId newId );

  void setGroupId( ObjectId newId );

  LevelType getLevel() const;

  void setLevel( LevelType newLevel );

  /**
   * Must be implemented in every derived instantiated class.
   */
  virtual std::unique_ptr<AudioObject> clone() const = 0;

protected:

private:
  ObjectId mObjectId;

  GroupId mGroupId;

  LevelType mLevel;
};

} // namespace objectmodel
} // namespace visr




#endif // VISR_OBJECTMODEL_AUDIO_OBJECT_HPP_INCLUDED
