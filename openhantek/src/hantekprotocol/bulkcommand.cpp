#include "bulkcommand.h"

BulkCommand::BulkCommand(Hantek::BulkCode code, unsigned size): std::vector<uint8_t>(size), code(code) {}
