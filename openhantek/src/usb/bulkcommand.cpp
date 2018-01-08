#include "bulkcommand.h"

BulkCommand::BulkCommand(Hantek::BulkCode code, unsigned size): DataArray<uint8_t>(size), code(code) {}
