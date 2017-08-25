parsed_lines = []
lines = open('node_ids.csv').readlines()
for line in lines:
    parsed_lines.append(line.rstrip().split(','))

print(r'''#pragma once

#include "scada/core/namespaces.h"
#include "scada/core/standard_node_ids.h"

namespace numeric_id {
''')

for name, id, type in parsed_lines:
    print("const scada::NumericId {} = {}; // {}".format(name, id, type))

print(r'''
} // numeric_id

namespace id {
''')

for name, id, type in parsed_lines:
    print("const scada::NodeId {}{{numeric_id::{}, NamespaceIndexes::SCADA}};".format(name, name))

print(r'''
} // namespace id''')
