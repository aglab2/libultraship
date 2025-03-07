#pragma once

#include "resource/Resource.h"
#include "resource/ResourceFactory.h"

namespace Ship {
class MatrixFactory : public ResourceFactory {
  public:
    std::shared_ptr<Resource> ReadResource(uint32_t version, std::shared_ptr<BinaryReader> reader);
};

class MatrixFactoryV0 : public ResourceVersionFactory {
  public:
    void ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) override;
};
} // namespace Ship
