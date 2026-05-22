/* memory_mapped_datafile.cpp */
#include "ujcamei/source/retrieval/storage/memory_mapped/memory_mapped_datafile.h"

// Runtime contract: source rows are expected to be sequential with a constant
// key step. Skipping binary materialization means callers must treat the input
// file as immutable and trusted while it is mapped.

namespace cuwacunu {
namespace ujcamei {
namespace source {
namespace retrieval {
namespace storage {
namespace memory_mapped {} /* namespace memory_mapped */
} /* namespace storage */
} /* namespace retrieval */
} /* namespace source */
} /* namespace ujcamei */
} /* namespace cuwacunu */
