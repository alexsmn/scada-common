#pragma once

#include "core/node_id.h"

namespace scada {
namespace id {

const NumericId Boolean = 1;
const NumericId SByte = 2;
const NumericId Byte = 3;
const NumericId Int32 = 6;
const NumericId UInt32 = 7;
const NumericId Int64 = 8;
const NumericId Double = 11;
const NumericId ByteString = 15;
const NumericId String = 12;
const NumericId QualifiedName = 20;
const NumericId LocalizedText = 21;
const NumericId NodeId = 17;
const NumericId ExpandedNodeId = 18;

const NumericId References = 31;
const NumericId NonHierarchicalReferences = 32;
const NumericId HierarchicalReferences = 33;
const NumericId Aggregates = 44;
const NumericId HasComponent = 47;
const NumericId HasProperty = 46;
const NumericId HasChild = 34;
const NumericId Organizes = 35;
const NumericId HasTypeDefinition = 40;
const NumericId HasSubtype = 45;
const NumericId HasModellingRule = 37;

const NumericId BaseDataType = 24;
const NumericId BaseObjectType = 58;
const NumericId BaseVariableType = 62;
const NumericId FolderType = 61;
const NumericId PropertyType = 68;

const NumericId RootFolder = 84;
const NumericId ObjectsFolder = 85;
const NumericId TypesFolder = 86;

const NumericId ModellingRule_Mandatory = 78;

const NumericId EnumStrings = 11432;

} // namespace id
} // namespace scada
